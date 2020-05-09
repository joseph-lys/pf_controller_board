/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include "pf_board.h"
#include "feedback_data.h"
/*End of auto generated code by Atmel studio */

//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio

#define LED_PIN 13


static FeedbackDataArray feedback_datas{};
static constexpr uint8_t motor_ids[] = {1, 19};
static constexpr uint8_t broadcast_id = 0xfe;
static constexpr uint8_t reg_led = 25;
static constexpr uint8_t reg_return_delay_time = 5;
static constexpr uint8_t number_of_bytes = 1;



uint8_t big_buffer[64];
void setup() {
  // put your setup code here, to run once:
  delay(3000);
  initAppComponents();
  SerialUSB.begin(0);
  delay(100);  
  for (uint8_t i=0; i<8; i++) {
    big_buffer[i] = 0;
  }
  volatile unsigned long x = micros();
  DmaSPI.doBeforeSpiStarts();
  x = micros() - x;
  if (x) {
    
  }
}

void loop() {
  uint8_t* p_tx;
  uint8_t* p_rx;
  
  p_rx = DmaSPI.getRxDataPtr();
  if (p_rx != nullptr) {
    p_tx = DmaSPI.getTxDataPtr();
    for(int i=0; i<8; i++) {
      p_tx[i] = p_rx[i]; 
    }
    DmaSPI.queueTxData();
  }
  delay(250);
  // auto broadcast_handle = Motors.createWriteHandle(broadcast_id);
  // broadcast_handle.writeByte(reg_led);
  // broadcast_handle.writeByte(1);
  // broadcast_handle.startTransmission();
  // while (broadcast_handle.poll() > 0) { }
  
  // delay(500);
  // broadcast_handle.writeByte(reg_led);
  // broadcast_handle.writeByte(0);
  // broadcast_handle.startTransmission();
  // while (broadcast_handle.poll() > 0) { }
  
  // broadcast_handle.close();
  // delay(500);
}