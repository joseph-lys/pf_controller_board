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
  SPI.startTransactionInterrupt();
}

void SpiEnd_Handler (void) {
  volatile int x;
  auto rxw = Dma::workingDesc(0);
  auto txw = Dma::workingDesc(1);
  auto rxf = Dma::firstDesc(0);
  auto txf = Dma::firstDesc(1);

  SPI.endTransactionInterrupt();
  
  delay(10);
  x++;
  
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
  // SPI 
  // Duplicated SS signal to trigger an action when SPI transfer complete
  pinMode(LED_PIN, INPUT);
  attachInterrupt(LED_PIN, &SpiEnd_Handler, RISING);
  SPI.begin();
  
  delay(500);
}

uint8_t serial_test[] = "abcde";
uint8_t dma_test[] = "dmatestload\n";
void loop() {
  volatile int x;

  while(1) {

    delay(5000);
    x++;
    delay(1);
    x++;
    delay(1);
    x++;
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
  delay(3000);
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
