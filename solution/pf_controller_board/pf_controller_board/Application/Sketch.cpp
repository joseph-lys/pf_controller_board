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
  SerialUSB.begin(0);
  delay(500);
}

void loop() {
  volatile long int x, y;
  SerialUSB.println("Test");
//  auto handle = Motors.createFeedbackHandle();
//  x = micros();
//  handle.readAllMotors();
//  y = micros();
//  auto m0 = handle.getFeedback(0);
//  SerialUSB.println(m0.position);
//  auto m1 = handle.getFeedback(1);
//  SerialUSB.println(m1.position);
   Motors.pingMotor(1);
  delay(1000);
}