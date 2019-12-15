/*
 * XSERCOM.h
 *
 * Created: 3/11/2019 9:47:54 PM
 *  Author: Joseph
 */ 


#ifndef XSERCOM_H_
#define XSERCOM_H_

#include "SERCOM.h"

typedef enum
{
	SPI_SLAVE_PAD_0_SCK_1_SS_2 = 0,
	SPI_SLAVE_PAD_2_SCK_3_SS_1,
	SPI_SLAVE_PAD_3_SCK_1_SS_2,
	SPI_SLAVE_PAD_0_SCK_3_SS_1
} SercomSpiTXSlavePad;

typedef enum
{
	SPI_SLAVE_RX_PAD_0 = 0,
	SPI_SLAVE_RX_PAD_1,
	SPI_SLAVE_RX_PAD_2,
	SPI_SLAVE_RX_PAD_3
} SercomSpiRXSlavePad;

class XSERCOM : public SERCOM {
private:
  const uint8_t sercom_id_;
  const IRQn_Type irqn_;
public:
  XSERCOM(Sercom* s);
  
  inline uint8_t getSercomId() {
    return sercom_id_;
  }
  
  inline IRQn_Type getIRQn() {
    return irqn_;
  }
  
  Sercom* getSercomPointer();
  void initSPISlave(SercomSpiTXSlavePad tx_pad, SercomSpiRXSlavePad rx_pad, SercomSpiCharSize charSize, SercomDataOrder dataOrder);
  void initSPISlaveClock(SercomSpiClockMode clockMode);
  inline void clearSpiSslInterrupt();
  
  inline void clearSpiInterruptFlags() {
    auto flags = sercom->SPI.INTFLAG.reg;
    sercom->SPI.INTFLAG.reg = flags;
  }

  inline void noWaitDisableSPI() {
    sercom->SPI.CTRLA.bit.ENABLE = 0;
  }
  inline void noWaitEnableSPI() {
    sercom->SPI.CTRLA.bit.ENABLE = 1;
  }
  inline void waitSyncSPI() {
    while(sercom->SPI.SYNCBUSY.reg) { }
  }
  
  inline void disableRxSPI() {
    sercom->SPI.CTRLB.bit.RXEN = 0;
  }
  
  inline void enableRxSPI() {
    sercom->SPI.CTRLB.bit.RXEN = 1;
  }
  
  inline void disableSpiInterrruptSSL() {
    sercom->SPI.INTENCLR.bit.SSL = 1;
  }
  
  inline void enableSpiInterrruptSSL() {
    sercom->SPI.INTENSET.bit.SSL = 1;
  }
};




#endif /* XSERCOM_H_ */