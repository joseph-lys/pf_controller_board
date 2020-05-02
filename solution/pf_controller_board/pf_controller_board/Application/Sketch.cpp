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
  delay(1000);
  initAppComponents();
  SerialUSB.begin(0);
}

void loop() {
  volatile long int x, y;
  SerialUSB.println("Test");
  auto handle = Motors.createFeedbackHandle();
  handle.readAllMotors();
  auto m1 = handle.getFeedback(1);
  SerialUSB.println(m1.position);
  Motors.pingMotor(1);
  // Motors.pingMotor(2);
  delay(1000);
}