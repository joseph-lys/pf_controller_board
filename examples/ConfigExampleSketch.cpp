/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include "wiring_private.h"
#include "dma_common.h"
// #include "dma_spi.h"
#include "configuration.h"
#include "DxlProtocolV1.h"
/*End of auto generated code by Atmel studio */

//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio

#define LED_PIN 13


static constexpr uint8_t motor_id = 19;
static constexpr uint8_t broadcast_id = 0xfe;
static constexpr uint8_t reg_return_delay_time = 5;
static constexpr uint8_t reg_led = 25;
static constexpr uint8_t reg_id = 3;

void setup() {
  // put your setup code here, to run once:
  delay(3000);
  initAppComponents();
  SerialUSB.begin(0);
  delay(100);
  
  auto broadcast_handle = Motors.createWriteHandle(broadcast_id);
  broadcast_handle.writeByte(reg_id);
  broadcast_handle.writeByte(motor_id);
  broadcast_handle.startTransmission();
  while (broadcast_handle.poll() > 0) { }
  
  delay(100);
  broadcast_handle.writeByte(reg_return_delay_time);
  broadcast_handle.writeByte(5);  // 10uS return delay
  broadcast_handle.startTransmission();
  while (broadcast_handle.poll() > 0) { }
  broadcast_handle.close();
  delay(100);
  
}

void loop() {
  Motors.pingMotor(motor_id);
  
  auto handle = Motors.createWriteHandle(motor_id);
  
  handle.writeByte(reg_led);
  handle.writeByte(1);
  handle.startTransmission();
  while (handle.poll() > 0) { }
  delay(500);
  
  handle.writeByte(reg_led);
  handle.writeByte(0);
  handle.startTransmission();
  while (handle.poll() > 0) { }
  delay(500);
}