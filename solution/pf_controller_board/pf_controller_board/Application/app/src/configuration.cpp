/*
 * configuration.cpp
 *
 * Created: 5/4/2020 11:52:02 AM
 *  Author: josep
 */ 

#include "configuration.h"
#include "dma_sercom.h"



static DmaSercom dma_sercom_uart{ SERCOM2 };
DmaUart DSerial{ 
  &dma_sercom_uart,
  4, 5,  // RX: Dma Channel 4, TX: Dma Channel 5
  3, 4,  // RX: D3 (PA09), TX: D4 (PA08)
  SERCOM_RX_PAD_1, UART_TX_PAD_0  // PAD Settings
};


static DmaSercom SercomSPI{SERCOM4};
DmaSpiSlave DSPI {
  &SercomSPI, 
  0, 1,  // RX: Dma Channel 0, TX: Dma Channel 1
  PIN_SPI_MISO,  PIN_SPI_SCK,  PIN_SPI_MOSI, SS,  // MISO: MISO (), SCK: SCK (), MOSI: MOSI (), SS: A2()
  DmaSpiSlave::getTxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX),  
  DmaSpiSlave::getRxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX)
};