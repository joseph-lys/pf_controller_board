/// motor_driver.h
///
/// Copyright (c) 2020 Joseph Lee Yuan Sheng
///
/// This file is part of pf_controller_board which is released under MIT license.
/// See LICENSE file or go to https://github.com/joseph-lys/pf_controller_board for full license details.
///
/// MotorDriver for this application.
/// provide specific handles to read and write data on the motors
///

#ifndef MOTOR_DRIVER_H_
#define MOTOR_DRIVER_H_


#include "DxlDriver.h"

/// Struct to store feedback data
struct MotorFeedbackData {  // make this data fixed, simpler
  uint8_t status = 0xff;
  uint16_t position = 0xffff;
  uint16_t speed = 0xffff;
  uint16_t torque = 0xffff;
  bool isValid() const {
    return (status != 0xff || position != 0xffff || speed != 0xffff || torque != 0xffff);
  }
};

/// handles for different purposes
namespace motor_handles {

class SyncWriteHandle;
class GenericHandle;
class FeedbackHandle;

}  // namespace motor_handles


/// Factory class for generating handles for communicating with the motors
class MotorHandleFactory {
 public:
  MotorHandleFactory();
  ~MotorHandleFactory();
  
  /// Add a DxlDriver
  /// @param driver instance of DxlDriver
  void addDriver(DxlDriver& driver);

  /// Run Initialization sequence
  /// This will search for motors on each driver and and it to memory
  void init();

  /// Ping a motor
  /// @param motor_id id of motor to ping
  bool pingMotor(uint8_t motor_id);

  /// Creates a handle for SyncWrite
  motor_handles::SyncWriteHandle createSyncWriteHandle(uint8_t target_register, uint8_t number_of_bytes);

  /// Create a handle to write specific values to a motor
  /// @param id motor's id number, use DxlProcotolV1::kBroadcastId to broadcast
  motor_handles::GenericHandle createWriteHandle(uint8_t id);

  /// Create a handle to get feedback from all motors
  motor_handles::FeedbackHandle createFeedbackHandle();

private:
  friend motor_handles::SyncWriteHandle;
  friend motor_handles::GenericHandle;
  friend motor_handles::FeedbackHandle;
  enum Constants: uint8_t {
    kMaxMotors = 32,
    kMaxDrivers = 4,
    kNoDriver = 0xff,
  };
  bool driver_lock_ = false;
  uint8_t n_drivers_ = 0;
  uint8_t* p_id_mappings_ = nullptr;
  DxlDriver** p_drivers_ = nullptr;
  MotorFeedbackData* p_feedbacks_ = nullptr; 
  inline uint8_t getDriverIndex(uint8_t id) { 
    return (id < kMaxMotors) ? p_id_mappings_[id] : 0xff; 
  }

  inline DxlDriver* getDxlDriver(uint8_t id) { 
    uint8_t idx = getDriverIndex(id);
    return (idx < n_drivers_) ? p_drivers_[idx] : nullptr;
  }
};



namespace motor_handles {


class SyncWriteHandle {
 public:
  SyncWriteHandle() = default;
  SyncWriteHandle(MotorHandleFactory* p_motor_driver, uint8_t target_register, uint8_t n_bytes);
  ~SyncWriteHandle();
  SyncWriteHandle(SyncWriteHandle&) = delete;  // no copy constructor
  SyncWriteHandle& operator=(SyncWriteHandle&) = delete;  // no copy assignment
  SyncWriteHandle(SyncWriteHandle&&) = default; // default move constructor
  SyncWriteHandle& operator=(SyncWriteHandle&&) = default;  // default move assignment

  bool toMotor(uint8_t id);
  bool writeByte(uint8_t value);
  bool writeWord(uint16_t value);
  bool startTransmission();
  bool poll();
  bool close();

 private:
  bool initialized_[MotorHandleFactory::kMaxDrivers];
  uint8_t state_ = 0;
  uint8_t target_register_ = 0xff;
  uint8_t n_bytes_ = 0;
  uint8_t written_ = 0;
  DxlDriver* p_current_dxl_ = nullptr;
  MotorHandleFactory* p_motors_ = nullptr;
};

class GenericHandle {
 public:
  GenericHandle() = default;
  GenericHandle(MotorHandleFactory* p_motor_driver, uint8_t id);
  ~GenericHandle();
  GenericHandle(GenericHandle&) = delete;  // no copy constructor
  GenericHandle& operator=(GenericHandle&) = delete;  // no copy assignment
  GenericHandle(GenericHandle&&) = default; // default move constructor
  GenericHandle& operator=(GenericHandle&&) = default;  // default move assignment

  bool setInstruction(uint8_t ins);
  bool writeByte(uint8_t value);
  bool writeWord(uint16_t value);
  bool startTransmission();
  bool poll();
  uint8_t getMotorId();
  uint8_t getMotorStatus();
  uint8_t readByte();
  uint8_t readWord();
  bool close();

 private:
  uint8_t state_ = 0;
  uint8_t id_ = 0xff;
  DxlDriver* p_current_dxl_ = nullptr;
  MotorHandleFactory* p_motors_ = nullptr;
};

class FeedbackHandle {
  public:
  FeedbackHandle() = default;
  FeedbackHandle(MotorHandleFactory* p_motor_driver);
  ~FeedbackHandle();
  FeedbackHandle(FeedbackHandle&) = delete;  // no copy constructor
  FeedbackHandle& operator=(FeedbackHandle&) = delete;  // no copy assignment
  FeedbackHandle(FeedbackHandle&&) = default; // default move constructor
  FeedbackHandle& operator=(FeedbackHandle&&) = default;  // default move assignment

  /// Start reading all the motors. 
  /// ***THIS FUNCTION BLOCKS UNTIL ALL MOTORS FEEDBACK IS PROCESSED.***
  bool readAllMotors();
  
  /// Get feedback data for a particular motor
  MotorFeedbackData getFeedback(uint8_t id);
  
  /// Closes the driver,
  bool close();

 private:
  uint8_t combined_state_ = 0;
  uint8_t states_[MotorHandleFactory::kMaxDrivers] = {0};
  MotorHandleFactory* p_motors_ = nullptr;
  enum Constants : uint8_t {
    kFirstReg = 36,
    kByteSize = 6
  };

};

}  // namespace motor_handles


#endif /* MOTOR_DRIVER_H_ */