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



void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(1000000);
  initAppComponents();
  delayMicroseconds(100);
}

uint8_t serial_test[] = "serial testload\n";
uint8_t dma_test[] = "dma testload\n";
void loop() {
  volatile int x;
  SerialUSB.println("Test");
  delay(1000);
}