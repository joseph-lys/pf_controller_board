/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include "DmaCommon.h"
#include "wiring_private.h"
#include "DmaSPI.h"
/*End of auto generated code by Atmel studio */

//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio


#define LED_PIN 13

void SERCOM4_Handler (void) {
  
  // debugging stuff
  DmacDescriptor* first_rx = Dma::firstDesc(0);
  DmacDescriptor* working_rx = Dma::workingDesc(0);
  DmacDescriptor* first_tx = Dma::firstDesc(1);
  DmacDescriptor* working_tx = Dma::workingDesc(1);
  volatile int x = 0;

  SPI.ssInterrupt();
  return;
  
  // move this to a seperate handler
  bool ss_value = static_cast<bool>(digitalRead(LED_PIN));
  uint8_t* p_tx;
  uint8_t* p_rx;

  if(ss_value) {
    p_tx = SPI.getTxDataPtr();
    p_rx = SPI.getRxDataPtr();
    if(p_rx != nullptr) {
      for(int i=0; i<SPI.buffer_size; i++) {
        p_tx[i] = p_rx[i];
      }
    }
    SPI.queueTxData();
  }
}

void setup() {
  // put your setup code here, to run once:
  Dma::init();
  /// Additional configuration for SERCOM 2 as UART
  SerialUSB.begin(1000000);
  Serial1.begin(9600);
  Serial.begin(9600);
  //Serial RX: D3 (PA09), TX: D4 (PA08)
  pinPeripheral(3, PIO_SERCOM_ALT);
  pinPeripheral(4, PIO_SERCOM_ALT);
  //SPI 
  SPI.begin();
  NVIC_DisableIRQ(SERCOM4_IRQn);
  delay(3000);
  //attachInterrupt(SS, SPI_InterruptHandler, CHANGE);
}

uint8_t serial_test[] = "abcde";
uint8_t dma_test[] = "dmatestload\n";
void loop() {
  volatile int x;
  /// Debug configurations
  Pm* pm = PM;
  Sercom* sercom = SERCOM2;
  Dmac* dmac = DMAC;
  DmacDescriptor* first_rx = Dma::firstDesc(0);
  DmacDescriptor* working_rx = Dma::workingDesc(0);
  DmacDescriptor* first_tx = Dma::firstDesc(1);
  DmacDescriptor* working_tx = Dma::workingDesc(1);
  
  // spi aggressive test loop
  uint8_t* ptr_tx = nullptr;
  uint8_t* ptr_rx = nullptr;
  while(0) {
    ptr_rx = SPI.getRxDataPtr();
    if(ptr_rx) {
      ptr_tx = SPI.getTxDataPtr();
      for (int i=0; i<SPI.buffer_size; i++) {
        ptr_tx[i] = ptr_rx[i];
      }
      SPI.queueTxData();
    }
  }
  while(1) {
    
    SPI.ssInterrupt();
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
    x++;
    delay(5000);
  }
  
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
  // digitalWrite(LED_PIN, LOW);
  delay(3000);
  // digitalWrite(LED_PIN, HIGH);
  Serial1.write(serial_test, sizeof(serial_test));
  Serial.write(dma_test, sizeof(dma_test));
  
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
