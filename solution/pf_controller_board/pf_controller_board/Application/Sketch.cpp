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



void setup() {
  // put your setup code here, to run once:
  delay(3000);
  initAppComponents();
  SerialUSB.begin(0);
}

enum Constants : uint8_t {
  kFirstReg = 36,
  kByteSize = 6,
  kMaxRetry = 1
};
 
struct Temp {
  int status = 0;
  uint32_t elapsed = 0;
};


void logData(int status, uint32_t elapsed) {
  constexpr int n_temp = 500;
  static Temp temp[n_temp];
  static int idx{0};
  if (idx < n_temp) {
    temp[idx].status = status;
    temp[idx].elapsed = elapsed;
    idx++;
  } else {
    // summarize and block
    int positive_status = 0;
    int negative_status = 0;
    uint32_t max_elapsed = 0;
    uint32_t max_positive_elapsed = 0;
    double ave_elapsed = 0.0;
    for (int i=0; i<n_temp; i++) {
      if (temp[i].status > 0) {
        positive_status++;
      }
      if (temp[i].status < 0) {
        negative_status++;
      }
      if (temp[i].elapsed > max_elapsed) {
        max_elapsed = temp[i].elapsed;
      }
      if (temp[i].status > 0 && temp[i].elapsed > max_positive_elapsed) {
        max_positive_elapsed = temp[i].elapsed;
      }
      ave_elapsed += static_cast<double>(temp[i].elapsed);
    }
    ave_elapsed /= n_temp;
    
    while (1) {
      volatile int x = 0;
    }
  }
}
  

void loop() {
  volatile uint8_t id;
  volatile uint16_t pos, spd, torq;
  volatile DmacDescriptor* working;
  uint32_t start_time;
  
  volatile long int x, y;
  //SerialUSB.println("Test");
  //auto handle = Motors.createFeedbackHandle();
  //handle.readAllMotors();
  //auto m1 = handle.getFeedback(1);
  //SerialUSB.println(m1.position);
  // Motors.pingMotor(1);
  // Motors.pingMotor(2);
  
  id = 0xff;
  pos = 0xffff;
  spd = 0xffff;
  torq = 0xffff;
  dxl0.setTxIns(1, DxlProtocolV1::Ins::kRead);
  dxl0.writeTxByte(kFirstReg);
  dxl0.writeTxByte(kByteSize);
  dxl0.beginTransmission();
  start_time = micros();
  auto handle = Motors.createFeedbackHandle();
  handle.readAllMotors();
  auto feedback = handle.getFeedback(1);
  if (feedback.valid)
  logData(int status, micros() - start_time);
  SerialUSB.println(handle.getFeedback(1).speed);
  handle.close();
  delay(1000);
}