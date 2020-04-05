/*
 * SPI Master library for Arduino Zero.
 * Copyright (c) 2015 Arduino LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Arduino.h>
#include "dma_spi.h"
#include "dma_common.h"
#include <wiring_private.h>
#include <assert.h>

#define SPI_IMODE_NONE   0
#define SPI_IMODE_EXTINT 1
#define SPI_IMODE_GLOBAL 2

template<typename T>
void swap_ptrs(T& a, T& b) {
  T temp = b;
  b = a;
  a = temp;
}

static void SPISlaveEquivalentPad(SercomSpiTXSlavePad& new_tx_pad, SercomSpiRXSlavePad& new_rx_pad, SercomSpiTXPad original_txpad, SercomRXPad original_rxpad) {
  int mosi_pad, miso_pad, sck_pad;
  switch(original_txpad) {
    case 	SPI_PAD_0_SCK_1:
        mosi_pad = 0;
        sck_pad = 1;
      break;
	  case SPI_PAD_2_SCK_3:
        mosi_pad = 2;
        sck_pad = 3;
      break;
	  case SPI_PAD_3_SCK_1:
        mosi_pad = 3;
        sck_pad = 1;
      break;
	  case SPI_PAD_0_SCK_3:
        mosi_pad = 0;
        sck_pad = 3;
      break;
    default:
      assert("Unknown pad");
  }
  miso_pad = static_cast<int>(original_rxpad);
  if (miso_pad == 0 && sck_pad == 1) {
    new_tx_pad = SPI_SLAVE_PAD_0_SCK_1_SS_2;
  } else if (miso_pad == 2 && sck_pad == 3) {
    new_tx_pad = SPI_SLAVE_PAD_2_SCK_3_SS_1;
  } else if (miso_pad == 3 && sck_pad == 1) {
    new_tx_pad = SPI_SLAVE_PAD_3_SCK_1_SS_2;
  } else if (miso_pad == 0 && sck_pad == 3) {
    new_tx_pad = SPI_SLAVE_PAD_0_SCK_3_SS_1;
  } else {
    assert("No possible conversion from SPI master MOSI, MISO, SCK to Slave equivalent, you need to define your own pad");
  }
  new_rx_pad = static_cast<SercomSpiRXSlavePad>(mosi_pad);
}

static SercomSpiTXSlavePad SPISlaveEquivalentTxPad(SercomSpiTXPad original_txpad, SercomRXPad original_rxpad) {
  SercomSpiTXSlavePad new_tx_pad;
  SercomSpiRXSlavePad new_rx_pad;
  SPISlaveEquivalentPad(new_tx_pad, new_rx_pad, original_txpad, original_rxpad);
  return new_tx_pad;
}

static SercomSpiRXSlavePad SPISlaveEquivalentRxPad(SercomSpiTXPad original_txpad, SercomRXPad original_rxpad) {
  SercomSpiTXSlavePad new_tx_pad;
  SercomSpiRXSlavePad new_rx_pad;
  SPISlaveEquivalentPad(new_tx_pad, new_rx_pad, original_txpad, original_rxpad);
  return new_rx_pad;
}

DmaSPISlaveClass::DmaSPISlaveClass(XSERCOM *p_sercom,uint8_t dma_rx_channel, uint8_t dma_tx_channel, uint8_t uc_pinMISO, uint8_t uc_pinSCK, uint8_t uc_pinMOSI, uint8_t uc_pinSS, SercomSpiTXSlavePad PadTx, SercomSpiRXSlavePad PadRx) 
: 
dma_rx_(dma_rx_channel, Callback{}), dma_tx_(dma_tx_channel, Callback{}),
data_register_(p_sercom->dataAddressSPI)
{
  initialized_ = false;
  assert(p_sercom != nullptr);
  p_sercom_ = p_sercom;

  // pins
  pin_miso_ = uc_pinMISO;
  pin_sck_ = uc_pinSCK;
  pin_mosi_ = uc_pinMOSI;
  pin_ss_ = uc_pinSS;

  // SERCOM pads
  pad_tx_=PadTx;
  pad_rx_=PadRx;
  
  // initialize the buffer memory
  p_tw_buffer_ = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
  p_tx_buffer_ = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
  p_rx_buffer_ = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
  p_rw_buffer_ = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
    
  _prev_rem = 0;
}

DmaSPISlaveClass::~DmaSPISlaveClass() {
  delete[] p_tw_buffer_;
  delete[] p_tx_buffer_;
  delete[] p_rx_buffer_;
  delete[] p_rw_buffer_;
}

void DmaSPISlaveClass::begin()
{
  DmacDescriptor* desc;
  
  init();
  
  // PIO init
  pinPeripheral(pin_miso_, g_APinDescription[pin_miso_].ulPinType);
  pinPeripheral(pin_sck_, g_APinDescription[pin_sck_].ulPinType);
  pinPeripheral(pin_mosi_, g_APinDescription[pin_mosi_].ulPinType);
  pinPeripheral(pin_ss_, g_APinDescription[pin_ss_].ulPinType);

  dma_tx_.stop();
  dma_rx_.stop();

  // config(settings);
  config();
  // _p_sercom->disableSPI();
  // config(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  
  // configure DMA
  uint8_t* p_work = const_cast<uint8_t*>(&w_buffer_[0]);
  dma_tx_.setupTxDescFirst(data_register_, p_work, buffer_size);
  dma_rx_.setupRxDescFirst(data_register_, p_work, buffer_size);
  desc = dma_tx_.getDescFirst();
  desc->DESCADDR.reg = reinterpret_cast<uint32_t>(desc);  // point back to itself, never ending loop
  desc = dma_rx_.getDescFirst();
  desc->DESCADDR.reg = reinterpret_cast<uint32_t>(desc);  // point back to itself, never ending loop
  dma_tx_.setupTxConfig(p_sercom_->getSercomId(), 0);  // setup Tx with priority 1
  dma_rx_.setupRxConfig(p_sercom_->getSercomId(), 0);  // setup Rx with priority 0
  
  
  dma_tx_.start();
  dma_rx_.start();
  
  p_sercom_->enableSPI();
  
}

void DmaSPISlaveClass::init()
{
  if (initialized_)
    return;
  initialized_ = true;
}

void DmaSPISlaveClass::config()
{
  p_sercom_->disableSPI();

  p_sercom_->initSPISlave(pad_tx_, pad_rx_, SPI_CHAR_SIZE_8_BITS, MSB_FIRST);
  p_sercom_->initSPISlaveClock(SERCOM_SPI_MODE_2);
  // _p_sercom->enableSPI();
}

void DmaSPISlaveClass::end()
{
  dma_tx_.stop();
  dma_rx_.stop();
  p_sercom_->resetSPI();
  initialized_ = false;
}

uint8_t* DmaSPISlaveClass::getTxDataPtr() {
  return const_cast<uint8_t*>(p_tw_buffer_);
}

void DmaSPISlaveClass::queueTxData() {
  noInterrupts();
  tx_pending_ = true;
  // swap_ptrs(_tw_buffer, _tx_buffer);
  interrupts();
}
  
uint8_t* DmaSPISlaveClass::getRxDataPtr() {
  uint8_t* ptr = nullptr;
  if(rx_pending_) {
    if(digitalRead(pin_ss_)) {
      noInterrupts();
      if(rx_pending_) {
        p_rw_buffer_[0] = 0;  // clear first byte, assume this will flag the buffer as empty
        // swap_ptrs(_w_buffer, _rw_buffer);
        ptr = const_cast<uint8_t*>(p_rw_buffer_);
        rx_pending_ = false;
      }    
      interrupts();
    }      
  }
  return ptr;
}

void DmaSPISlaveClass::endTransactionInterrupt() {
  uint32_t cur_rem; 
  uint32_t cur_last;
  uint32_t idx;
  cur_rem = 0;
  while(dma_rx_.isPending() || dma_rx_.isBusy()) { }
  do {  // assuming that dma has started, working count should not stay 0 (possibly 0 only if loading desc)
    cur_rem = dma_rx_.getWorkingCount();
  } while( cur_rem == 0 );
  
  // dma_rx.suspendChannel();
  idx = _prev_rem;
  cur_last = buffer_size - cur_rem - 1;
  for (uint32_t i=0; i<buffer_size; i++) {
    p_rx_buffer_[i] = w_buffer_[idx];
    if (idx == cur_last) {
 
      break;
    }
    idx = (idx + 1) & buffer_mask;
  }
  // dma_rx.resumeChannel();
  _prev_rem = cur_rem;
  rx_pending_ = true;
  
  p_sercom_->clearSpiInterruptFlags();
}

void DmaSPISlaveClass::startTransactionInterrupt() {
  
  int i;
  p_tx_buffer_[i]++;  // test load
  for (i=1; i<buffer_size; i++) {  // test load
    p_tx_buffer_[i] = i;
  } 
  
  
  
  uint32_t x;
  uint32_t idx;
  while(dma_tx_.isPending() || dma_tx_.isBusy()) { }
  do {  // assuming that dma has started, working count should not stay 0 (possibly 0 only if loading desc)
    x = dma_tx_.getWorkingCount();   
  } while (x == 0);
  x = buffer_size - x;
  // _p_sercom->transferDataSPI(_w_buffer[x]);  // write first byte
  for (uint32_t i=0; i<buffer_size; i++) {
    idx = (i + x) & buffer_mask;
    w_buffer_[idx] = p_tx_buffer_[i];
  }
  p_sercom_->clearSpiInterruptFlags();
}

static XSERCOM SercomSPI{SERCOM4};
DmaSPISlaveClass SPI (&SercomSPI, 0, 1,  PIN_SPI_MISO,  PIN_SPI_SCK,  PIN_SPI_MOSI, SS, SPISlaveEquivalentTxPad(PAD_SPI_TX, PAD_SPI_RX), SPISlaveEquivalentRxPad(PAD_SPI_TX, PAD_SPI_RX));
