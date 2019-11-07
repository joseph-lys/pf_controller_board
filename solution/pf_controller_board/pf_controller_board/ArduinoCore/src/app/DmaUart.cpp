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
: sercom(_s), ch_desc(Dma::firstDesc(_dma_channel)) {
    sercom = _s;
    dma_channel = _dma_channel;
    uc_pinRX = _pinRX;
    uc_pinTX = _pinTX;
    uc_padRX = _padRX ;
    uc_padTX = _padTX;
    timeout = 500000;
    last_transfer = micros();
    Dma::registerChannel(dma_channel, this);
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
  if (current_state == is_busy) {
    if(micros() - last_transfer > timeout) {
      current_state = is_timeout;
    }
  }
  return current_state;
}

void DmaUart::write(uint8_t* _tx_buf, uint32_t _tx_len) {
  stopTransfer();
  setupTxDesc(_tx_buf, _tx_len);
  setupTxConfig();
  transferStart();
}
void DmaUart::setupTxConfig() {
  Dmac* dmac = DMAC;
  
  // set configurations
  DMAC_CHCTRLB_Type ch_ctrl_b;
  ch_ctrl_b.reg = 0;
  ch_ctrl_b.bit.EVACT = DMAC_CHCTRLB_EVACT_NOACT_Val;
  ch_ctrl_b.bit.EVIE = 0;  // no input event
  ch_ctrl_b.bit.EVOE = 0;  // no output event
  ch_ctrl_b.bit.LVL = DMAC_CHCTRLB_LVL_LVL1_Val;  // reserve one level for SPI
  ch_ctrl_b.bit.TRIGSRC = Dma::getSercomTx(sercom->getSercomId());
  ch_ctrl_b.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;  //
  
  __disable_irq();
  dmac->CHID.bit.ID = dma_channel;
  dmac->CHCTRLB.reg = ch_ctrl_b.reg;
  dmac->CHINTENSET.bit.TCMPL = 1;
  dmac->CHINTENSET.bit.TERR = 1;
  dmac->CHINTENCLR.bit.SUSP = 1;
  __enable_irq();
  
}

void DmaUart::transferStart() {
  Dmac* dmac = DMAC;
  __disable_irq();
  // select DMA channel
  dmac->CHID.bit.ID = dma_channel;
  // enable interrupts
  dmac->CHINTENSET.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;
  // enable dma
  dmac->CHCTRLA.bit.ENABLE = 1;
  __enable_irq();
}

void DmaUart::setupTxDesc(uint8_t* buf, uint32_t len) {
  // initialize descriptor data
  ch_desc->BTCNT.reg = 0;
  ch_desc->BTCTRL.reg = 0;
  // Set descriptor for TX
  DMAC_BTCTRL_Type btctrl;
  btctrl.reg = 0;
  btctrl.bit.VALID = 1;
  btctrl.bit.EVOSEL = DMAC_BTCTRL_EVOSEL_DISABLE_Val;
  btctrl.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
  btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  btctrl.bit.SRCINC = 1;
  btctrl.bit.DSTINC = 0;  // writes to fixed sercom->DATA address
  btctrl.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
  btctrl.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;
  ch_desc->BTCTRL.reg = btctrl.reg;
  /* Set transfer size, source address and destination address */
  ch_desc->BTCNT.reg = len;
  ch_desc->SRCADDR.reg = reinterpret_cast<uint32_t>(buf) + static_cast<uint32_t>(len);
  ch_desc->DSTADDR.reg = reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg));
  ch_desc->DESCADDR.reg = 0;
}

void DmaUart::setupRxDesc(uint8_t* buf, uint32_t len) {
  // initialize descriptor data
  ch_desc->BTCNT.reg = 0;
  ch_desc->BTCTRL.reg = 0;
  // Set descriptor for TX
  DMAC_BTCTRL_Type btctrl;
  btctrl.reg = 0;
  btctrl.bit.VALID = 1;
  btctrl.bit.EVOSEL = DMAC_BTCTRL_EVOSEL_DISABLE_Val;
  btctrl.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
  btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  btctrl.bit.SRCINC = 0;
  btctrl.bit.DSTINC = 1;  // writes to fixed sercom->DATA address
  btctrl.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;
  btctrl.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;
  ch_desc->BTCTRL.reg = btctrl.reg;
  /* Set transfer size, source address and destination address */
  ch_desc->BTCNT.reg = len;
  ch_desc->SRCADDR.reg = reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg));
  ch_desc->DSTADDR.reg = reinterpret_cast<uint32_t>(buf) + static_cast<uint32_t>(len);
  ch_desc->DESCADDR.reg = 0;
}

void DmaUart::stopTransfer() {
  if(current_state == is_busy) {
    current_state = is_timeout;
  }
  Dmac* dmac = DMAC;
  
  __disable_irq();
  dmac->CHID.bit.ID = dma_channel;
  // channel disable
  dmac->CHCTRLA.bit.ENABLE = 0;
  while(0 != dmac->CHCTRLA.bit.ENABLE) {}  // wait for disable
  // sw reset
  dmac->CHCTRLA.bit.SWRST = 1;
  while(0 != dmac->CHCTRLA.bit.SWRST) {}  // wait for reset
  __enable_irq();
}

void DmaUart::callback(int status) {
  DmacDescriptor* working = Dma::workingDesc(dma_channel);
  volatile uint32_t cnt = working->BTCNT.reg;
  stopTransfer();
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