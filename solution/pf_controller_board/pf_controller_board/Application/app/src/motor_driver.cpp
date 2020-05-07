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
  p_request_id_ = new uint8_t[kMaxDrivers];
  p_retries_ = new uint8_t[kMaxDrivers];
  p_handles_ = new motor_handles::SingleHandle[kMaxDrivers];

  for (i=0; i<kMaxMotors; i++) {
    p_id_mappings_[i] = 0xff;
  }
  for (i=0; i<kMaxDrivers; i++) {
    p_drivers_[i] = nullptr;
    driver_lock_[i] = false;
  }
}

MotorHandleFactory::~MotorHandleFactory() {
  delete[] p_id_mappings_;
  delete[] p_drivers_;
  delete[] p_request_id_;
  delete[] p_retries_;
  delete[] p_handles_;
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

motor_handles::GenericHandle MotorHandleFactory::createWriteHandle(uint8_t id) {
  return motor_handles::GenericHandle{this, id, DxlProtocolV1::Ins::kWrite};
}

motor_handles::SyncWriteHandle MotorHandleFactory::createSyncWriteHandle(uint8_t target_register, uint8_t number_of_bytes) {
  return motor_handles::SyncWriteHandle(this, target_register, number_of_bytes);
}


uint8_t MotorHandleFactory::countDrivers() {
  return n_drivers_;
}

void MotorHandleFactory::MappingQueue::build(uint8_t* mapping) {
  uint8_t i, j;
  for (i=0; i <kMaxDrivers; i++) {
    if (i==0) {
      start_[i] = 0;
    } else {
      start_[i] = end_[i - 1];
    }
    end_[i] = start_[i];
    for(j=0; j<kMaxMotors; j++) {
      if (i==mapping[j]) {
        motor_ids_[end_[i]] = mapping[j];
        end_[i]++;
      }
    }
  }
}

uint8_t MotorHandleFactory::MappingQueue::pop(uint8_t index) {
  // unsafe
  return motor_ids_[start_[index]++];
}

bool MotorHandleFactory::MappingQueue::hasItem(uint8_t index) {
  // unsafe
  return start_[index] < end_[index];
}


void MotorHandleFactory::MappingQueue::rewind() {
  uint8_t i;
  for (i=0; i<kMaxDrivers; i++) {
    if (i==0) {
      start_[i] = 0;
    } else {
      start_[i] = end_[i-1];
    }
  }
}

bool MotorHandleFactory::readAllMotors(MotorDataInterface& datas) {
  if (!aquireLockAll()) {
    return false;
  }
  DxlDriver* driver;
  uint8_t i, motor_id;
  uint8_t driver_idx;
  uint8_t reg = datas.requestedRegister();
  uint8_t len = datas.requestedLength();
  uint8_t max_retries = datas.requestRetries();
  int result;
  bool loop_done;
  
  /// Data initialization
  for (driver_idx=0; driver_idx<MotorHandleFactory::kMaxDrivers; driver_idx++) {
    p_request_id_[driver_idx] = 0xff;
    p_retries_[motor_id] = max_retries;
  }
  datas.initializeDatas();
  queue_.build(p_id_mappings_);
  /// Request/update loop
  while (1) {
    loop_done = true;
      
    for (driver_idx=0; driver_idx<n_drivers_; driver_idx++) {
      if (p_handles_[driver_idx]) {
        continue;  // driver is still busy, skip
      }
      if(p_retries_[driver_idx] >= max_retries && queue_.hasItem(driver_idx)) {
        p_request_id_[driver_idx] = queue_.pop(driver_idx);
        p_retries_[motor_id] = 0;
      }
      if (p_retries_[driver_idx] < max_retries) {
        driver = getDriverPtrById(motor_id);
        p_handles_[driver_idx] = motor_handles::SingleHandle(driver);
        p_handles_[driver_idx].setInstruction(motor_id, DxlProtocolV1::Ins::kRead);
        p_handles_[driver_idx].writeByte(reg);
        p_handles_[driver_idx].writeByte(len);
        p_handles_[driver_idx].startTransmission();
        p_request_id_[driver_idx] = motor_id;
        p_retries_[motor_id]++;
          
        loop_done = false;
      }
    }
    for (driver_idx=0; driver_idx<n_drivers_; driver_idx++) {
      if (!p_handles_[driver_idx]) {  // idle driver, skip
        continue;
      }
      result = p_handles_[driver_idx].poll();
      if (result > 0) {  // still busy, skip
        loop_done = false;
        continue;
      }
      if (result == 0) {  // successful reply
        if (p_request_id_[driver_idx] == p_handles_[driver_idx].getMotorId()) {
          datas.replyFromHandle(p_handles_[driver_idx], p_request_id_[driver_idx]);
          p_retries_[driver_idx] = max_retries;
          } else {  // mismatch is an error
          datas.errorFromHandle(p_request_id_[driver_idx]);
        }
        } else if (result < 0) {  // all other errors
        datas.errorFromHandle(p_request_id_[driver_idx]);
      }
      p_handles_[driver_idx] = motor_handles::SingleHandle{};  // set to empty handle after processed
    }
    if (loop_done) {
      break;  // break out of the for loop
    }
  }
  releaseLockAll();
  return true;
}


namespace motor_handles {


enum TransactionState : uint8_t {
  kUnitiliaized = 0xff,
  kInitial = 0,
  kInTransit,
  kTransactionComplete,
  kClosed,
  kUnknown,
};

//////////////////////////////////////////////////////////////////////////
/// SINGLE TRANSACTION HANDLE
//////////////////////////////////////////////////////////////////////////

SingleHandle::SingleHandle()
: BaseHandle(kClosed), p_driver_(nullptr) {
}

SingleHandle::SingleHandle(DxlDriver* p_driver)
: BaseHandle(kUnitiliaized), p_driver_(p_driver) {
  if (p_driver == nullptr) {
    setState(kClosed);
  }
}


bool SingleHandle::setInstruction(uint8_t id, uint8_t ins) {
  bool success = false;
  if (p_driver_ != nullptr && (state_ == kUnitiliaized || state_ == kTransactionComplete)) {
    success = true;
    p_driver_->setTxIns(id, ins);
    state_ = kInitial;
  } else {
    close();
  }
  return success;
}

bool SingleHandle::writeByte(uint8_t value) {
  bool success = false;
  if (checkState(kInitial)) {
    success = p_driver_->writeTxByte(value);
    if (!success) {
      close();
    }
  }
  return success;
}

bool SingleHandle::writeWord(uint16_t value) {
  bool success = false;
  if (checkState(kInitial)) {
    success = p_driver_->writeTxWord(value);
    if (!success) {
      close();
    }
  }
  return success;
}

bool SingleHandle::startTransmission() {
  bool started = false;
  uint8_t i;
  if (checkState(kInitial)) {
    started = p_driver_->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
    if (started) {
      state_ = kInTransit;
      } else {
      close();
    }
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
    result == memoized_result_;
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
    if (result < 1) {
      memoized_result_ = result;
      state_ = kTransactionComplete;
    }
  }
  return result;
}

uint8_t SingleHandle::getMotorId() {
  return (checkState(kTransactionComplete)) ? p_driver_->getRxId() : 0xff;
}

uint8_t SingleHandle::getMotorStatus() {
  return (checkState(kTransactionComplete)) ? p_driver_->getRxStatusByte() : 0xff;
}

uint8_t SingleHandle::readByte() {
  return (checkState(kTransactionComplete)) ? p_driver_->readRxByte() : 0xff;
}

uint8_t SingleHandle::readWord() {
  return (checkState(kTransactionComplete)) ? p_driver_->readRxWord() : 0xffff;
}

void SingleHandle::close() {
  if (p_driver_ != nullptr && state_ != kClosed) {
    // TODO: cleanup
  }
  p_driver_ = nullptr;
  state_ = kClosed;
}

bool SingleHandle::checkState(uint8_t expected_state) {
  bool state_ok = false;
  if (p_driver_ != nullptr && state_ == expected_state) {
    state_ok = true;
  } else {
    close();
  }
}


//////////////////////////////////////////////////////////////////////////
/// BroadcasTransactiontHandle
//////////////////////////////////////////////////////////////////////////
BroadcastHandle::BroadcastHandle()
: BaseHandle(kClosed)
{}

BroadcastHandle::BroadcastHandle(uint8_t n_drivers, DxlDriver** p_drivers)
: BaseHandle(kUnitiliaized), n_handles_(n_drivers)
{
  int i;
  p_handles_ = new SingleHandle[n_drivers];
  for (i=0; i<n_handles_; i++) {
    if (p_drivers == nullptr) {
      p_handles_[i] = SingleHandle{};
    } else {
      p_handles_[i] = SingleHandle{p_drivers[i]};
    }     
  }
}


BroadcastHandle::~BroadcastHandle() {
  close();
  if (p_handles_ != nullptr) {
    delete[] p_handles_;
    p_handles_ = nullptr;    
  }
}

bool BroadcastHandle::setInstruction(uint8_t id, uint8_t ins) {
  int i;
  bool success = false;
  if (p_handles_ != nullptr &&(state_ == kUnitiliaized || state_ == kTransactionComplete)) {
    success = true;
    for (i=0; i<n_handles_; i++) {
      if (p_handles_[i]) {
        success = (p_handles_[i].setInstruction(id, ins)) ? success : false;
      }        
    }
  }
  if (success) {
    setState(kInitial);
  } else {
    close();
  }
  return success;
}

bool BroadcastHandle::writeByte(uint8_t value) {
  int i;
  bool success = false;
  if (checkState(kInitial)) {
    success = true;
    for (i=0; i<n_handles_; i++) {
      if (p_handles_[i])
      {
        success = success && p_handles_[i].writeByte(value);
      }
    }
  }
  if (!success) {
    close();
  }
  return success;
}


bool BroadcastHandle::writeWord(uint16_t value) {
  int i;
  bool success = false;
  if (checkState(kInitial)) {
    success = true;
    for (i=0; i<n_handles_; i++) {
      if (p_handles_[i])
      {
        success = success && p_handles_[i].writeWord(value);
      }
    }
  }
  if (!success) {
    close();
  }
  return success;
}

bool BroadcastHandle::startTransmission() {
  int i;
  bool success = false;
  if (checkState(kInitial)) {
    success = true;
    for (i=0; i<n_handles_; i++) {
      if (p_handles_[i]) {
        success = success && p_handles_[i].startTransmission();
      }
    }
  }
  if (success) {
    state_ = kInTransit;
    } else {
    close();
  }
  return success;
}

int BroadcastHandle::poll() {
  int i;
  int result;
  int summary_result = -1;
  if (p_handles_ == nullptr) {
  // error
  } else if (state_ == kTransactionComplete) {
    // already done, no need to repeat
    summary_result == memoized_result_;
    // return     
  } else if (state_ != kInTransit) {
    // error
  } else {
    summary_result = 0;
    for (i=0; i<n_handles_; i++) {
      if (!p_handles_[i]) {
        continue;
      }
      result = p_handles_[i].poll();
      if (result > 0 || summary_result > 0) {
        // if any driver is still busy, give a busy status 
        summary_result = 1;
      } else if (result < 0) {
        // if no driver busy, but driver has error, return error
        summary_result = -1;
      }
    }
    if (summary_result < 1) {
      memoized_result_ = summary_result;
      state_ = kTransactionComplete;
    }    
  }
  return summary_result;
}

void BroadcastHandle::close() {
  int i;
  if (state_ != kClosed) {
    for (i=0; i<n_handles_; i++) {
      p_handles_[i].close();
    }
  }
  state_ = kClosed;
}

uint8_t BroadcastHandle::getState() {
  bool first_state = true;
  uint8_t i;
  uint8_t running_state = kUnknown;  // mismatched states
  for (i=0; i<n_handles_; i++) {
    if (p_handles_[i]) {
      if (first_state) {
        first_state = false;
        running_state = p_handles_[i].getState();
      } else {
        if (p_handles_[i].getState() != running_state) {
          running_state = kUnknown;
          break;
        }
      }
    }
  }
  return running_state;
};

bool BroadcastHandle::checkState(uint8_t expected_state) {
  bool state_ok = false;
  if (p_handles_ != nullptr && state_ == expected_state) {
    state_ok = true;
  } else {
    close();
  }
  return state_ok;
}



//////////////////////////////////////////////////////////////////////////
/// GenericHandle
//////////////////////////////////////////////////////////////////////////
GenericHandle::GenericHandle(MotorHandleFactory* p_motors, uint8_t id, uint8_t ins)
: BaseHandle(kInitial), id_(id), ins_(ins) {
  int i;
  handle_ = nullptr;
  if (id_ == DxlProtocolV1::kBroadcastId) {
    if (p_motors->aquireLockAll()) {
      handle_ = new BroadcastHandle{MotorHandleFactory::kMaxDrivers, p_motors->getDriverPtrArray()};
      handle_->setInstruction(id, ins);
    }
  } else {
    DxlDriver* driver = p_motors->getDriverPtrById(id);
    if (driver != nullptr) {
      if (p_motors->aquireLock(p_motors->getDriverIndex(id))) {
        handle_ = new SingleHandle{driver};
        handle_->setInstruction(id, ins);
      }
    }      
  }
  if (handle_ != nullptr) {
    p_motors_ = p_motors;
  }
}

GenericHandle::~GenericHandle() {
  close();
  if (handle_ != nullptr) {
    delete handle_;
    handle_ = nullptr;
  }
}

bool GenericHandle::setInstruction(uint8_t id, uint8_t ins) {
  bool success = false; 
  if (p_motors_ != nullptr && handle_ != nullptr
      && (handle_->getState() == kUnitiliaized || handle_->getState() == kTransactionComplete)) {
    success = handle_->setInstruction(id, ins);
  }    
  return success;
}

bool GenericHandle::writeByte(uint8_t value) {
  if (p_motors_ != nullptr && handle_ != nullptr) {
    if(handle_->getState() == kTransactionComplete) {
      handle_->setInstruction(id_, ins_);
    } 
  }
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->writeByte(value) : false;
}

bool GenericHandle::writeWord(uint16_t value) {
  if (p_motors_ != nullptr && handle_ != nullptr) {
    if(handle_->getState() == kTransactionComplete) {
      handle_->setInstruction(id_, ins_);
    }
  }
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->writeWord(value) : false;
}

bool GenericHandle::startTransmission() {
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->startTransmission() : false;
}

int GenericHandle::poll() {
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->poll() : -1;
}

uint8_t GenericHandle::getMotorId() {
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->getMotorId() : 0xff;
}

uint8_t GenericHandle::getMotorStatus() {
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->getMotorStatus() : 0xff;
}

uint8_t GenericHandle::readByte() {
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->readByte() : 0xff;
}

uint8_t GenericHandle::readWord() {
  return (p_motors_ != nullptr && handle_ != nullptr) ? handle_->readWord() : 0xffff;
}

void GenericHandle::close() {
  if (handle_ != nullptr) {
    if (*handle_) {
      handle_->close();
    }    
  }
  if (p_motors_ != nullptr) {
    if (id_ == DxlProtocolV1::kBroadcastId) {
        p_motors_->releaseLockAll();
    } else {
        p_motors_->releaseLock(p_motors_->getDriverIndex(id_));
    }
  }  
  p_motors_ = nullptr;
}


//////////////////////////////////////////////////////////////////////////
/// SyncWriteHandle
//////////////////////////////////////////////////////////////////////////
SyncWriteHandle::SyncWriteHandle(MotorHandleFactory* p_motor_driver, uint8_t target_register, uint8_t n_bytes) 
: BroadcastHandle(MotorHandleFactory::kMaxDrivers), target_register_(target_register), n_bytes_(n_bytes) {
  if(p_motors_->aquireLockAll()) {
    p_motors_ = p_motor_driver;
  }
}

SyncWriteHandle::~SyncWriteHandle() {
  close();
  if (p_handles_ != nullptr) {
    delete[] p_handles_;
    p_handles_ = nullptr;
  }
}

bool SyncWriteHandle::toMotor(uint8_t id) {
  bool success = false;
  DxlDriver* driver = nullptr;
  uint8_t driver_idx;
  SingleHandle* handle;
  if (state_ != kInitial) {
    // out of sequence call
  } else if (current_handle_ != nullptr) {
    // mismatched number of bytes
  } else if (p_motors_ != nullptr) {
    driver = p_motors_->getDriverPtrById(id);
    driver_idx = p_motors_->getDriverIndex(id);
  }
  if (driver != nullptr) {
    if (!p_handles_[driver_idx]) {
      // handle for this driver has not been created, try to create a new handle
      p_handles_[driver_idx] = SingleHandle{driver};
    }
    current_handle_ = &p_handles_[driver_idx];
    current_handle_->setInstruction(DxlProtocolV1::Ins::kBroadcastId, DxlProtocolV1::Ins::kSyncWrite);
    current_handle_->writeByte(target_register_);
    current_handle_->writeByte(n_bytes_);
    success = true;
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
  } else if (current_handle_ == nullptr) {
    // invalid
  } else if (written_ < n_bytes_) {
    success = current_handle_->writeByte(value);
    written_ += 1;
    if (written_ == n_bytes_) {  // completed write for this motor
      current_handle_ = nullptr;
    }
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
  } else if (current_handle_ == nullptr) {
  // invalid
  } else if (written_ < n_bytes_ && (written_ + 1) < n_bytes_) {
    success = current_handle_->writeWord(value);
    written_ += 2;
    if (written_ == n_bytes_) {  // completed write for this motor
      current_handle_ = nullptr;
    }
  }
  if (!success) {
    close();
  }
  return success;
}

void SyncWriteHandle::close() {
  BroadcastHandle::close();
  if (p_motors_ != nullptr) {
      p_motors_->releaseLockAll();
  }
}




}  // namespace motor_handles