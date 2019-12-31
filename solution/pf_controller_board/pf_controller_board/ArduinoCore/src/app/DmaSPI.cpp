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
#include "DmaSPI.h"
#include "DmaCommon.h"
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
settings(SPISettings(0, MSBFIRST, SPI_MODE0)),
dma_rx(dma_rx_channel, nullptr), dma_tx(dma_tx_channel, nullptr),
_data_register(reinterpret_cast<uint32_t>(&p_sercom->getSercomPointer()->SPI.DATA.reg))
{
  initialized = false;
  assert(p_sercom != nullptr);
  _p_sercom = p_sercom;

  // pins
  _uc_pinMiso = uc_pinMISO;
  _uc_pinSCK = uc_pinSCK;
  _uc_pinMosi = uc_pinMOSI;
  _uc_pinSS = uc_pinSS;

  // SERCOM pads
  _padTx=PadTx;
  _padRx=PadRx;
  
  // initialize the buffer memory
  _tw_buffer = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
  _tx_buffer = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
  _rx_buffer = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
  _rw_buffer = new uint8_t[DMA_SPI_BUFFER_SIZE]{0};
}

DmaSPISlaveClass::~DmaSPISlaveClass() {
  delete[] _tw_buffer;
  delete[] _tx_buffer;
  delete[] _rx_buffer;
  delete[] _rw_buffer;
}

void DmaSPISlaveClass::begin()
{
  DmacDescriptor* desc;
  
  init();
  
  // PIO init
  pinPeripheral(_uc_pinMiso, g_APinDescription[_uc_pinMiso].ulPinType);
  pinPeripheral(_uc_pinSCK, g_APinDescription[_uc_pinSCK].ulPinType);
  pinPeripheral(_uc_pinMosi, g_APinDescription[_uc_pinMosi].ulPinType);
  pinPeripheral(_uc_pinSS, g_APinDescription[_uc_pinSS].ulPinType);

  // config(settings);
  // config(DEFAULT_SPI_SETTINGS);
  
  config(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  
  // configure DMA
  dma_tx.stop();
  dma_rx.stop();
  dma_tx.setupTxDescFirst(_data_register, const_cast<uint8_t*>(_w_buffer), buffer_size);
  dma_rx.setupRxDescFirst(_data_register, const_cast<uint8_t*>(_w_buffer), buffer_size);
  dma_tx.setupTxConfig(_p_sercom->getSercomId(), 0);  // setup Tx with priority 0
  dma_rx.setupRxConfig(_p_sercom->getSercomId(), 0);  // setup Tx with priority 0
  desc = dma_tx.getDescFirst();
  desc->DESCADDR.reg = reinterpret_cast<uint32_t>(desc);  // point back to itself, never ending loop
  desc = dma_rx.getDescFirst();
  desc->DESCADDR.reg = reinterpret_cast<uint32_t>(desc);  // point back to itself, never ending loop
  dma_tx.start();
  dma_rx.start();
}

void DmaSPISlaveClass::init()
{
  if (initialized)
    return;
  initialized = true;
}

void DmaSPISlaveClass::config(SPISettings settings)
{
  if (this->settings != settings) {
    this->settings = settings;
    _p_sercom->disableSPI();

    _p_sercom->initSPISlave(_padTx, _padRx, SPI_CHAR_SIZE_8_BITS, settings.bitOrder);
    _p_sercom->initSPISlaveClock(settings.dataMode);
    _p_sercom->enableSPI();
  }
}

void DmaSPISlaveClass::end()
{
  _p_sercom->resetSPI();
  initialized = false;
}


void DmaSPISlaveClass::setBitOrder(BitOrder order)
{
  if (order == LSBFIRST) {
    _p_sercom->setDataOrderSPI(LSB_FIRST);
  } else {
    _p_sercom->setDataOrderSPI(MSB_FIRST);
  }
}

void DmaSPISlaveClass::setDataMode(uint8_t mode)
{
  switch (mode)
  {
    case SPI_MODE0:
      _p_sercom->setClockModeSPI(SERCOM_SPI_MODE_0);
      break;

    case SPI_MODE1:
      _p_sercom->setClockModeSPI(SERCOM_SPI_MODE_1);
      break;

    case SPI_MODE2:
      _p_sercom->setClockModeSPI(SERCOM_SPI_MODE_2);
      break;

    case SPI_MODE3:
      _p_sercom->setClockModeSPI(SERCOM_SPI_MODE_3);
      break;

    default:
      break;
  }
}

uint8_t* DmaSPISlaveClass::getTxDataPtr() {
  return const_cast<uint8_t*>(_tw_buffer);
}

void DmaSPISlaveClass::queueTxData() {
  noInterrupts();
  _tx_pending = true;
  // swap_ptrs(_tw_buffer, _tx_buffer);
  interrupts();
}
  
uint8_t* DmaSPISlaveClass::getRxDataPtr() {
  uint8_t* ptr = nullptr;
  if(_rx_pending) {
    if(digitalRead(_uc_pinSS)) {
      noInterrupts();
      if(_rx_pending) {
        _rw_buffer[0] = 0;  // clear first byte, assume this will flag the buffer as empty
        // swap_ptrs(_w_buffer, _rw_buffer);
        ptr = const_cast<uint8_t*>(_rw_buffer);
        _rx_pending = false;
      }    
      interrupts();
    }      
  }
  return ptr;
}

void DmaSPISlaveClass::dataIn() {
  uint32_t x; 
  uint32_t idx;
  x = 0;
  while(dma_rx.isPending() || dma_rx.isBusy()) { }
  while (x == 0) {  // assuming that dma has started, working count should not stay 0 (possibly 0 only if loading desc)
    x = dma_rx.getWorkingCount();
  }
  x = buffer_size - x;
  for (uint32_t i=1; i<buffer_size; i++) {
    idx = (i + x) & buffer_mask;
    _rx_buffer[i] = _w_buffer[idx];
  }
  _rx_pending = true;
}

void DmaSPISlaveClass::dataOut() {
  uint32_t x;
  uint32_t idx;
  x = 0;
  while(dma_tx.isPending() || dma_tx.isBusy()) { }
  while (x == 0) {  // assuming that dma has started, working count should not stay 0 (possibly 0 only if loading desc)
    x = dma_tx.getWorkingCount();   
  }
  x = buffer_size - x;
  _p_sercom->transferDataSPI(_w_buffer[x]);  // write first byte
  for (uint32_t i=1; i<buffer_size; i++) {
    idx = (i + x) & buffer_mask;
    _w_buffer[idx] = _tx_buffer[i];
  }
}
void DmaSPISlaveClass::ssInterrupt() {    
  auto ss = PORT->Group[g_APinDescription[_uc_pinSS].ulPort].IN.reg & PORT->Group[g_APinDescription[_uc_pinSS].ulPort].IN.reg;

  _p_sercom->clearSpiInterruptFlags();
  if(ss) { // rising edge (end of transfer)
    dataIn();
  } else {  // falling edge (start of transfer)
    dataOut();
    for (int i=0; i<buffer_size; i++) {
      _w_buffer[i] = i;
    }
  }
  _p_sercom->clearSpiInterruptFlags();
}


DmaSPISlaveClass SPI (&PERIPH_SPI, 0, 1,  PIN_SPI_MISO,  PIN_SPI_SCK,  PIN_SPI_MOSI, SS, SPISlaveEquivalentTxPad(PAD_SPI_TX, PAD_SPI_RX), SPISlaveEquivalentRxPad(PAD_SPI_TX, PAD_SPI_RX));
