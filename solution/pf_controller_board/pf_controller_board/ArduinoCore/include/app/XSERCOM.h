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
public:
  XSERCOM(Sercom* s);
  uint8_t getSercomId();
  Sercom* getSercomPointer();
  void initSPISlave(SercomSpiTXSlavePad tx_pad, SercomSpiRXSlavePad rx_pad, SercomSpiCharSize charSize, SercomDataOrder dataOrder);
  void initSPISlaveClock(SercomSpiClockMode clockMode);
  void clearSpiSslInterrupt();
};




#endif /* XSERCOM_H_ */