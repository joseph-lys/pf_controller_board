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
#include <Arduino.h>
#include "wiring_private.h"
#include "dma_common.h"
#include "extended_sercom.h"

static constexpr unsigned long kUartBaudrate = 1000000ul;
static constexpr uint32_t duplicated_pin_ss = 12ul;

static ExtendedSercom xsercom_uart0{ SERCOM0 };
static ExtendedSercom xsercom_uart1{ SERCOM2 };

DmaUart DSerial0 {
  &xsercom_uart0,
  2, 4,  // RX: Dma Channel 2, TX: DmaChannel 6
  0, 1,  // RX: D0 (PA11 | PMUX-C),  TX: D1 (PA10 | PMUX-C) 
  SERCOM_RX_PAD_3, UART_TX_PAD_2   // PAD Settings
};

//Uart Serial1( &sercom0, PIN_SERIAL1_RX, PIN_SERIAL1_TX, PAD_SERIAL1_RX, PAD_SERIAL1_TX ) ;
DmaUart DSerial1 { 
  &xsercom_uart1,
  3, 5,  // RX: Dma Channel 3, TX: Dma Channel 7
  5, 4,  // RX: D5 (PA15 | PMUX-C), TX: D4 (PA08 | PMUX-C)
  SERCOM_RX_PAD_3, UART_TX_PAD_0  // PAD Settings
};


static ExtendedSercom xsercom_spi{SERCOM4};

DmaSpiSlave DSPI {
  &xsercom_spi, 
  0, 1,  // RX: Dma Channel 0, TX: Dma Channel 1
  PIN_SPI_MISO,  PIN_SPI_SCK,  PIN_SPI_MOSI, SS,  // MISO: MISO (), SCK: SCK (), MOSI: MOSI (), SS: A2()
  DmaSpiSlave::getTxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX),  
  DmaSpiSlave::getRxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX)
};




/// Interrupt Handlers

void SERCOM0_Handler (void) {
  
}

void SERCOM4_Handler (void) {
  DSPI.doBeforeSpiStarts();
}

void SpiEnd_Handler (void) {
  DSPI.doAfterSpiStops();
}


/// Application Specific initialization
void initAppComponents() {
  dma_common::init();
  
  DSerial0.begin(kUartBaudrate);
  DSerial1.begin(kUartBaudrate);
  pinPeripheral(0, PIO_SERCOM);
  pinPeripheral(1, PIO_SERCOM);
  pinPeripheral(4, PIO_SERCOM);  // why our sample does not match ???
  pinPeripheral(5, PIO_SERCOM);  // why our sample does not match ???
  
  // Duplicated SS signal to trigger an action when SPI transfer complete (LED_PIN in this case)
  pinMode(duplicated_pin_ss, INPUT);
  attachInterrupt(duplicated_pin_ss, &SpiEnd_Handler, RISING);

  DSPI.begin();
};