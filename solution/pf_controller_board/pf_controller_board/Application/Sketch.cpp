﻿/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include "DmaCommon.h"
#include "wiring_private.h"
/*End of auto generated code by Atmel studio */


//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio

#define LED_PIN 13
void setup() {
  // put your setup code here, to run once:
  Dma::init();
  /// Additional configuration for SERCOM 2 as UART
  SerialUSB.begin(1000000);
  Serial1.begin(9600);
  Serial.begin(9600);
  //RX: D3 (PA09), TX: D4 (PA08)
  pinPeripheral(3, PIO_SERCOM_ALT);
  pinPeripheral(4, PIO_SERCOM_ALT);
  pinMode(LED_PIN, OUTPUT);
  delay(3000);
}

uint8_t dma_test[] = "DMA\n";
void loop() {
  // put your main code here, to run repeatedly:
  int x;
  
  SerialUSB.println("USB_TEST_LOAD");
  while(Serial1.available()) {
    x = Serial1.read();
    if(x > 0) {
      SerialUSB.write(static_cast<uint8_t>(x));
    }
  }
  while(Serial.available()) {
    x = Serial.read();
    if(x > 0) {
      SerialUSB.write(static_cast<uint8_t>(x));
    }
  }
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  Serial1.println("SERIAL1_TEST_LOAD\n");
  Serial.write(dma_test, sizeof(dma_test));
  /// Debug configurations
  Pm* pm = PM;
  Sercom* sercom = SERCOM2;
  Dmac* dmac = DMAC;
  DmacDescriptor* first_rx = Dma::firstDesc(0);
  DmacDescriptor* working_rx = Dma::workingDesc(0);
  DmacDescriptor* first_tx = Dma::firstDesc(1);
  DmacDescriptor* working_tx = Dma::workingDesc(1);
  
  
  delay(1000);
}
