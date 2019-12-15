/*
 * DmaUartMaster.cpp
 *
 * Created: 3/11/2019 3:32:56 PM
 *  Author: Joseph
 */ 

#include "Arduino.h"
#include "dmac.h"
#include "DmaUart.h"
#include "SERCOM.h"
#include "HardwareSerial.h"
#include "wiring_private.h"
#include "DmaCommon.h"
#include "limits.h"

DmaContinuousReader::DmaContinuousReader(uint8_t _dma_channel, XSERCOM* _sercom)
: dma_(_dma_channel, this), sercom(_sercom), 
read_(0), write_(0), work_idx_(0), 
dma_channel(_dma_channel){
}

void DmaContinuousReader::callback(int) {
  volatile uint32_t read_block_idx;
  
  // push read counter out of current block
  __disable_irq();
  work_idx_ = (work_idx_ + 1) & num_blocks_mask;
  read_block_idx =  (read_ >> block_shift) & num_blocks_mask;
  if (read_block_idx == work_idx_) {
    read_ = (read_ & ~block_data_mask) + block_data_size;
  }
  write_ += block_data_size;
  __enable_irq();
}

int DmaContinuousReader::readByte() {
  int val;
  uint32_t read, write, work_idx, diff;
  __disable_irq();
  read = read_;
  write = write_;
  work_idx = work_idx_;
  __enable_irq();
  
  diff = write - read; 
  if(diff < 0x7FFFFFFF && diff > 0) {
    val = static_cast<int>(buffer_[read & buffer_index_mask]);
    __disable_irq();
    read_++;
    __enable_irq();
  } else if(available()) {
    val = static_cast<int>(buffer_[read & buffer_index_mask]);
    __disable_irq();
    read_++;
    __enable_irq();
  } else {
    val = -1;
  }
  return val;
}

uint32_t DmaContinuousReader::available() {
  uint32_t diff;
  uint32_t read;
  
  read = read_;
  diff = getWrittenCount();
  diff -= read;
  
  if (diff > 0x7FFFFFFF) {
    diff = 0;
  }
  return diff;
}

void DmaContinuousReader::start() {
  dma_.stop();
  read_ = 0;
  write_ = 0;
  work_idx_ = 0;
  setup();
  dma_.setupRxConfig(sercom->getSercomId());
  dma_.start();
}

void DmaContinuousReader::stop() {
  dma_.stop();
}


uint32_t DmaContinuousReader::getWrittenCount() {
  volatile uint32_t write, working_remaining, next_addr;
  DmacDescriptor* volatile desc = Dma::workingDesc(dma_channel);
  // do {
  //  next_addr = desc->DSTADDR.reg;
  //  write = write_;
  //  working_remaining = desc->BTCNT.reg;
  // } while(working_remaining != desc->BTCNT.reg || write != write_ || next_addr != desc->DSTADDR.reg || dma_.isPending());
  do {
    next_addr = desc->DSTADDR.reg;
    working_remaining = desc->BTCNT.reg;
    write = write_;
  } while(working_remaining != desc->BTCNT.reg || next_addr != desc->DSTADDR.reg);
  if(working_remaining < 1) {
    working_remaining = 1;
  }
  return write + (block_data_size - working_remaining);
}

void DmaContinuousReader::setup(){
  uint32_t rx_source_address = reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg));
  // setup rx information
  dma_.setupRxConfig(sercom->getSercomId());
  
  // setup descriptor information
  dma_.setupRxDescFirst(rx_source_address, const_cast<uint8_t*>(buffer_), block_data_size);
  for(int i=0; i<num_blocks - 1; i++) {
    dma_.setupRxDescAny(&(descriptors[i]), rx_source_address, const_cast<uint8_t*>(&buffer_[(i + 1) * block_data_size]), block_data_size);
  }
  // link all descriptors
  dma_.ch_desc->DESCADDR.reg = reinterpret_cast<uint32_t>(&(descriptors[0]));
  for(int i=0; i<num_blocks - 2; i++) {
    descriptors[i].DESCADDR.reg = reinterpret_cast<uint32_t>(&(descriptors[i + 1]));
  }
  descriptors[num_blocks - 2].DESCADDR.reg = reinterpret_cast<uint32_t>(dma_.ch_desc);
}


DmaOneOffWriter::DmaOneOffWriter(uint8_t _dma_channel, XSERCOM* _sercom) 
: dma_(_dma_channel, this), sercom(_sercom), dma_channel(_dma_channel), is_busy(false) {
}

void DmaOneOffWriter::write(uint8_t* _tx_buf, uint32_t _tx_len) {
  dma_.stop();
  dma_.setupTxDescFirst(reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg)), _tx_buf, _tx_len);
  dma_.setupTxConfig(sercom->getSercomId());
  dma_.start();
  is_busy = true;
}

void DmaOneOffWriter::callback(int status) {
  is_busy = false;
  dma_.stop();
  if (post_transfer_callback != nullptr) {
    post_transfer_callback->callback(status);
  }
}

bool DmaOneOffWriter::isBusy() {
  return is_busy;
}

DmaUart::DmaUart(XSERCOM *_s, uint8_t _rx_dma_channel, uint8_t _tx_dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX) 
: sercom(_s), 
reader(_rx_dma_channel, _s), 
writer(_tx_dma_channel, _s)
{
    sercom = _s;
    uc_pinRX = _pinRX;
    uc_pinTX = _pinTX;
    uc_padRX = _padRX ;
    uc_padTX = _padTX;
}

void DmaUart::begin(unsigned long baudrate)
{
  begin(baudrate, SERIAL_8N1);
}

void DmaUart::begin(unsigned long baudrate, uint16_t config) {
  pinPeripheral(uc_pinRX, g_APinDescription[uc_pinRX].ulPinType);
  pinPeripheral(uc_pinTX, g_APinDescription[uc_pinTX].ulPinType);
  
  sercom->initUART(UART_INT_CLOCK, SAMPLE_RATE_x16, baudrate);
  sercom->initFrame(extractCharSize(config), LSB_FIRST, extractParity(config), extractNbStopBit(config));
  sercom->initPads(uc_padTX, uc_padRX);
  NVIC_DisableIRQ(sercom->getIRQn());
  // clear some interrupts
  SERCOM2->USART.INTENCLR.bit.CTSIC = 1;
  SERCOM2->USART.INTENCLR.bit.DRE = 1;
  SERCOM2->USART.INTENCLR.bit.RXBRK = 1;
  SERCOM2->USART.INTENCLR.bit.RXC = 1;
  SERCOM2->USART.INTENCLR.bit.RXS = 1;
  // SERCOM2->USART.INTENCLR.bit.TXC = 1;
  SERCOM2->USART.INTENSET.bit.TXC = 1;
  NVIC_SetPriority(sercom->getIRQn(), 2);
  NVIC_EnableIRQ(sercom->getIRQn());
  sercom->enableUART();
  
  reader.start();
}

void DmaUart::write(uint8_t* _tx_buf, uint32_t _tx_len) {
  writer.write(_tx_buf, _tx_len);
}

bool DmaUart::isTransmitting() {
  return !writer.isBusy();
}

void DmaUart::stopTransmit() {
  return writer.stopTransfer();
}

int DmaUart::available() {
  return reader.available();
}

int DmaUart::read() {
  return reader.readByte();
}

SercomNumberStopBit DmaUart::extractNbStopBit(uint16_t config) {
  switch(config & HARDSER_STOP_BIT_MASK)
  {
    case HARDSER_STOP_BIT_1:
    default:
    return SERCOM_STOP_BIT_1;

    case HARDSER_STOP_BIT_2:
    return SERCOM_STOP_BITS_2;
  }
}

SercomUartCharSize DmaUart::extractCharSize(uint16_t config)
{
  switch(config & HARDSER_DATA_MASK)
  {
    case HARDSER_DATA_5:
    return UART_CHAR_SIZE_5_BITS;

    case HARDSER_DATA_6:
    return UART_CHAR_SIZE_6_BITS;

    case HARDSER_DATA_7:
    return UART_CHAR_SIZE_7_BITS;

    case HARDSER_DATA_8:
    default:
    return UART_CHAR_SIZE_8_BITS;

  }
}

SercomParityMode DmaUart::extractParity(uint16_t config)
{
  switch(config & HARDSER_PARITY_MASK)
  {
    case HARDSER_PARITY_NONE:
    default:
    return SERCOM_NO_PARITY;

    case HARDSER_PARITY_EVEN:
    return SERCOM_EVEN_PARITY;

    case HARDSER_PARITY_ODD:
    return SERCOM_ODD_PARITY;
  }
}