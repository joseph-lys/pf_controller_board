/*
 * XSERCOM.h
 *
 * Created: 3/11/2019 9:47:54 PM
 *  Author: Joseph
 */ 


#ifndef DMA_SERCOM_H_
#define DMA_SERCOM_H_

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

class XSERCOM {
protected:
  Sercom* sercom_;
  const uint8_t sercom_id_;
  const IRQn_Type irqn_;
public:
  XSERCOM(Sercom* s);
  SERCOM SERCOM_;  // default implementation of Arduino sercom library.
  
  inline uint8_t getSercomId() {
    return sercom_id_;
  }
  
  inline IRQn_Type getIRQn() {
    return irqn_;
  }
  const uint32_t dataAddressI2CM;
  const uint32_t dataAddressI2CS;
  const uint32_t dataAddressUSART;
  const uint32_t dataAddressSPI;
  
  Sercom* getSercomPointer();
  void initSPISlave(SercomSpiTXSlavePad tx_pad, SercomSpiRXSlavePad rx_pad, SercomSpiCharSize charSize, SercomDataOrder dataOrder);
  void initSPISlaveClock(SercomSpiClockMode clockMode);
  void clearSpiInterruptFlags();
  void disableSpiInterrruptSSL();
  void enableSpiInterrruptSSL();
  // void writeNowaitSPI(uint32_t value);
  
  void enableSPI();
  void disableSPI();
  void resetSPI();
  
  void initUART(SercomUartMode mode,
                SercomUartSampleRate sampleRate,
                uint32_t baudrate=0);
  void initFrame(SercomUartCharSize charSize,
                 SercomDataOrder dataOrder,
                 SercomParityMode parityMode,
                 SercomNumberStopBit nbStopBits);
  void initPads(SercomUartTXPad txPad, SercomRXPad rxPad);
  void enableUART();
  
  void disableIRQ();
  void enableIRQ();
  void setPriorityIRQ(uint32_t priority);
  void disableUartInterrrupt();
  void enableUARTInterrruptTXC();
};




#endif /* XSERCOM_H_ */