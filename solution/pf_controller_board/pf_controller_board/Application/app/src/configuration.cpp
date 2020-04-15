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
static constexpr uint8_t kPinRxUart0 = 0u;
static constexpr uint8_t kPinTxUart0 = 1u;
static constexpr uint8_t kPinDirUart0 = 2u;
static constexpr uint8_t kPinRxUart1 = 5u;
static constexpr uint8_t kPinTxUart1 = 4u;
static constexpr uint8_t kPinDirUart1 = 10u;
static constexpr uint32_t kPinSSDuplicate = 12u;

static ExtendedSercom xsercom0{SERCOM0};
static ExtendedSercom xsercom2{SERCOM2};
static ExtendedSercom xsercom4{SERCOM4};


DmaUart dma_uart0 {
  &xsercom0,
  2, 6,  // RX: Dma Channel 2, TX: DmaChannel 6
  kPinRxUart0, kPinTxUart0,  // RX: D0 (PA11 | PMUX-C),  TX: D1 (PA10 | PMUX-C) 
  SERCOM_RX_PAD_3, UART_TX_PAD_2   // PAD Settings
};

DmaUart dma_uart1 { 
  &xsercom2,
  3, 7,  // RX: Dma Channel 3, TX: Dma Channel 7
  kPinRxUart1, kPinTxUart1,  // RX: D5 (PA15 | PMUX-C), TX: D4 (PA08 | PMUX-C)
  SERCOM_RX_PAD_3, UART_TX_PAD_0  // PAD Settings
};



static ImpHwDxl imp_hw_dxl0 {
  dma_uart0,
  kPinDirUart0, HIGH  /// DIR: D2 (PA),  TX direction when HIGH
};

static ImpHwDxl imp_hw_dxl1 {
  dma_uart1,
  kPinDirUart1, HIGH  /// DIR: D10 (PA),  TX direction when HIGH
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



void SERCOM0_Handler (void) {
  dma_uart0.errorHandler();
}
void SERCOM2_Handler (void) {
  dma_uart1.errorHandler();
}  


void SERCOM4_Handler (void) {
  DSPI.doBeforeSpiStarts();  
}

void SpiEnd_Handler (void) {
  DSPI.doAfterSpiStops();
}


/// Application Specific initialization
void initAppComponents() {
  
  // Pin configuration
  pinMode(kPinRxUart0, INPUT_PULLUP);
  pinPeripheral(kPinRxUart0, PIO_SERCOM);
  pinMode(kPinTxUart0, OUTPUT);
  pinPeripheral(kPinTxUart0, PIO_SERCOM);
  pinMode(kPinDirUart0, OUTPUT);
  
  pinMode(kPinRxUart1, INPUT_PULLUP);
  pinPeripheral(kPinRxUart1, PIO_SERCOM);
  pinMode(kPinTxUart1, OUTPUT);
  pinPeripheral(kPinTxUart1, PIO_SERCOM_ALT);
  pinMode(kPinDirUart1, OUTPUT);
  
  // Initialize DMA
  DmaCommon::init();

  // Initialize DMA SPI
  // Duplicated SS signal to trigger an action when SPI transfer complete (LED_PIN in this case)
  pinMode(kPinSSDuplicate, INPUT_PULLUP);
  attachInterrupt(kPinSSDuplicate, &SpiEnd_Handler, RISING);
  // DSPI.begin();

  // Initialize DMA UART
  dma_uart0.begin(kUartBaudrate);
  dma_uart1.begin(kUartBaudrate);
  
  // Initialized the motor interface
  Motors.addDriver(dxl0);
  Motors.addDriver(dxl1);
  Motors.init();
  
};