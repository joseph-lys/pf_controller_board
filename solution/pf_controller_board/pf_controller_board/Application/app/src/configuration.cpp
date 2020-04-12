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
#include "imp_hw_dxl.h"
#include "DxlProtocolV1.h"
#include "DxlDriver.h"


static constexpr unsigned long kUartBaudrate = 1000000ul;
static constexpr uint32_t duplicated_pin_ss = 12ul;

static ExtendedSercom xsercom0{SERCOM0};
static ExtendedSercom xsercom2{SERCOM2};
static ExtendedSercom xsercom4{SERCOM4};

static DmaUart dma_uart0 {
  &xsercom0,
  2, 4,  // RX: Dma Channel 2, TX: DmaChannel 6
  0, 1,  // RX: D0 (PA11 | PMUX-C),  TX: D1 (PA10 | PMUX-C) 
  SERCOM_RX_PAD_3, UART_TX_PAD_2   // PAD Settings
};

static DmaUart dma_uart1 { 
  &xsercom2,
  3, 5,  // RX: Dma Channel 3, TX: Dma Channel 7
  5, 4,  // RX: D5 (PA15 | PMUX-C), TX: D4 (PA08 | PMUX-C)
  SERCOM_RX_PAD_3, UART_TX_PAD_0  // PAD Settings
};


static ImpHwDxl imp_hw_dxl0 {
  dma_uart0,
  2, HIGH  /// DIR: D2 (PA),  TX direction when HIGH
};

static ImpHwDxl imp_hw_dxl1 {
  dma_uart1,
  10, HIGH  /// DIR: D10 (PA),  TX direction when HIGH
};

/// Protocol V1, 80 byte write buffer, 16 byte read buffer
static DxlDriver dxl0 = DxlDriver::create<DxlProtocolV1, 80, 16>(imp_hw_dxl0);

/// Protocol V1, 80 byte write buffer, 16 byte read buffer
static DxlDriver dxl1 = DxlDriver::create<DxlProtocolV1, 80, 16>(imp_hw_dxl1);

MotorHandleFactory Motors{};



DmaSpiSlave DSPI {
  &xsercom4, 
  0, 1,  // RX: Dma Channel 0, TX: Dma Channel 1
  PIN_SPI_MISO,  PIN_SPI_SCK,  PIN_SPI_MOSI, SS,  // MISO: MISO (), SCK: SCK (), MOSI: MOSI (), SS: A2()
  DmaSpiSlave::getTxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX),  
  DmaSpiSlave::getRxPadFromMasterPad(PAD_SPI_TX, PAD_SPI_RX)
};

void Sercom4_Handler() {
  DSPI.doBeforeSpiStarts();  
}

static void SpiEnd_Handler (void) {
  DSPI.doAfterSpiStops();
}


/// Application Specific initialization
void initAppComponents() {
  // Initialize DMA
  dma_common::init();

  // Initialize DMA SPI
  // Duplicated SS signal to trigger an action when SPI transfer complete (LED_PIN in this case)
  pinMode(duplicated_pin_ss, INPUT);
  attachInterrupt(duplicated_pin_ss, &SpiEnd_Handler, RISING);
  DSPI.begin();

  // Initialize DMA UART
  dma_uart0.begin(kUartBaudrate);
  dma_uart1.begin(kUartBaudrate);

  pinPeripheral(0, PIO_SERCOM);
  pinPeripheral(1, PIO_SERCOM);
  pinPeripheral(4, PIO_SERCOM);  // why our sample does not match ???
  pinPeripheral(5, PIO_SERCOM);  // why our sample does not match ???
  
  // Initialized the motor interface
  Motors.addDriver(dxl0);
  Motors.addDriver(dxl1);
  Motors.init();
};