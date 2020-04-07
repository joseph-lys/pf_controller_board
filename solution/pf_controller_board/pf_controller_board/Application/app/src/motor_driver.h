/*
 * dxl_driver.h
 *
 * Created: 6/4/2020 7:52:12 PM
 *  Author: josep
 */ 


#ifndef MOTOR_DRIVER_H_
#define MOTOR_DRIVER_H_


#include "DxlDriver.h"

struct MotorFeedback {  // make this data fixed, simpler
  uint8_t status = 0xff;
  uint16_t position = 0xffff;
  uint16_t speed = 0xffff;
  uint16_t torque = 0xffff;
};

namespace motor_handles {

class SyncWriteHandle;
class GenericHandle;
class FeedbackHandle;

}  // namespace motor_handles


class MotorDriver {
  friend motor_handles::SyncWriteHandle;
  friend motor_handles::GenericHandle;
  friend motor_handles::FeedbackHandle;
 private:
  enum uint8_t {
    kMaxMotors = 32,
    kMaxDrivers = 4,
    kNoDriver = 0xff,
  };
  bool driver_lock_ = false;
  uint8_t n_drivers = 0;
  uint8_t motor_mapping[kMaxMotors]{kNoDriver};
  DxlDriver* drivers_[kMaxDrivers]{nullptr};

  inline uint8_t getDriverIndex(uint8_t id) { 
    return (id < kMaxMotors) ? motor_mapping[id] : 0xff; 
  }

  inline DxlDriver* getDxlDriver(uint8_t id) { 
    uint8_t idx = getDriverIndex(id);
    return (idx < n_drivers) ? drivers_[idx] : nullptr;
  }

 public:  
  inline void addDriver(DxlDriver& driver) { 
    if (n_drivers < kMaxDrivers) {
      drivers_[n_drivers++] = &driver;
    }
  }

  void init();

  bool pingMotor(uint8_t motor_id);

  motor_handles::SyncWriteHandle createBroadcastHandle();

  motor_handles::GenericHandle createWriteHandle(uint8_t id);  

  motor_handles::FeedbackHandle createFeedbackHandle(uint8_t reg_adr, uint8_t n_bytes);
};



namespace motor_handles {


class SyncWriteHandle {
  bool initialized_[MotorDriver::kMaxDrivers];
  uint8_t state_ = 0;
  DxlDriver* p_current_dxl_ = nullptr;
  MotorDriver* p_motors_ = nullptr;
 public:
  SyncWriteHandle(MotorDriver* p_motor_driver);
  ~SyncWriteHandle();
  bool toMotor(uint8_t id);
  bool writeByte(uint8_t value);
  bool writeWord(uint16_t value);
  bool startTransmission();
  bool poll();
  bool close();
};

class GenericHandle {
  uint8_t state_ = 0;
  const uint8_t id_ = 0xff;
  DxlDriver* p_current_dxl_ = nullptr;
  MotorDriver* p_motors_ = nullptr;
 public:
  GenericHandle(MotorDriver* p_motor_driver, uint8_t id);
  ~GenericHandle();
  bool setInstruction(uint8_t ins);
  bool writeByte(uint8_t value);
  bool writeWord(uint16_t value);
  bool startTransmission();
  bool poll();
  uint8_t getMotorId();
  uint8_t getMotorStatus();
  uint8_t readWord();
  uint8_t writeWord();
  bool close();
};


}  // namespace motor_handles


#endif /* MOTOR_DRIVER_H_ */