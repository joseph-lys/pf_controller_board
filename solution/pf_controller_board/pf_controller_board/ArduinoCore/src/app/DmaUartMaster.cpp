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
#include "dmashared.h"

DmaUartMaster::DmaUartMaster(XSERCOM *_s, uint8_t _dma_channel, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX) 
: sercom(_s), tx_desc(*(Dma::firstDesc(_dma_channel))) {
    sercom = _s;
    dma_channel = _dma_channel;
    uc_pinRX = _pinRX;
    uc_pinTX = _pinTX;
    uc_padRX = _padRX ;
    uc_padTX = _padTX;
    timeout = 500000;
    last_transfer = micros();
    setupDescriptors();
    Dma::registerChannel(dma_channel, this);
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
    setupTxConfig();
    /* Set transfer size, source address and destination address */
    tx_desc.BTCNT.reg = _tx_len;
    tx_desc.SRCADDR.reg = reinterpret_cast<uint32_t>(_tx_buf) + static_cast<uint32_t>(_tx_len);
    tx_desc.DESCADDR.reg = 0;
    transferStart();
}
void DmaUartMaster::setupTxConfig() {
  Dmac* dmac = DMAC;
  
  // set configurations
  DMAC_CHCTRLB_Type ch_ctrl_b;
  ch_ctrl_b.reg = 0;
  ch_ctrl_b.bit.EVACT = DMAC_CHCTRLB_EVACT_NOACT_Val;
  ch_ctrl_b.bit.EVIE = 0;  // no input event
  ch_ctrl_b.bit.EVOE = 0;  // no output event
  ch_ctrl_b.bit.LVL = DMAC_CHCTRLB_LVL_LVL1_Val;  // reserve one level for SPI
  ch_ctrl_b.bit.TRIGSRC = Dma::getSercomTx(sercom->getSercomId());
  ch_ctrl_b.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_TRANSACTION_Val;  //
  
  __disable_irq();
  dmac->CHID.bit.ID = dma_channel;
  dmac->CHCTRLB.reg = ch_ctrl_b.reg;
  __enable_irq();
}

void DmaUartMaster::transferStart() {
  Dmac* dmac = DMAC;
  
  __disable_irq();
  // select DMA channel
  dmac->CHID.bit.ID = dma_channel;
  // enable interrupts
  dmac->CHINTENSET.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;
  // enable dma
  dmac->CHCTRLA.bit.ENABLE = 1;
  __enable_irq();
  
  // software trigger start (first byte in Tx)
  Dma::swTrigger(dma_channel);
}

void DmaUartMaster::setupDescriptors() {
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
  tx_desc.DSTADDR.reg = reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg));
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
  rx_desc.SRCADDR.reg = reinterpret_cast<uint32_t>(&(sercom->getSercomPointer()->USART.DATA.reg));
  rx_desc.DSTADDR.reg = 0;  // undetermined until transfer initiated 
  rx_desc.DESCADDR.reg = 0;  // undetermined until transfer initiated
}

void DmaUartMaster::begin(unsigned long baudrate, uint16_t config) {
  pinPeripheral(uc_pinRX, g_APinDescription[uc_pinRX].ulPinType);
  pinPeripheral(uc_pinTX, g_APinDescription[uc_pinTX].ulPinType);
  
  sercom->initUART(UART_INT_CLOCK, SAMPLE_RATE_x16, baudrate);
  sercom->initFrame(extractCharSize(config), LSB_FIRST, extractParity(config), extractNbStopBit(config));
  sercom->initPads(uc_padTX, uc_padRX);

  sercom->enableUART();
}

void DmaUartMaster::stopTransfer() {
  if(current_state == is_busy) {
    current_state = is_timeout;
  }
  Dmac* dmac = DMAC;
  dmac->CHID.bit.ID = dma_channel;
  
  // channel disable
  dmac->CHCTRLA.bit.ENABLE = 1;
  while(dmac->CHCTRLA.bit.ENABLE) {}  // wait for disable
  
  // sw reset
  dmac->CHCTRLA.bit.SWRST = 1;
  while(dmac->CHCTRLA.bit.SWRST) {}  // wait for reset
  
  // cleanup interrupts
  dmac->CHINTENCLR.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;
}

void DmaUartMaster::callback(int status) {
  switch(status) {
    case Callback::is_error:
      // todo
      break;
    case Callback::is_done:
      current_state = is_done;
      stopTransfer();
      break;
  }
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