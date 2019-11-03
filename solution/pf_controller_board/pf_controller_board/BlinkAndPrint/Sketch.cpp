/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */


//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio

#define LED_PIN 13
void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(1000000);
  Serial1.begin(9600);
  pinMode(LED_PIN, OUTPUT);
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
}
