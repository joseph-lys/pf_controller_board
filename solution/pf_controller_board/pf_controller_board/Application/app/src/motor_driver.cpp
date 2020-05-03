/// motor_driver.cpp
///
/// Copyright (c) 2020 Joseph Lee Yuan Sheng
///
/// This file is part of pf_controller_board which is released under MIT license.
/// See LICENSE file or go to https://github.com/joseph-lys/pf_controller_board for full license details.
///
/// MotorDriver for this application. 
/// provide specific handles to read and write data on the motors
///

#include <Arduino.h>
#include "motor_driver.h"
#include "DxlProtocolV1.h"
typedef unsigned int uint;

//////////////////////////////////////////////////////////////////////////
/// MotorDriver
//////////////////////////////////////////////////////////////////////////
MotorHandleFactory::MotorHandleFactory() {
  int i;
  p_id_mappings_ = new uint8_t[kMaxMotors];
  p_drivers_ = new DxlDriver*[kMaxDrivers];
  p_feedbacks_ = new MotorFeedbackData[kMaxMotors];
  for (i=0; i<kMaxMotors; i++) {
    p_id_mappings_[i] = 0xff;
    p_feedbacks_[i] = MotorFeedbackData{};
  }
  for (i=0; i<kMaxDrivers; i++) {
    p_drivers_[i] = nullptr;
    driver_lock_[i] = false;
  }
}

MotorHandleFactory::~MotorHandleFactory() {
  delete[] p_id_mappings_;
  delete[] p_drivers_;
  delete[] p_feedbacks_;
}

void MotorHandleFactory::addDriver(DxlDriver& driver) {
  if (n_drivers_ < kMaxDrivers) {
    p_drivers_[n_drivers_++] = &driver;
  }
}

void MotorHandleFactory::init() {
  uint8_t i, id;
  for (i= 0; i<3; i++) {  // multiple tries to avoid missing motors
    for (id=0u; id<kMaxMotors; id++) {
      if(pingMotor(id)) {
        break;
      }
    }
  }
}

bool MotorHandleFactory::pingMotor(uint8_t id) {
  bool is_found = false;
  uint8_t i;
  DxlDriver* driver;
  volatile DxlDriver::Status status;
  if (id < static_cast<uint>(kMaxMotors)) {
    for (i=0; i<n_drivers_; i++) {
      driver = p_drivers_[i];
      driver->setTxIns(id, DxlProtocolV1::Ins::kPing);
      driver->beginTransmission();
      volatile uint32_t DEBUG_X = micros();
      for(;;) {
        status = driver->poll();
        volatile uint32_t DEBUG_Y = micros();
        if (status == DxlDriver::kErrorInvalidReceiveData
            || status == DxlDriver::kErrorInvalidTransmitData 
            || status == DxlDriver::kErrorTimeout) {
          break;
        }
        if (status == DxlDriver::kDone) {
          if (driver->getRxId()==id) {
            is_found = true;
            p_id_mappings_[id] = i;  // assign the index of the driver
          }          
          break;
        }
      }
      if (is_found) {
        break;
      }
    }
  }
  return is_found;
}

motor_handles::SyncWriteHandle MotorHandleFactory::createSyncWriteHandle(uint8_t target_register, uint8_t number_of_bytes) {
  if (aquireLockAll()) {
    return motor_handles::SyncWriteHandle(this, target_register, number_of_bytes);  
  } else {
    return motor_handles::SyncWriteHandle();
  }
}

motor_handles::Handle MotorHandleFactory::createWriteHandle(uint8_t id) {
  handle = motor_handles::Handle{};
  if (aquireLockAll()) {
    handle = motor_handles::SingleHandle{this, null, true};
    handle.setInstruction(DxlProtocolV1::Ins::kWrite);  // specialize to a write type
  }
  return handle;
}  

motor_handles::FeedbackHandle MotorHandleFactory::createFeedbackHandle() {
  return  motor_handles::FeedbackHandle(this);
}



namespace motor_handles {

enum TransactionState : uint8_t {
  kInitial = 0,
  kInTransit,
  kTransactionComplete,
  kClosed,
};
//////////////////////////////////////////////////////////////////////////
/// SINGLE HANDLE
//////////////////////////////////////////////////////////////////////////
SingleHandle SingleHandle::create(MotorHandleFactory* p_motor_driver, uint8_t driver_idx) {
  if (p_motor_driver->aquireLock(driver_idx)) {
    return SingleHandle();
  } else {
    return SingleHandle(p_motor_driver, uint8_t driver_idx));
  }
}

SingleHandle::SingleHandle()
: state_(kClosed), p_driver_(nullptr) {
}

SingleHandle::SingleHandle(DxlDriver* driver, uint8_t id, uint8_t ins)
: state_(kInitial), id_(id), ins_(ins), p_driver_(driver) {
  driver->setTxIns(id, ins);
}

SingleHandle::~SingleHandle() {
}

bool SingleHandle::writeByte(uint8_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_driver_ != nullptr) {
    success = p_driver_->writeTxByte(value);
  }
  if (!success) {
    state_ = kClosed;
  }
  return success;
}

bool SingleHandle::writeWord(uint16_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_driver_ != nullptr) {
    success = p_driver_->writeTxWord(value);
  }
  if (!success) {
    state_ = kClosed;
  }
  return success;
}

bool SingleHandle::startTransmission() {
  bool started = false;
  uint8_t i;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (id_ == DxlProtocolV1::kBroadcastId) {
    started = p_driver_->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
  }
  if (started) {
    state_ = kInTransit;
    } else {
    state_ = kClosed;
  }
  return started;
}

int SingleHandle::poll() {
  DxlDriver::Status status;
  uint8_t i;
  int result = -1;
  if (p_driver_ == nullptr) {
    // this is an error
  } else if (state_ == kTransactionComplete) {
    // already done, no need to repeat
  } else if (state_ != kInTransit) {
  // out of sequence call, return default value
  } else {
    status = p_driver_->poll();
    switch(status) {
      case DxlDriver::kReceiving:
      case DxlDriver::kTransmitting:
      result = 1;
      break;
      case DxlDriver::kDone:
      result = 0;
      default:
      result = -1;
    }
  }    
  if (result) {
    state_ = kTransactionComplete;
  }
  return result;
}

uint8_t SingleHandle::getMotorId() {
  return (p_driver_ != nullptr && state_ == kTransactionComplete) ? p_driver_->getRxId() : 0xff;
}

uint8_t SingleHandle::getMotorStatus() {
  return (p_driver_ != nullptr && state_ == kTransactionComplete) ? p_driver_->getRxStatusByte() : 0xff;
}

uint8_t SingleHandle::readByte() {
  return (p_driver_ != nullptr && state_ == kTransactionComplete) ? p_driver_->readRxByte() : 0xff;
}

uint8_t SingleHandle::readWord() {
  return (p_driver_ != nullptr && state_ == kTransactionComplete) ? p_driver_->readRxWord() : 0xffff;
}


//////////////////////////////////////////////////////////////////////////
/// BroadcastHandle
//////////////////////////////////////////////////////////////////////////
BroadcastHandle::BroadcastHandle(MotorHandleFactory* p_motors, uint8_t ins) {
  int i;
  DxlDriver driver;
  handles_ = new SingleHandle[p_motors->n_drivers_];
  for (i=0; i <p_motors->n_drivers_; i++) {
    handles_[i] = {p_motors->p_drivers_[i], DxlProtocolV1::kBroadcastId, ins};
  }
}

bool BroadcastHandle::writeByte(uint8_t value) {
  int i;
  bool success = false;
  if (p_motor_ == nullptr) {
    // error
    } else if (state_ != kInitial) {
    // error
    } else {
    success = true;
    for (i=0; i<p_motor_->n_drivers_; i++) {
      success = (handles_[i].writeByte(value)) ? success : false;
    }
  }
  return success;
}
bool BroadcastHandle::writeWord(uint16_t value) {
  int i;
  bool success = false;
  if (p_motor_ == nullptr) {
    // error
    } else if (state_ != kInitial) {
    // error
    } else {
    success = true;
    for (i=0; i<p_motor_->n_drivers_; i++) {
      success = (handles_[i].writeWord(value)) ? success : false;
    }
  }
  return success;
}

bool BroadcastHandle::startTransmission() {
  int i;
  bool success = false;
  if (p_motor_ == nullptr) {
    // error
  } else if (state_ != kInitial) {
    // error  
  } else {
    success = true;
    for (i=0; i<p_motor_->n_drivers_; i++) {
      success = (handles_[i].startTransmission()) ? success : false;
      state_ = kInTransit;
    }
  }
  return success;
}

int BroadcastHandle::poll() {
  int i;
  int poll_status;
  int summary_poll_status = -1;
  if (p_motor_ == nullptr) {
    // error
    } else if (state_ != kInitial) {
    // error
    } else {
    summary_poll_status = 0;
    for (i=0; i<p_motor_->n_drivers_; i++) {
      poll_status = handles_[i].poll() == 0;
      if (poll_status > 1) {
        // if any driver is still busy, give a busy status 
        summary_poll_status = 1;
      } else if (poll_status < 0) {
        // if no driver busy, but driver has error, return error
        summary_poll_status = -1;
      }
    }
    if (summary_poll_status < 1) {
      state_ = kTransactionComplete;
    }
  }
  return summary_poll_status;
}

uint8_t BroadcastHandle::getMotorId() {
  return 0xff;
}
uint8_t BroadcastHandle::getMotorStatus() {
  return 0xff;
}
uint8_t BroadcastHandle::readByte() {
  return 0xff;
}
uint8_t BroadcastHandle::readWord() {
  return 0xffff;
}

BroadcastHandle::~BroadcastHandle() {
  delete[] handles_;
}

//////////////////////////////////////////////////////////////////////////
/// GenericHandle
//////////////////////////////////////////////////////////////////////////
GenericHandle::GenericHandle(MotorHandleFactory* p_motors, uint8_t id, uint8_t ins)
: id_(id), ins_(ins)
{
  if (id_ == DxlProtocolV1::kBroadcastId) {
    if (p_motors->aquireLockAll()) {
      handle_ = new BroadcastHandle{p_motors_, ins};
      p_motors_ = p_motors;
    }          
  } else {
    if (p_motors->aquireLock(p_motors->getDriverIndex(id))) {
      handle_ = new SingleHandle{p_motors, p_motors->getDxlDriver(), id, ins};
    }
  }
}

GenericHandle::~GenericHandle() {
  close();
  if (handle_ != nullptr) {
    delete handle_;
  }
}

bool GenericHandle::writeByte(uint8_t value) {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->writeByte(value) : false;
}

bool GenericHandle::writeWord(uint16_t value) {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->writeWord(value) : false;
}

bool GenericHandle::startTransmission() {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->startTransmission() : false;
}

bool GenericHandle::poll() {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->poll() : false;
}

uint8_t GenericHandle::getMotorId() {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->getMotorId() : 0xff;
}

uint8_t GenericHandle::getMotorStatus() {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->getMotorStatus() : 0x80;
}

uint8_t GenericHandle::readByte() {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->readByte() : 0xff;
}

uint8_t GenericHandle::readWord() {
  return (p_motors_ != nullptr && handle_ != nullptr) handle_->readWord() : 0xffff;
}

bool GenericHandle::close() {
  if (id_ == DxlProtocolV1::kBroadcastId) {
    
    if (p_motors_ != nullptr) {
      // TODO: some cleanup
      p_motors_->releaseLockAll();    
    }
  } else {
    if (p_motors_ != nullptr) {
      // TODO: some cleanup
      p_motors_->releaseLock(p_motors_->getDriverIndex(id_));
    }
  }
  p_motors_ = nullptr;
  state_ = kClosed;
}

//////////////////////////////////////////////////////////////////////////
/// SyncWriteHandle
//////////////////////////////////////////////////////////////////////////
SyncWriteHandle::SyncWriteHandle(MotorHandleFactory* p_motor_driver, uint8_t target_register, uint8_t n_bytes) 
: state_(kInitial), target_register_(target_register), n_bytes_(n_bytes), p_motors_(p_motor_driver) { }

SyncWriteHandle::~SyncWriteHandle() {
  close();
}

bool SyncWriteHandle::toMotor(uint8_t id) {
  bool success = false;
  uint8_t driver_idx = 0xff;
  if (state_ != kInitial) {
    // out of sequence call
  } else if (p_motors_ != nullptr) {
    driver_idx = p_motors_->getDriverIndex(id);
  }
  
  if (id >= MotorHandleFactory::kMaxMotors ) {
    // not a valid id!
  } else if (written_ != 0 && written_ != n_bytes_) {
    // incorrectly written length! 
  } else if (driver_idx < p_motors_->n_drivers_) {
    p_current_dxl_ = p_motors_->p_drivers_[driver_idx];
    if (!initialized_[driver_idx]) {
      // not yet initialize this particular driver;
      p_current_dxl_->setTxIns(DxlProtocolV1::Ins::kBroadcastId, DxlProtocolV1::Ins::kSyncWrite);
      p_current_dxl_->writeTxByte(target_register_);
      p_current_dxl_->writeTxByte(n_bytes_);
      initialized_[driver_idx] = true;    
    }
    success = p_current_dxl_->writeTxByte(id);
    written_ = 0;
  }
  if (!success) {
    close();
  }
  return success;
}

bool SyncWriteHandle::writeByte(uint8_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ == nullptr) {
    // invalid
  } else if (written_ < n_bytes_) {
    success = p_current_dxl_->writeTxByte(value);
  }
  if (!success) {
    close();
  }
  return success;
}

bool SyncWriteHandle::writeWord(uint16_t value) {
  bool success = false;
  if (state_ != kInitial) {
  // out of sequence call, return default value false
  } else if (p_current_dxl_ == nullptr) {
  // invalid
  } else if (written_ < n_bytes_ && (written_ + 1) < n_bytes_) {
    success = p_current_dxl_->writeTxWord(value);
  }
  if (!success) {
    close();
  }
  return success;
}

bool SyncWriteHandle::startTransmission() {
  bool started = false;
  uint8_t i;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ == nullptr) {
    // invalid
  }else if (written_ != n_bytes_) {
    // something incomplete
  } else {
    p_current_dxl_ = nullptr;
    for (i=(uint8_t)0; i<p_motors_->n_drivers_; i++) {
      if (initialized_[i]) {
        started = started && p_motors_->p_drivers_[i]->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
      }
    }  
  }
  if (started) {
    state_ = kInTransit;
  } else {
    close();
  }
  return started;
}

bool SyncWriteHandle::poll() {
  DxlDriver::Status status;
  uint8_t i;
  bool is_done;
  if (state_ == kTransactionComplete) {
    is_done = true;
  } else if (state_ != kInTransit) {
    is_done = false;
    // out of sequence call, return default value false
  } else {
    is_done = true;
    for (i=(uint8_t)0; i<p_motors_->n_drivers_; i++) {
      if (initialized_[i]) {
        // sync-write has no reply, timeout is the expected result
        status = p_motors_->p_drivers_[i]->poll();
        switch(status) {
          case DxlDriver::kTransmitting:
          case DxlDriver::kReceiving:
            is_done = false;  // not done yet if any of the drivers and still sending/receiving
            break;
          default:
            break;
        }
      }
    }
  }

  if (is_done) {
    state_ = kTransactionComplete;
  }  
  return is_done;
}

bool SyncWriteHandle::close() {
  if (p_motors_ != nullptr) {
    if (state_ != kTransactionComplete) {
      // TODO: some cleanup should be done.
    }
    p_motors_->driver_lock_ = false;
  }
  p_current_dxl_ = nullptr;
  p_motors_ = nullptr;
  state_ = kClosed;
}





FeedbackHandle::FeedbackHandle(MotorHandleFactory* p_motor_driver) 
: combined_state_(kInitial), 
p_motors_(p_motor_driver),
{
  uint8_t id,;
  
  // initialize memory
  for (id=0; id<MotorHandleFactory::kMaxMotors; id++) {
    driver = p_motors_->p_id_mappings_[id];
    p_motors_->p_feedbacks_[id] = MotorFeedbackData{};
  }
  memset(p_driver_states_, kInitial, sizeof(decltype(*p_driver_states_) * MotorHandleFactory::kMaxDrivers));
  memset(p_reqeust_id_, 0xff, sizeof(decltype(*p_reqeust_id_) * MotorHandleFactory::kMaxDrivers));
  memset(p_retries_)
}

FeedbackHandle::~FeedbackHandle() {
  close();
}

bool FeedbackHandle::readAllMotors() {
  bool is_done = false;
  DxlDriver* driver;
  uint8_t i, motor_id;
  uint8_t driver_idx;
  bool loop_done;
  
  // Some temporary data
  uint8_t p_request_id_[MotorHandleFactory::kMaxDrivers];
  uint8_t p_retries_[MotorHandleFactory::kMaxMotors];

  for (driver_idx=0; driver_idx<MotorHandleFactory::kMaxDrivers; driver_idx++) {
    p_request_id_[driver_idx] = 0xff;
    p_driver_states_[driver_idx] = kInitial;
  }
  if (p_motors_ == nullptr) {
    // invalid handle
  } else if (combined_state_ != kInitial) {
    // out of sequence call, return default value false
  } else {
    for (motor_id=0; motor_id<MotorHandleFactory::kMaxMotors; motor_id++) {
      /// clear the feedback data
      p_motors_->p_feedbacks_[motor_id] = MotorFeedbackData{};
    }
    while (1) {
      loop_done = true;
      /// Trigger request for each Motor
      for (motor_id=0; motor_id<MotorHandleFactory::kMaxMotors; motor_id++) {
        if (p_retries_[motor_id] >= kMaxRetry) {
          continue;  // already obtained valid data, do nothing
        } 
        driver_idx = p_motors_->getDriverIndex(motor_id);
        loop_done = false; // getting here means there are unread motors
        if (p_driver_states_[driver_idx] == kInTransit) {
          continue;  // driver is busy, try again later
        }
        /// Begin read request
        driver = p_motors_->p_drivers_[driver_idx];
        driver->setTxIns(motor_id, DxlProtocolV1::Ins::kRead);
        driver->writeTxByte(kFirstReg);
        driver->writeTxByte(kByteSize);
        driver->beginTransmission();
        p_driver_states_[driver_idx] = kInTransit;
        p_request_id_[driver_idx] = motor_id;
        p_retries_[motor_id]++;
      }
      /// Poll each driver
      for (driver_idx=0; driver_idx<p_motors_->n_drivers_; driver_idx++) {
        if (p_driver_states_[driver_idx] != kInTransit) {
          continue;  // not in transit, no need to poll
        }
        loop_done = false; // getting here means there are busy drivers
        driver = p_motors_->p_drivers_[driver_idx];
        auto driver_state = driver->poll();
        switch(driver_state) {
          case DxlDriver::Status::kDone:
            p_driver_states_[driver_idx] = kInitial;
            motor_id = driver->getRxId();
            if (motor_id != p_request_id_[driver_idx]) {
              break;  // some error on the reply data!
            }
            p_motors_->p_feedbacks_[motor_id].status = driver->getRxStatusByte();
            p_motors_->p_feedbacks_[motor_id].position = driver->readRxWord();
            p_motors_->p_feedbacks_[motor_id].speed = driver->readRxWord();
            p_motors_->p_feedbacks_[motor_id].torque = driver->readRxWord();
            p_motors_->p_feedbacks_[motor_id].valid = true;
            break;
          case DxlDriver::Status::kErrorInvalidReceiveData:
          case DxlDriver::Status::kErrorInvalidTransmitData:
          case DxlDriver::Status::kErrorTimeout:
            p_driver_states_[driver_idx] = kInitial;
            p_motors_->p_feedbacks_[p_request_id_[driver_idx]].status = 0x80;
            p_motors_->p_feedbacks_[motor_id].valid = true;
            break;
          default: {}
        }
      }
      if (loop_done) {
        break;  // break out of the for loop
      }
    }
    combined_state_ = kTransactionComplete;
  }
  return is_done;
}
MotorFeedbackData FeedbackHandle::getFeedback(uint8_t id) {
  MotorFeedbackData feedback{};  // return empty data by default
  if (p_motors_ == nullptr) {
    // invalid handle
  } else if (combined_state_ != kTransactionComplete) {
    // out of sequence call
  } else if (id >= MotorHandleFactory::kMaxMotors) {
    // invalid id
  } else {  
    feedback = p_motors_->p_feedbacks_[id];
  }
  return feedback;
}

bool FeedbackHandle::close() {
  uint8_t i;
  if (p_motors_ != nullptr) {
    for (i=(uint8_t)0; i<MotorHandleFactory::kMaxDrivers; i++) {
      if (p_driver_states_[i] == kInTransit) {
        // TODO: some cleanup should be done.
      }
    }
    p_motors_->driver_lock_ = false;  
  }
  p_motors_ = nullptr;
  combined_state_ = kClosed;
}
  

}  // namespace motor_handles