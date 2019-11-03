/*
 * DmaUartMaster.cpp
 *
 * Created: 3/11/2019 3:32:56 PM
 *  Author: Joseph
 */ 

#include "Arduino.h"
#include "dmac.h"
#include "DmaUartMaster.h"
#include "SERCOM.h"
#include "HardwareSerial.h"
#include "wiring_private.h"

DmaUartMaster::DmaUartMaster(Sercom *_s, uint32_t _dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX) 
: arduino_sercom(_s) {
    sercom = _s;
    dma_channel = _dma_channel;
    uc_pinRX = _pinRX;
    uc_pinTX = _pinTX;
    uc_padRX = _padRX ;
    uc_padTX = _padTX;
    timeout = 500000;
    last_transfer = micros();
    setup_descriptors();
}

void DmaUartMaster::begin(unsigned long baudrate)
{
  begin(baudrate, SERIAL_8N1);
}

int DmaUartMaster::poll() {
  if (current_state == is_busy) {
    if(micros() - last_transfer > timeout) {
      current_state = is_timeout;
    }
  }
  return current_state;
}

void DmaUartMaster::transfer(uint8_t* _tx_buf, uint32_t _tx_len, unsigned long int timeout) {
  
  __disable_irq();
  // select DMA channel
  DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
  
  // reset DMA channel
  DMAC->CHCTRLA.bit.ENABLE = 0;
  DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
  DMAC->SWTRIGCTRL.reg = ~(uint32_t{1} << dma_channel);
  
  // set configurations 
  DMAC_CHCTRLB_Type ch_ctrl_b;
  ch_ctrl_b.reg = 0;
  
  
  
  
  DMAC->CHCTRLB.reg = ch_ctrl_b.reg;
  
  /* Set transfer size, source address and destination address */
  tx_desc.BTCNT.reg = _tx_len;
  tx_desc.SRCADDR.reg = reinterpret_cast<uint32_t>(_tx_buf);
  tx_desc.DESCADDR.reg = 0;
  
  // enable dma
  DMAC->CHCTRLA.bit.ENABLE = 1;
  __enable_irq();
  
  // enable interrupt
  
}

void DmaUartMaster::setup_descriptors() {
  // initialize descriptor data
  tx_desc.BTCNT.reg = 0;
  tx_desc.BTCTRL.reg = 0;
  rx_desc.BTCNT.reg = 0;
  rx_desc.BTCTRL.reg = 0;
  
  // Set descriptor for TX
  tx_desc.BTCTRL.bit.VALID = 1;
  tx_desc.BTCTRL.bit.EVOSEL = DMAC_BTCTRL_EVOSEL_DISABLE_Val;  
  tx_desc.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  tx_desc.BTCTRL.bit.SRCINC = 1;
  tx_desc.BTCTRL.bit.DSTINC = 0;  // writes to fixed sercom->DATA address
  tx_desc.BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
  tx_desc.BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;
  tx_desc.BTCNT.reg = 0;  // undetermined until transfer initiated
  tx_desc.SRCADDR.reg = 0;  // undetermined until transfer initiated
  tx_desc.DSTADDR.reg = reinterpret_cast<uint32_t>(&(sercom->USART.DATA.reg));
  tx_desc.DESCADDR.reg = 0;  // undetermined until transfer initiated

  // Set descriptor for RX
  rx_desc.BTCTRL.bit.VALID = 1;
  rx_desc.BTCTRL.bit.EVOSEL = DMAC_BTCTRL_EVOSEL_DISABLE_Val;
  rx_desc.BTCTRL.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  rx_desc.BTCTRL.bit.SRCINC = 1;
  rx_desc.BTCTRL.bit.DSTINC = 0;  // writes to fixed sercom->DATA address
  rx_desc.BTCTRL.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
  rx_desc.BTCTRL.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;
  rx_desc.BTCNT.reg = 0;  // undetermined until transfer initiated
  rx_desc.SRCADDR.reg = reinterpret_cast<uint32_t>(&(sercom->USART.DATA.reg));
  rx_desc.DSTADDR.reg = 0;  // undetermined until transfer initiated 
  rx_desc.DESCADDR.reg = 0;  // undetermined until transfer initiated
}

void DmaUartMaster::begin(unsigned long baudrate, uint16_t config) {
  pinPeripheral(uc_pinRX, g_APinDescription[uc_pinRX].ulPinType);
  pinPeripheral(uc_pinTX, g_APinDescription[uc_pinTX].ulPinType);
  
  arduino_sercom.initUART(UART_INT_CLOCK, SAMPLE_RATE_x16, baudrate);
  arduino_sercom.initFrame(extractCharSize(config), LSB_FIRST, extractParity(config), extractNbStopBit(config));
  arduino_sercom.initPads(uc_padTX, uc_padRX);

  arduino_sercom.enableUART();
}


SercomNumberStopBit DmaUartMaster::extractNbStopBit(uint16_t config) {
  switch(config & HARDSER_STOP_BIT_MASK)
  {
    case HARDSER_STOP_BIT_1:
    default:
    return SERCOM_STOP_BIT_1;

    case HARDSER_STOP_BIT_2:
    return SERCOM_STOP_BITS_2;
  }
}

SercomUartCharSize DmaUartMaster::extractCharSize(uint16_t config)
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

SercomParityMode DmaUartMaster::extractParity(uint16_t config)
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