/*
 * XSERCOM.cpp
 *
 * Created: 3/11/2019 9:50:32 PM
 *  Author: Joseph
 */ 

#include "dma_sercom.h"
#include "variant.h"
#include "assert.h"

static uint8_t getSercomIdInternal(Sercom* sercom) {
  uint8_t x = 0;
  if(sercom == SERCOM0) { 
    x = 0; 
  } else if(sercom == SERCOM1) {
    x = 1;
  } else if(sercom == SERCOM2) {
    x = 2;
  } else if(sercom == SERCOM3) {
    x = 3;
  } else if(sercom == SERCOM4) {
    x = 4; 
  } else if(sercom == SERCOM5) {
    x = 5;
  }
  return x;
}

static IRQn_Type getIRQnInternal(Sercom* sercom) {
  if(sercom == SERCOM0) { 
    return SERCOM0_IRQn;
  } else if(sercom == SERCOM1) {
    return SERCOM1_IRQn;
  } else if(sercom == SERCOM2) {
    return SERCOM2_IRQn;
  } else if(sercom == SERCOM3) {
    return SERCOM3_IRQn;
  } else if(sercom == SERCOM4) {
    return SERCOM4_IRQn;
  } else if(sercom == SERCOM5) {
    return SERCOM5_IRQn;
  } else {
    return PendSV_IRQn;  
  }
}

XSERCOM::XSERCOM(Sercom* s) 
: SERCOM_(s), sercom_(s),
sercom_id_(getSercomIdInternal(s)), 
irqn_(getIRQnInternal(s)),
dataAddressI2CM(reinterpret_cast<uint32_t>(&(s->I2CM.DATA.reg))),
dataAddressI2CS(reinterpret_cast<uint32_t>(&(s->I2CS.DATA.reg))),
dataAddressUSART(reinterpret_cast<uint32_t>(&(s->USART.DATA.reg))),
dataAddressSPI(reinterpret_cast<uint32_t>(&(s->SPI.DATA.reg)))
{
  
}


Sercom* XSERCOM::getSercomPointer() {
  return sercom_;
}

void XSERCOM::initSPISlave(SercomSpiTXSlavePad tx_pad, SercomSpiRXSlavePad rx_pad, SercomSpiCharSize charSize, SercomDataOrder dataOrder)
{
  resetSPI();
  // initClockNVIC();
    uint8_t clockId = 0;
  IRQn_Type IdNvic= irqn_ ; // Dummy init to intercept potential error later

  if(sercom_ == SERCOM0)
  {
    clockId = GCM_SERCOM0_CORE;
  }
  else if(sercom_ == SERCOM1)
  {
    clockId = GCM_SERCOM1_CORE;
  }
  else if(sercom_ == SERCOM2)
  {
    clockId = GCM_SERCOM2_CORE;
  }
  else if(sercom_ == SERCOM3)
  {
    clockId = GCM_SERCOM3_CORE;
  }
  #if defined(SERCOM4)
  else if(sercom_ == SERCOM4)
  {
    clockId = GCM_SERCOM4_CORE;
  }
  #endif // SERCOM4
  #if defined(SERCOM5)
  else if(sercom_ == SERCOM5)
  {
    clockId = GCM_SERCOM5_CORE;
  }
  #endif // SERCOM5
  if ( IdNvic == PendSV_IRQn )
  {
    // We got a problem here
    return ;
  }

  // Setting NVIC
  NVIC_DisableIRQ(IdNvic);
  NVIC_SetPriority(IdNvic, 0);  /* set Priority 0 */
  
  GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_ID( clockId ) | // Sercom Clock Id
                      GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is source
                      GCLK_CLKCTRL_CLKEN ;
   while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY ) {}  // Wait for sync
  // connect sercom slow clock
  GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_ID_SERCOMX_SLOW | // Generic Clock 13 (SERCOMx Slow)
                       GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is source
                       GCLK_CLKCTRL_CLKEN ;
  while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY ) {}  // Wait for sync
  
  //Setting the CTRLA register
  sercom_->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE_SPI_SLAVE |
    SERCOM_SPI_CTRLA_DOPO(tx_pad) |
    SERCOM_SPI_CTRLA_DIPO(rx_pad) |
    (dataOrder << SERCOM_SPI_CTRLA_DORD_Pos);

  //Setting the CTRLB register
  sercom_->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(charSize) | 
    SERCOM_SPI_CTRLB_RXEN | // Active the SPI receiver.
    SERCOM_SPI_CTRLB_SSDE ;	// Enable Slave Select Low Interrupt
  sercom_->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_MASK; // clear all interrupts
  clearSpiInterruptFlags();
  enableSpiInterrruptSSL();
  NVIC_EnableIRQ(IdNvic);
}

void XSERCOM::initSPISlaveClock(SercomSpiClockMode clockMode) {
  uint32_t mode;
  switch(clockMode) {
    case SERCOM_SPI_MODE_0:
      sercom_->SPI.CTRLA.bit.CPOL = 0;
      sercom_->SPI.CTRLA.bit.CPHA = 0;
      break;
    case SERCOM_SPI_MODE_1:
      sercom_->SPI.CTRLA.bit.CPOL = 0;
      sercom_->SPI.CTRLA.bit.CPHA = 1;
      break;
    case SERCOM_SPI_MODE_2:
      sercom_->SPI.CTRLA.bit.CPOL = 1;
      sercom_->SPI.CTRLA.bit.CPHA = 0;
      break;
    case SERCOM_SPI_MODE_3:
      sercom_->SPI.CTRLA.bit.CPOL = 1;
      sercom_->SPI.CTRLA.bit.CPHA = 1;
      break;
    default:
      while (1) { } // problem
      break;
  }
  sercom_->SPI.BAUD.reg = 0;  //
}

void XSERCOM::clearSpiInterruptFlags() {
  sercom_->SPI.INTFLAG.reg = sercom_->SPI.INTFLAG.reg;
}

void XSERCOM::disableSpiInterrruptSSL() {
  sercom_->SPI.INTENCLR.bit.SSL = 0;
}

void XSERCOM::enableSpiInterrruptSSL() {
  sercom_->SPI.INTENSET.bit.SSL = 1;
}

// void XSERCOM::writeNowaitSPI(uint32_t value) {
//   sercom_->SPI.DATA.reg = SERCOM_SPI_DATA_DATA(value);
// }

void XSERCOM::enableSPI() {
  SERCOM_.enableSPI();
}

void XSERCOM::disableSPI() {
  SERCOM_.disableSPI();
}

void XSERCOM::resetSPI() {
  SERCOM_.resetSPI();
}

void XSERCOM::initUART(SercomUartMode mode, 
                       SercomUartSampleRate sampleRate, 
                       uint32_t baudrate) {
  SERCOM_.initUART(mode, sampleRate, baudrate);
}

void XSERCOM::initFrame(SercomUartCharSize charSize, 
                        SercomDataOrder dataOrder, 
                        SercomParityMode parityMode, 
                        SercomNumberStopBit nbStopBits) {
  SERCOM_.initFrame(charSize, dataOrder, parityMode, nbStopBits);
}

void XSERCOM::initPads(SercomUartTXPad txPad, SercomRXPad rxPad) {
  SERCOM_.initPads(txPad, rxPad);
}

void XSERCOM::enableUART() {
  SERCOM_.enableUART();
}

void XSERCOM::disableIRQ() {
  NVIC_DisableIRQ(irqn_);
}

void XSERCOM::enableIRQ() {
  NVIC_EnableIRQ(irqn_);
}

void XSERCOM::setPriorityIRQ(uint32_t priority) {
  NVIC_SetPriority(irqn_, priority);
}

void XSERCOM::disableUartInterrrupt() {
  sercom_->USART.INTENCLR.reg = sercom_->USART.INTENSET.reg;
  
  // SERCOM2->USART.INTENCLR.bit.CTSIC = 1;
  // SERCOM2->USART.INTENCLR.bit.DRE = 1;
  // SERCOM2->USART.INTENCLR.bit.RXBRK = 1;
  // SERCOM2->USART.INTENCLR.bit.RXC = 1;
  // SERCOM2->USART.INTENCLR.bit.RXS = 1;
}

void XSERCOM::enableUARTInterrruptTXC() {
  sercom_->USART.INTENSET.bit.TXC = 1;
}