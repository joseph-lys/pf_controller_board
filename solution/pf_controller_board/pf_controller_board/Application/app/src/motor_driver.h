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
  bool valid = false;
  uint8_t status = 0xff;
  uint16_t position = 0xffff;
  uint16_t speed = 0xffff;
  uint16_t torque = 0xffff;
};

/// handles for different purposes
namespace motor_handles {

class SyncWriteHandle;
class SingleHandle;
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

  /// Create a handle to write specific values to a motor
  /// @param id motor's id number, use DxlProcotolV1::kBroadcastId to broadcast
  motor_handles::SingleHandle createWriteHandle(uint8_t id);
  
  /// Creates a handle for SyncWrite
  motor_handles::SyncWriteHandle createSyncWriteHandle(uint8_t target_register, uint8_t number_of_bytes);


  /// Create a handle to get feedback from all motors
  motor_handles::FeedbackHandle createFeedbackHandle();

private:
friend motor_handles::SyncWriteHandle;
  friend motor_handles::SingleHandle;
  friend motor_handles::FeedbackHandle;
  enum Constants: uint8_t {
    kMaxMotors = 32,
    kMaxDrivers = 4,
    kNoDriver = 0xff,
  };
  bool driver_lock_[kNoDriver];
  uint8_t n_drivers_ = 0;
  uint8_t* p_id_mappings_ = nullptr;
  DxlDriver* p_drivers_ = nullptr;
  MotorFeedbackData* p_feedbacks_ = nullptr; 
  inline uint8_t getDriverIndex(uint8_t id) { 
    return (id < kMaxDrivers) ? p_id_mappings_[id] : 0xff; 
  }

  inline DxlDriver* getDxlDriver(uint8_t id) { 
    uint8_t idx = getDriverIndex(id);
    return (idx < n_drivers_) ? p_drivers_[idx] : nullptr;
  }
  
  inline bool aquireLock(uint8_t driver_index) {
    bool lock_success = false;
    if (driver_index < n_drivers_) {
      if (!driver_lock_[driver_index]) {
        driver_lock_[driver_index] = true;
        lock_success = true;
      }
    }
    return lock_success;
  }
  inline bool aquireLockAll() {
    int i;
    bool lock_success = false;
    for (i=0; i<n_drivers_; i++) {
      if (driver_lock_[i]) {
        lock_success=false;  
        break;
      } else {
        lock_success = true;
      }
    }
    if (lock_success) {
      for (i=0; i<n_drivers_; i++) {
        driver_lock_[i] = true;
      }
    }
    return lock_success;
  }
  inline bool releaseLock(uint8_t driver_index) {
    bool unlock_success = false;
    if (driver_index < n_drivers_) {
      driver_lock_[driver_index] = false;
      unlock_success = true;
    }
    return unlock_success;
  }
  inline bool releaseLockAll() {
    int i;
    for (i=0; i<n_drivers_; i++) {
      driver_lock_[i] = false;
    }
    return true;
  }
};



namespace motor_handles {

class SingleHandle {
  public:
  SingleHandle();
  explicit SingleHandle(DxlDriver* driver, uint8_t id, uint8_t ins);
  virtual ~SingleHandle();

  virtual bool writeByte(uint8_t value);
  virtual bool writeWord(uint16_t value);
  virtual bool startTransmission();
  /// poll for transaction completion
  /// @returns 0: done, 1: not complete, -1 any error
  virtual int poll();
  virtual uint8_t getMotorId();
  virtual uint8_t getMotorStatus();
  virtual uint8_t readByte();
  virtual uint8_t readWord();
  
 protected:
  uint8_t state_ = 0;
  uint8_t const id_ = 0xff;
  uint8_t const ins_ = 0xff; 
  DxlDriver* p_driver_ = nullptr;
};

class BroadcastHandle : public SingleHandle {
  explicit BroadcastHandle(MotorHandleFactory* p_motors, uint8_t ins);
  ~BroadcastHandle();
  
  bool writeByte(uint8_t value) override;
  bool writeWord(uint16_t value) override;
  bool startTransmission() override;
  int poll() override;
  uint8_t getMotorId() override;
  uint8_t getMotorStatus() override;
  uint8_t readByte() override;
  uint8_t readWord() override;

 private:
  MotorHandleFactory* p_motor_ = nullptr;
  SingleHandle* handles_ = nullptr;
};


/// Handle that encapsulates both transaction to single motor as well as broadcast
/// Also handles resource management via lock/release mechanism.
/// To release driver call the close() function
class GenericHandle {
 public:
  GenericHandle() = default;
  ~GenericHandle();
  explicit GenericHandle(MotorHandleFactory* p_motors, uint8_t id, uint8_t ins);
  virtual GenericHandle = default;
  GenericHandle(GenericHandle&) = delete;  // no copy constructor
  GenericHandle& operator=(GenericHandle&) = delete;  // no copy assignment
  GenericHandle(GenericHandle&&) = default; // default move constructor
  GenericHandle& operator=(GenericHandle&&) = default;  // default move assignment
  inline operator bool() const {
    return p_motors_ != nullptr;
  }
  
  bool writeByte(uint8_t value) = 0;
  bool writeWord(uint16_t value) = 0;
  bool startTransmission() = 0;
  bool poll() = 0;
  uint8_t getMotorId() = 0;
  uint8_t getMotorStatus() = 0;
  uint8_t readByte() = 0;
  uint8_t readWord() = 0;
  
  virtual bool close();
 protected:
  uint8_t state_ = 0;
  MotorHandleFactory* p_motors_ = nullptr;
  const uint8_t id_;
  const uint8_t ins_;
  SingleHandle* handle_;  // actual implementation
};

class SyncWriteHandle{
 public:
  SyncWriteHandle() = default;
  SyncWriteHandle(MotorHandleFactory* p_motor_driver, uint8_t target_register, uint8_t n_bytes);
  ~SyncWriteHandle();
  SyncWriteHandle(SyncWriteHandle&) = delete;  // no copy constructor
  SyncWriteHandle& operator=(SyncWriteHandle&) = delete;  // no copy assignment
  SyncWriteHandle(SyncWriteHandle&&) = default; // default move constructor
  SyncWriteHandle& operator=(SyncWriteHandle&&) = default;  // default move assignment

  bool toMotor(uint8_t id);
  bool writeByte(uint8_t value) override;
  bool writeWord(uint16_t value) override;
  bool startTransmission() override;
  bool poll() override;
  bool close() override;

 private:
  uint8_t state_ = 0;
  uint8_t target_register_ = 0xff;
  uint8_t n_bytes_ = 0;
  uint8_t written_ = 0;
  SingleHandle current_handle_ = nullptr;
  SingleHandle* handle_ = nullptr;
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
  uint8_t* const p_driver_states_ = nullptr;
  uint8_t* const p_reqeust_id_ = nullptr;
  uint8_t* const p_retries_ = nullptr;
  MotorHandleFactory* p_motors_ = nullptr;
  enum Constants : uint8_t {
    kFirstReg = 36,
    kByteSize = 6,
    kMaxRetry = 1
  };
};

}  // namespace motor_handles


#endif /* MOTOR_DRIVER_H_ */