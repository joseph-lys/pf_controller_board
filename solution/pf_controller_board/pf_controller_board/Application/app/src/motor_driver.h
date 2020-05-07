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

#define NO_COPY_CLASS(TypeName)  \
  TypeName(const TypeName&) = delete;   \
  TypeName& operator=(const TypeName&) = delete
#define DEFAULT_MOVE_CLASS(TypeName)  \
  TypeName(TypeName&&) = default;   \
  TypeName& operator=(TypeName&&) = default


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

class BaseHandle;
class SingleHandle;
class BroadcastHandle;
class SyncWriteHandle;
class GenericHandle;

}  // namespace motor_handles

/// Factory class for generating handles for communicating with the motors
class MotorHandleFactory {
 public:
  MotorHandleFactory();
  ~MotorHandleFactory();
  
  /// Run Initialization sequence
  /// This will search for motors on each driver and and it to memory
  /// * this is a blocking method
  void init();

  /// Ping a motor
  /// @param motor_id id of motor to ping
  bool pingMotor(uint8_t motor_id);

  /// Create a handle to write specific values to a motor
  /// @param id motor's id number, use DxlProcotolV1::kBroadcastId to broadcast
  motor_handles::GenericHandle createWriteHandle(uint8_t id);
  
  /// Creates a handle for SyncWrite
  motor_handles::SyncWriteHandle createSyncWriteHandle(uint8_t target_register, uint8_t number_of_bytes);
  
  /// Add a DxlDriver
  /// @param driver instance of DxlDriver
  void addDriver(DxlDriver& driver);

  uint8_t countDrivers();
  
  inline uint8_t getDriverIndex(uint8_t id) {
    return (id < kMaxMotors) ? p_id_mappings_[id] : 0xff;
  }
  inline DxlDriver* getDriverPtrById(uint8_t id) {
    uint8_t idx = getDriverIndex(id);
    return (idx < n_drivers_) ? p_drivers_[idx] : nullptr;
  }
  inline DxlDriver** getDriverPtrArray() {
    return p_drivers_;
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
    bool lock_success = true;
    for (i=0; i<n_drivers_; i++) {
      if (driver_lock_[i]) {
        lock_success=false;
        break;
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
  enum Constants: uint8_t {
    kMaxMotors = 32,
    kMaxDrivers = 4,
    kDoesNotExists = 0xff,
  };
  /// Mapping Queue takes existing motor mapping to make a set of queues
  class MappingQueue {
   public:
    void build(uint8_t* mapping); // build the queue using an array motor mappings
    uint8_t pop(uint8_t index);  // remove one
    bool hasItem(uint8_t index);  // peek to see if there is any
    void rewind();  // resets the queue to the state after build()
   private:
    uint8_t motor_ids_[kMaxMotors];
    uint8_t start_[kMaxDrivers];
    uint8_t end_[kMaxDrivers];
  };
  
  /// Motor Data Interface
  struct MotorDataInterface {
    friend MotorHandleFactory;
    /// These functions are used by user to read the data
    virtual bool hasData(uint8_t id) { return false; }
      
    private:
    /// These functions are used to by the handle to update the data
    virtual void initializeDatas() = 0;
    virtual void replyFromHandle(motor_handles::BaseHandle&, uint8_t id) = 0;
    virtual void errorFromHandle(uint8_t id) = 0;
    virtual uint8_t requestedRegister() = 0;
    virtual uint8_t requestedLength() = 0;
    virtual uint8_t requestRetries() = 0;
  };
  /// Start reading all the motors.
  /// ***THIS FUNCTION BLOCKS UNTIL ALL MOTORS FEEDBACK IS PROCESSED.***
  bool readAllMotors(MotorDataInterface& datas);
 private:
  uint8_t n_drivers_ = 0;
  bool driver_lock_[kMaxDrivers];
  uint8_t* p_id_mappings_ = nullptr;
  DxlDriver** p_drivers_ = nullptr;
  uint8_t* p_request_id_ = nullptr;
  uint8_t* p_retries_ = nullptr;
  motor_handles::SingleHandle* p_handles_ = nullptr;
  MappingQueue queue_;
};



namespace motor_handles {
  
/// Handle interface
class BaseHandle {
 public:
  BaseHandle() = default;
  explicit BaseHandle(uint8_t state) : state_(state) {}
  virtual ~BaseHandle() = default;
  virtual operator bool() const { return false; }
  virtual bool setInstruction(uint8_t id, uint8_t ins) { return false; }
  virtual bool writeByte(uint8_t value) { return false; }
  virtual bool writeWord(uint16_t value) { return false; }
  virtual bool startTransmission() { return false; }
  /// poll for transaction completion
  /// @returns 0: done, 1: not complete, -1 any error
  virtual int poll()  { return -1; };
  virtual uint8_t getMotorId() { return 0xff; }
  virtual uint8_t getMotorStatus() { return 0xff; }
  virtual uint8_t readByte() { return 0xff; }
  virtual uint8_t readWord() { return 0xffff; }
  virtual void close() {}
  virtual uint8_t getState() { return state_; }
 protected:
  uint8_t state_ = 0;
  inline void setState(uint8_t next_state) { state_ = next_state; }
};

class SingleHandle : public BaseHandle {
 public:
  SingleHandle();
  SingleHandle(DxlDriver* p_driver);
  operator bool() const { return p_driver_ != nullptr; }
  bool setInstruction(uint8_t id, uint8_t ins) override;
  bool writeByte(uint8_t value) override;
  bool writeWord(uint16_t value) override;
  bool startTransmission() override;
  /// poll for transaction completion
  /// @returns 0: done, 1: not complete, -1 any error
  int poll() override;
  uint8_t getMotorId() override;
  uint8_t getMotorStatus() override;
  uint8_t readByte() override;
  uint8_t readWord() override;
  void close() override;
 protected:
  uint8_t memoized_result_ = 0;
  DxlDriver* p_driver_=nullptr;
 private:
  bool checkState(uint8_t expected_state);
};


class BroadcastHandle : public BaseHandle{
 public:
  BroadcastHandle();
  BroadcastHandle(uint8_t n_drivers, DxlDriver** p_drivers=nullptr);
  ~BroadcastHandle();
  operator bool() const { return p_handles_ != nullptr; }
  bool setInstruction(uint8_t id, uint8_t ins) override;
  bool writeByte(uint8_t value) override;
  bool writeWord(uint16_t value) override;
  bool startTransmission() override;
  /// poll for transaction completion
  /// @returns 0: done, 1: not complete, -1 any error
  int poll() override;
  void close() override;
  uint8_t getState() override;
 protected:
  SingleHandle* p_handles_ = nullptr;
  const uint8_t n_handles_ = 0; 
  int memoized_result_ = -1;
  bool checkState(uint8_t expected_state);
};


/// Handle that encapsulates both transaction to single motor as well as broadcast
/// Also handles resource management via lock/release mechanism.
/// To release driver call the close() function
class GenericHandle : private BaseHandle {
 public:
  NO_COPY_CLASS(GenericHandle);
  DEFAULT_MOVE_CLASS(GenericHandle);
  GenericHandle() = default;
  explicit GenericHandle(MotorHandleFactory* p_motors, uint8_t id, uint8_t ins);
  
  ~GenericHandle();
  operator bool() const override { return p_motors_ != nullptr; }
  
  bool setInstruction(uint8_t id, uint8_t ins) override;
  bool writeByte(uint8_t value) override;
  bool writeWord(uint16_t value) override;
  bool startTransmission() override;
  int poll() override;
  uint8_t getMotorId() override;
  uint8_t getMotorStatus() override;
  uint8_t readByte() override;
  uint8_t readWord() override;
  
  void close() override;
 protected:
  const uint8_t id_;
  const uint8_t ins_;
  BaseHandle* handle_;  // actual implementation
  MotorHandleFactory* p_motors_ = nullptr;
};

class SyncWriteHandle : private BroadcastHandle {
 public:
  
  NO_COPY_CLASS(SyncWriteHandle);
  DEFAULT_MOVE_CLASS(SyncWriteHandle);
  SyncWriteHandle() = default;
  SyncWriteHandle(MotorHandleFactory* p_motor_driver, uint8_t target_register, uint8_t n_bytes);
  ~SyncWriteHandle();
  operator bool() const override { return p_motors_ != nullptr; }

  bool toMotor(uint8_t id);
  bool writeByte(uint8_t value) override;
  bool writeWord(uint16_t value) override;
  using BroadcastHandle::startTransmission;
  using BroadcastHandle::poll;
  void close() override;

 private:
  uint8_t target_register_ = 0xff;
  uint8_t n_bytes_ = 0;
  uint8_t written_ = 0;
  SingleHandle* current_handle_ = nullptr;
  MotorHandleFactory* p_motors_ = nullptr;
};


}  // namespace motor_handles


#endif /* MOTOR_DRIVER_H_ */