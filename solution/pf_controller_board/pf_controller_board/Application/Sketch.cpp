/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include "wiring_private.h"
#include "dma_common.h"
// #include "dma_spi.h"
#include "configuration.h"
/*End of auto generated code by Atmel studio */

//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio


#define LED_PIN 13

void SERCOM4_Handler (void) {
  DSPI.startTransactionInterrupt();
}

void SpiEnd_Handler (void) {
  DSPI.endTransactionInterrupt();
}

void setup() {
  // put your setup code here, to run once:
  Dma::init();
  /// Additional configuration for SERCOM 2 as UART
  SerialUSB.begin(1000000);
  Serial1.begin(9600);  // Start Serial1 (Arduino UART) at 9600 Baud Rate
  DSerial.begin(9600);  // Start Serial (DMA UART) at 9600 Baud Rate
  
  // Serial (DMA UART) RX: D3 (PA09), TX: D4 (PA08), pinmux is configured here as it is not correct in the variant.cpp file
  pinPeripheral(3, PIO_SERCOM_ALT);
  pinPeripheral(4, PIO_SERCOM_ALT);
  
  // SPI 
  // Duplicated SS signal to trigger an action when SPI transfer complete (LED_PIN in this case)
  pinMode(LED_PIN, INPUT);
  attachInterrupt(LED_PIN, &SpiEnd_Handler, RISING);
  DSPI.begin();
  
  delay(500);
}

uint8_t serial_test[] = "serial testload\n";
uint8_t dma_test[] = "dma testload\n";
void loop() {
  volatile int x;
  Serial1.write(serial_test, sizeof(serial_test));
  DSerial.write(dma_test, sizeof(dma_test));
  delay(1000);
  
  if(DSerial.available()) {
    SerialUSB.println("\n\nSerial (DMA UART) received:");
    while(DSerial.available()) {
      x = DSerial.read();
      if(x >= 0) {
        SerialUSB.write(static_cast<char>(x & 0xff));
      }
    }
  }  
  
  if (Serial1.available()) {
    SerialUSB.println("\n\nSerial1 (Arduino UART) received:");
    while(Serial1.available()) {
      x = Serial1.read();
      if(x >= 0) {
        SerialUSB.write(static_cast<char>(x & 0xff));
      }
    }
  }    
  
  uint8_t* pSpiRx = DSPI.getRxDataPtr();
  if (pSpiRx) {
    SerialUSB.println("\n\SPI received:"); 
    for (int i=0; i<DSPI.buffer_size; i++) {
      SerialUSB.write(pSpiRx[i]);
    }
  }  
}