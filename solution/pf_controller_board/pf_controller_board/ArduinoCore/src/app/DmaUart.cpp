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

DmaUart::DmaUart(XSERCOM *_s, uint8_t _dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX) 
: sercom(_s), dma_(_dma_channel, this) {
    sercom = _s;
    dma_channel = _dma_channel;
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
  NVIC_DisableIRQ(SERCOM2_IRQn);
  // clear some interrupts
  SERCOM2->USART.INTENCLR.bit.CTSIC = 1;
  SERCOM2->USART.INTENCLR.bit.DRE = 1;
  SERCOM2->USART.INTENCLR.bit.RXBRK = 1;
  SERCOM2->USART.INTENCLR.bit.RXC = 1;
  SERCOM2->USART.INTENCLR.bit.RXS = 1;
  // SERCOM2->USART.INTENCLR.bit.TXC = 1;
  SERCOM2->USART.INTENSET.bit.TXC = 1;
  sercom->enableUART();
}

int DmaUart::poll() {
  return current_state;
}

void DmaUart::write(uint8_t* _tx_buf, uint32_t _tx_len) {
  dma_.stop();
  dma_.setupTxDesc(reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg)), _tx_buf, _tx_len);
  dma_.setupTxConfig(sercom->getSercomId());
  dma_.start();
}

void DmaUart::read(uint8_t* _rx_buf, uint32_t _rx_len) {
  dma_.stop();
  dma_.setupRxDesc(reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg)), _rx_buf, _rx_len);
  dma_.setupRxConfig(sercom->getSercomId());
  dma_.start();
}

void DmaUart::callback(int status) {
  DmacDescriptor* working = Dma::workingDesc(dma_channel);
  volatile uint32_t cnt = working->BTCNT.reg;
  dma_.stop();
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