/*
 * XSERCOM.cpp
 *
 * Created: 3/11/2019 9:50:32 PM
 *  Author: Joseph
 */ 

#include "XSERCOM.h"
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
    assert(false && "Unknown IRQn");  
  }
}

XSERCOM::XSERCOM(Sercom* s) 
: SERCOM(s), sercom_id_(getSercomIdInternal(s)), irqn_(getIRQnInternal(s)),
dataAddressI2CM(reinterpret_cast<uint32_t>(&(s->I2CM.DATA.reg))),
dataAddressI2CS(reinterpret_cast<uint32_t>(&(s->I2CS.DATA.reg))),
dataAddressUSART(reinterpret_cast<uint32_t>(&(s->USART.DATA.reg))),
dataAddressSPI(reinterpret_cast<uint32_t>(&(s->SPI.DATA.reg)))
{
  
}


Sercom* XSERCOM::getSercomPointer() {
  return sercom;
}

void XSERCOM::initSPISlave(SercomSpiTXSlavePad tx_pad, SercomSpiRXSlavePad rx_pad, SercomSpiCharSize charSize, SercomDataOrder dataOrder)
{
  resetSPI();
  // initClockNVIC();
    uint8_t clockId = 0;
  IRQn_Type IdNvic=PendSV_IRQn ; // Dummy init to intercept potential error later

  if(sercom == SERCOM0)
  {
    clockId = GCM_SERCOM0_CORE;
    IdNvic = SERCOM0_IRQn;
  }
  else if(sercom == SERCOM1)
  {
    clockId = GCM_SERCOM1_CORE;
    IdNvic = SERCOM1_IRQn;
  }
  else if(sercom == SERCOM2)
  {
    clockId = GCM_SERCOM2_CORE;
    IdNvic = SERCOM2_IRQn;
  }
  else if(sercom == SERCOM3)
  {
    clockId = GCM_SERCOM3_CORE;
    IdNvic = SERCOM3_IRQn;
  }
  #if defined(SERCOM4)
  else if(sercom == SERCOM4)
  {
    clockId = GCM_SERCOM4_CORE;
    IdNvic = SERCOM4_IRQn;
  }
  #endif // SERCOM4
  #if defined(SERCOM5)
  else if(sercom == SERCOM5)
  {
    clockId = GCM_SERCOM5_CORE;
    IdNvic = SERCOM5_IRQn;
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
  sercom->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE_SPI_SLAVE |
    SERCOM_SPI_CTRLA_DOPO(tx_pad) |
    SERCOM_SPI_CTRLA_DIPO(rx_pad) |
    (dataOrder << SERCOM_SPI_CTRLA_DORD_Pos);

  //Setting the CTRLB register
  sercom->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(charSize) | 
    SERCOM_SPI_CTRLB_RXEN | // Active the SPI receiver.
    SERCOM_SPI_CTRLB_SSDE ;	// Enable Slave Select Low Interrupt
  sercom->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_MASK; // clear all interrupts
  clearSpiInterruptFlags();
  enableSpiInterrruptSSL();
  NVIC_EnableIRQ(IdNvic);
}

void XSERCOM::initSPISlaveClock(SercomSpiClockMode clockMode)
{
//   //Extract data from clockMode
//   int cpha, cpol;
// 
//   if((clockMode & (0x1ul)) == 0 )
//     cpha = 0;
//   else
//     cpha = 1;
// 
//   if((clockMode & (0x2ul)) == 0)
//     cpol = 0;
//   else
//     cpol = 1;
// 
//   //Setting the CTRLA register
//   sercom->SPI.CTRLA.reg |=	( cpha << SERCOM_SPI_CTRLA_CPHA_Pos ) |
//                             ( cpol << SERCOM_SPI_CTRLA_CPOL_Pos );
//   
//   //Synchronous arithmetic
//   sercom->SPI.BAUD.reg = 0;
  uint32_t mode;
  switch(clockMode) {
    case SERCOM_SPI_MODE_0:
      sercom->SPI.CTRLA.bit.CPOL = 0;
      sercom->SPI.CTRLA.bit.CPHA = 0;
      break;
    case SERCOM_SPI_MODE_1:
      sercom->SPI.CTRLA.bit.CPOL = 0;
      sercom->SPI.CTRLA.bit.CPHA = 1;
      break;
    case SERCOM_SPI_MODE_2:
      sercom->SPI.CTRLA.bit.CPOL = 1;
      sercom->SPI.CTRLA.bit.CPHA = 0;
      break;
    case SERCOM_SPI_MODE_3:
      sercom->SPI.CTRLA.bit.CPOL = 1;
      sercom->SPI.CTRLA.bit.CPHA = 1;
      break;
    default:
      while (1) { } // problem
      break;
  }
  sercom->SPI.BAUD.reg = 0;  //
}

void XSERCOM::clearSpiInterruptFlags() {
  sercom->SPI.INTFLAG.reg = sercom->SPI.INTFLAG.reg;
}

void XSERCOM::disableSpiInterrruptSSL() {
  sercom->SPI.INTENCLR.bit.SSL = 0;
}

void XSERCOM::enableSpiInterrruptSSL() {
  sercom->SPI.INTENSET.bit.SSL = 1;
}

void XSERCOM::writeNowaitSPI(uint32_t value) {
  sercom->SPI.DATA.reg = SERCOM_SPI_DATA_DATA(value);
}