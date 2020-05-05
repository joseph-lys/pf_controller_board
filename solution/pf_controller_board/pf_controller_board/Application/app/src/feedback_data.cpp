/*
 * motor_data.cpp
 *
 * Created: 5/5/2020 7:06:52 PM
 *  Author: josep
 */ 


#include "feedback_data.h"


FeedbackData& FeedbackDataArray::operator[](uint8_t id) {
  return datas_[id];
}
void FeedbackDataArray::initializeDatas() {
  int i;
  for (i=0; i<MotorHandleFactory::kMaxMotors; i++) {
    datas_[i].is_valid = false;
  }  
}

void FeedbackDataArray::replyFromHandle(motor_handles::BaseHandle& handle, uint8_t id) {
  datas_[id].status = handle.getMotorStatus();
  datas_[id].present_position = handle.readByte();
  datas_[id].present_position = handle.readByte();
  datas_[id].present_position = handle.readByte();
  datas_[id].is_valid = true;
}

void FeedbackDataArray::errorFromHandle(uint8_t id) {
  datas_[id].is_valid = true;
  datas_[id].status = kStatusError;
}

uint8_t FeedbackDataArray::requestedRegister() {
  return kRegister;
}

uint8_t FeedbackDataArray::requestedLength() {
  return kLength;
}

uint8_t FeedbackDataArray::requestRetries() {
  return kRetries;
}
