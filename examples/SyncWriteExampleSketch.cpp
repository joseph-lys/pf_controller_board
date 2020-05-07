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


static constexpr uint8_t motor_ids[] = {1, 19};
  
static constexpr uint8_t broadcast_id = 0xfe;
static constexpr uint8_t reg_led = 25;
static constexpr uint8_t reg_return_delay_time = 5;
static constexpr uint8_t number_of_bytes = 1;

static FeedbackDataArray feedback_datas{};

void setup() {
  // put your setup code here, to run once:
  delay(3000);
  initAppComponents();
  SerialUSB.begin(0);
  delay(100);  
}

void loop() {
  uint8_t i, id;
  auto handle = Motors.createSyncWriteHandle(reg_led, number_of_bytes);
  
  for (i=0; i<sizeof(motor_ids); i++) {
    handle.toMotor(motor_ids[i]);
    handle.writeByte(i % 2);    
  }
  handle.startTransmission();
  while(handle.poll() > 0) { }
  
  delay(500);
  
  for (i=0; i<sizeof(motor_ids); i++) {
    handle.toMotor(motor_ids[i]);
    handle.writeByte((i + 1) % 2);    
  }
  handle.startTransmission();
  while(handle.poll() > 0) { }
    
  delay(500);
}