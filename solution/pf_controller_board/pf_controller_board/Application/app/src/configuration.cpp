/// configuration.cpp
///
/// Copyright (c) 2020 Joseph Lee Yuan Sheng
///
/// This file is part of pf_controller_board which is released under MIT license.
/// See LICENSE file or go to https://github.com/joseph-lys/pf_controller_board for full license details.
///
/// Application assets
///

#include "configuration.h"
#include "extended_sercom.h"



static ExtendedSercom xsercom_uart{ SERCOM2 };
DmaUart DSerial{ 
  &xsercom_uart,
  4, 5,  // RX: Dma Channel 4, TX: Dma Channel 5
  3, 4,  // RX: D3 (PA09), TX: D4 (PA08)
  SERCOM_RX_PAD_1, UART_TX_PAD_0  // PAD Settings
};


static ExtendedSercom SercomSPI{SERCOM4};
DmaSpiSlave DSPI {
  &SercomSPI, 
  0, 1,  // RX: Dma Channel 0, TX: Dma Channel 1
  PIN_SPI_MISO,  PIN_SPI_SCK,  PIN_SPI_MOSI, SS,  // MISO: MISO (), SCK: SCK (), MOSI: MOSI (), SS: A2()
  DmaSpiSlave::getTxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX),  
  DmaSpiSlave::getRxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX)
};