/*Begining of Auto generated code by Atmel studio */
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
uint8_t serial_test[] = "abcde";
uint8_t dma_test[] = "dmatestload\n";
void loop() {
  // put your main code here, to run repeatedly:
  volatile int x;
  delay(500);
  SerialUSB.println("USBTESTLOAD");
  delay(1);
  while(Serial1.available()) {
    x = Serial1.read();
    if(x > 0) {
      SerialUSB.write(static_cast<uint8_t>(x));
      delay(1);
    }
  }
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  Serial1.write(serial_test, sizeof(serial_test));
  Serial.write(dma_test, sizeof(dma_test));
  /// Debug configurations
  Pm* pm = PM;
  Sercom* sercom = SERCOM2;
  Dmac* dmac = DMAC;
  DmacDescriptor* first_rx = Dma::firstDesc(1);
  DmacDescriptor* working_rx = Dma::workingDesc(1);
  DmacDescriptor* first_tx = Dma::firstDesc(0);
  DmacDescriptor* working_tx = Dma::workingDesc(0);
  
  while(Serial.available()) {
    x = Serial.read();
    if(x >= 0) {
      SerialUSB.write(static_cast<uint8_t>(x));
      SerialUSB.println(x);
    } else {
      x++;
    }
  }
  delay(200);
  while(Serial.available()) {
    x = Serial.read();
    if(x >= 0) {
      SerialUSB.write(static_cast<uint8_t>(x));
      SerialUSB.println(x);
    } else {
      x++;
    }
  }
}
