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
  uint32_t reg =0ul;
  uint32_t count =0xfffffffful;
  int status = 0;
  uint32_t elapsed = 0;
};
Temp temp[10];
int idx = 0;
  
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
  while (1) {
    auto status = dxl0.poll();    
    if (status==DxlDriver::kErrorTimeout) {
      working = DmaCommon::workingDesc(2);
      temp[idx].reg = working->DSTADDR.reg;
      temp[idx].count = working->BTCNT.reg;
      temp[idx].status = -1;
      temp[idx].elapsed = micros() - start_time;
      idx++;
      auto x = 0;
      break;
    }
    if (status==DxlDriver::kErrorInvalidReceiveData | status==DxlDriver::kErrorInvalidReceiveData) {
      auto x = 0;
      break;
    }
    if (status==DxlDriver::kDone) {
      working = DmaCommon::workingDesc(2);
      temp[idx].reg = working->DSTADDR.reg;
      temp[idx].count = working->BTCNT.reg;
      temp[idx].status = 1;
      temp[idx].elapsed = micros() - start_time;
      idx++;
      id = dxl0.getRxId();
      pos = dxl0.readRxWord();
      spd = dxl0.readRxWord();
      torq = dxl0.readRxWord();
      auto x = 0;
      break;
    }
  }
  while (idx>=10)
  {
    auto x = 0;
          
  }
  SerialUSB.println(id);
  delay(1000);
}