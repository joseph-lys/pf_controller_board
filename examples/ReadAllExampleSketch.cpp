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
  Motors.readAllMotors(feedback_datas);
  for (i=0; i < sizeof(motor_ids); i++) {
    id = motor_ids[i];
    auto data = feedback_datas[id];
    SerialUSB.print("id: ");
    SerialUSB.print(static_cast<unsigned int>(id));
    if (data.is_valid) {
      SerialUSB.print(", status:");
      SerialUSB.print(static_cast<unsigned int>(data.status), HEX);
      SerialUSB.print(", pos:");
      SerialUSB.print(data.present_position);
      SerialUSB.print(", spd:");
      SerialUSB.print(data.present_speed);
      SerialUSB.print(", load:");
      SerialUSB.print(data.present_load);
      SerialUSB.println();
    } else {
      SerialUSB.print(", NOT FOUND");
      SerialUSB.println();
    }      
  }

  delay(1000);
}