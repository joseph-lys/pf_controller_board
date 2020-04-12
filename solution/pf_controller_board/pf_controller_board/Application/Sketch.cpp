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
  initAppComponents();
  SerialUSB.begin(1000000);
  delayMicroseconds(100);
}

uint8_t serial_test[] = "serial testload\n";
uint8_t dma_test[] = "dma testload\n";
void loop() {
  volatile int x;
  SerialUSB.println("Test");
  auto handle = Motors.createFeedbackHandle();
  handle.readAllMotors();
  auto m0 = handle.getFeedback(0);
  SerialUSB.println(m0.position);
  delay(1000);
}