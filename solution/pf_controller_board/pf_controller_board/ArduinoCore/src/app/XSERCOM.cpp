/*
 * XSERCOM.cpp
 *
 * Created: 3/11/2019 9:50:32 PM
 *  Author: Joseph
 */ 

#include "XSERCOM.h"

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
  initClockNVIC();

  //Setting the CTRLA register
  sercom->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE_SPI_SLAVE |
  SERCOM_SPI_CTRLA_DOPO(tx_pad) |
  SERCOM_SPI_CTRLA_DIPO(rx_pad) |
  dataOrder << SERCOM_SPI_CTRLA_DORD_Pos;

  //Setting the CTRLB register
  sercom->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(charSize) | SERCOM_SPI_CTRLB_RXEN;	//Active the SPI receiver.
}

void XSERCOM::initSPISlaveClock(SercomSpiClockMode clockMode, uint32_t baudrate)
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