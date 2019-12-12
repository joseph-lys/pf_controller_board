/*
 * XSERCOM.cpp
 *
 * Created: 3/11/2019 9:50:32 PM
 *  Author: Joseph
 */ 

#include "XSERCOM.h"
#include "variant.h"

XSERCOM::XSERCOM(Sercom* s) 
: SERCOM(s) {
  
}

uint8_t XSERCOM::getSercomId() {
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
  NVIC_EnableIRQ(IdNvic);
  NVIC_SetPriority (IdNvic, 0);  /* set Priority 0, this interrupt needs to be handled ASAP*/
  
  GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_ID( clockId ) | // Sercom Clock Id
                      GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is source
                      GCLK_CLKCTRL_CLKEN ;
   while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY ) {}  // Wait for sync
  // connect sercom slow clock
  GCLK->CLKCTRL.reg =  GCLK_CLKCTRL_ID_SERCOMX_SLOW | // Generic Clock 13 (SERCOMx Slow)
                       GCLK_CLKCTRL_GEN_GCLK3 | // Generic Clock Generator 0 is source
                       GCLK_CLKCTRL_CLKEN ;
  while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY ) {}  // Wait for sync
  
  //Setting the CTRLA register
  sercom->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE_SPI_SLAVE |
    SERCOM_SPI_CTRLA_DOPO(tx_pad) |
    SERCOM_SPI_CTRLA_DIPO(rx_pad) |
    dataOrder << SERCOM_SPI_CTRLA_DORD_Pos;

  //Setting the CTRLB register
  sercom->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(charSize) | 
    SERCOM_SPI_CTRLB_RXEN | // Active the SPI receiver.
    SERCOM_SPI_CTRLB_SSDE ;	// Enable Slave Select Low Interrupt
  sercom->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_MASK; // clear all interrupts
  sercom->SPI.INTFLAG.reg = SERCOM_SPI_INTFLAG_MASK;
  sercom->SPI.INTENSET.reg = SERCOM_SPI_INTENSET_SSL;
  
}

void XSERCOM::initSPISlaveClock(SercomSpiClockMode clockMode)
{
  //Extract data from clockMode
  int cpha, cpol;

  if((clockMode & (0x1ul)) == 0 )
    cpha = 0;
  else
    cpha = 1;

  if((clockMode & (0x2ul)) == 0)
    cpol = 0;
  else
    cpol = 1;

  //Setting the CTRLA register
  sercom->SPI.CTRLA.reg |=	( cpha << SERCOM_SPI_CTRLA_CPHA_Pos ) |
                            ( cpol << SERCOM_SPI_CTRLA_CPOL_Pos );
  
  //Synchronous arithmetic
  sercom->SPI.BAUD.reg = 0;
}

void XSERCOM::clearSpiSslInterrupt() {
  if(sercom->SPI.INTFLAG.bit.SSL) {
    sercom->SPI.INTFLAG.bit.SSL = 1;  // writing a 1 clears the bit
  }
}

void XSERCOM::clearSpiInterrupts() {
  volatile uint32_t all_interrupts = sercom->SPI.INTFLAG.reg;
  sercom->SPI.INTFLAG.reg = all_interrupts;
}