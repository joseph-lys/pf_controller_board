/*
 * motor_data.h
 *
 * Created: 5/5/2020 7:07:05 PM
 *  Author: josep
 */ 


#ifndef FEEDBACK_DATA_H_
#define FEEDBACK_DATA_H_

#include "motor_driver.h"

struct FeedbackData {
  bool is_valid = false;
  uint8_t status;
  uint16_t present_position;
  uint16_t present_speed;
  uint16_t present_load;
};


struct FeedbackDataArray : public MotorHandleFactory::MotorDataInterface {
  FeedbackData& operator[](uint8_t id);
  void initializeDatas() override;
  void replyFromHandle(motor_handles::BaseHandle& handle, uint8_t id) override;
  void errorFromHandle(uint8_t id) override;
  uint8_t requestedRegister() override;
  uint8_t requestedLength() override;
  uint8_t requestRetries() override;
 private:
  FeedbackData datas_[MotorHandleFactory::kMaxMotors];
  enum Constants : uint8_t {
    kRegister = 36,
    kLength = 6,
    kRetries = 2,
    kStatusError = 0x80
  };
};




#endif /* FEEDBACK_DATA_H_ */