/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>
#include "dmashared.h"

/*End of auto generated code by Atmel studio */


//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio

#define LED_PIN 13
void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(1000000);
  Serial1.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  Dma::init();
}

void loop() {
  // put your main code here, to run repeatedly:
  int x;
  SerialUSB.println("USB_TEST_LOAD");
  while(Serial1.available()) {
    x = Serial1.read();
    if(x > 0) {
      SerialUSB.write(static_cast<uint8_t>(x));
    }
  }
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  Serial1.println("SERIAL1_TEST_LOAD");
  delay(1000);
  /// Debug configurations
  Pm* pm = PM;
  Sercom* sercom = SERCOM2;
  Dmac* dmac = DMAC;
  DmacDescriptor* first_desc = Dma::firstDesc(0);
  DmacDescriptor* working_desc = Dma::workingDesc(0);
  
  delay(0);
}
