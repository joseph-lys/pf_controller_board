/*
 * motor_driver.cpp
 *
 * Created: 6/4/2020 7:51:57 PM
 *  Author: josep
 */ 

#include "motor_driver.h"
#include "DxlProtocolV1.h"

typedef unsigned int uint;

//////////////////////////////////////////////////////////////////////////
/// MotorDriver
//////////////////////////////////////////////////////////////////////////
void MotorDriver::init() {
  uint8_t i, id;
  for (id=0u; id<kMaxMotors; id++) {
    for (i= 0; i<3; i++) {  // multiple tries to avoid missing motors
      if(pingMotor(id)) {
        break;
      }
    }
  }
}

bool MotorDriver::pingMotor(uint8_t id) {
  bool is_found = false;
  uint i;
  DxlDriver* driver;
  DxlDriver::Status status;
  if (id < static_cast<uint>(kMaxMotors)) {
    for (i=0u; i<static_cast<uint>(n_drivers_); i++) {
      driver = drivers_[i];
      driver->setTxIns(id, DxlProtocolV1::Ins::kPing);
      driver->beginTransmission();
      for(;;) {
        status = driver->poll();
        if (status == DxlDriver::kErrorInvalidReceiveData
            || status == DxlDriver::kErrorInvalidTransmitData 
            || status == DxlDriver::kErrorTimeout) {
          is_found = false;
          break;
        }
        if (status == DxlDriver::kDone) {
          is_found = true;
          motor_mapping[id] = i;  // assign the index of the driver
          break;
        }
      }
    }
  }
  return is_found;
}

motor_handles::SyncWriteHandle MotorDriver::createBroadcastHandle() {
  motor_handles::SyncWriteHandle handle;
  if (!driver_lock_) {
    driver_lock_ = true;
    handle = motor_handles::SyncWriteHandle(this);
  }
  return handle;
}

motor_handles::GenericHandle MotorDriver::createWriteHandle(uint8_t id) {
  motor_handles::GenericHandle handle;
  if (!driver_lock_) {
    driver_lock_ = true;
    handle = motor_handles::GenericHandle(this, id);
    handle.setInstruction(DxlProtocolV1::Ins::kWrite);  // specialize to a write type
  }
  return handle;
}

motor_handles::FeedbackHandle MotorDriver::createFeedbackHandle() {
  motor_handles::FeedbackHandle handle;
  if (!driver_lock_) {
    driver_lock_ = true;
    handle = motor_handles::FeedbackHandle();
  }
  return handle;
}



namespace motor_handles {

enum TransactionState : uint8_t {
  kUninitialized = 0xff,
  kInitial = 0,
  kInTransit,
  kTransactionComplete,
  kClosed,
};

//////////////////////////////////////////////////////////////////////////
/// SyncWriteHandle
//////////////////////////////////////////////////////////////////////////
SyncWriteHandle::SyncWriteHandle(MotorDriver* p_motor_driver) 
: state_(kInitial), p_motors_(p_motor_driver) { }

SyncWriteHandle::~SyncWriteHandle() {
  close();
}

bool SyncWriteHandle::toMotor(uint8_t id) {
  bool success = false;
  uint8_t driver_idx = p_motors_->getDriverIndex(id);
  if (state_ != kInitial) {
    // out of sequence call
  } else if (id == DxlProtocolV1::kBroadcastId) {
    // not a valid id!
  } else if (driver_idx < p_motors_->n_drivers_) {
    p_current_dxl_ = p_motors_->drivers_[driver_idx];
    if (!initialized_[driver_idx]) {
      // not yet initialize this particular driver;
      p_current_dxl_->setTxIns(DxlProtocolV1::Ins::kBroadcastId, DxlProtocolV1::Ins::kSyncWrite);
      initialized_[driver_idx] = true;    
    }
    success = p_current_dxl_->writeTxByte(id);
    state_ = kInTransit;
  }
  return success;
}

bool SyncWriteHandle::writeByte(uint8_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ != nullptr) {
    success = p_current_dxl_->writeTxByte(value);
  }
  return success;
}

bool SyncWriteHandle::writeWord(uint16_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ != nullptr) {
    success = p_current_dxl_->writeTxWord(value);
  }
  return success;
}

bool SyncWriteHandle::startTransmission() {
  bool started = false;
  uint8_t i;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ == nullptr) {
    // out of sequence call, no motor was every used
  } else {
    p_current_dxl_ = nullptr;
    for (i=(uint8_t)0; i<p_motors_->n_drivers_; i++) {
      if (initialized_[i]) {
        started = started && p_motors_->drivers_[i]->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
      }
    }  
  }
  if (started) {
    state_ = kInTransit;
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
        status = p_motors_->drivers_[i]->poll();
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
  if (state_ != kTransactionComplete) {
    // TODO: some cleanup should be done.
  }
  state_ = kClosed;
  p_motors_->driver_lock_ = false;
  p_current_dxl_ = nullptr;
}


//////////////////////////////////////////////////////////////////////////
/// GENERIC HANDLE
//////////////////////////////////////////////////////////////////////////
GenericHandle::GenericHandle(MotorDriver* p_motor_driver, uint8_t id)
: state_(kUninitialized), id_(id), p_motors_(p_motor_driver) { 
}

GenericHandle::~GenericHandle() {
  close();
}

bool GenericHandle::setInstruction(uint8_t ins) {
  bool initialized = false;
  uint8_t driver_idx;
  if (state_ != kUninitialized) {
    // out of sequence call, return default value false
  } else if (id_ == DxlProtocolV1::kBroadcastId) {
    // case for broadcast
    
  } else {
    // case for targeted
    p_current_dxl_ = p_motors_->getDxlDriver(id_);
    if (p_current_dxl_ != nullptr) {
      p_current_dxl_->setTxIns(id_, ins);
      initialized = true;
      state_ = kInitial;
    }
  }
  return initialized;
}

bool GenericHandle::writeByte(uint8_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ != nullptr) {
    success = p_current_dxl_->writeTxByte(value);
  }
  return success;
}

bool GenericHandle::writeWord(uint16_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
    } else if (p_current_dxl_ != nullptr) {
    success = p_current_dxl_->writeTxWord(value);
  }
  return success;  
}

bool GenericHandle::startTransmission() {
  bool started = false;
  uint8_t i;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (id_ == DxlProtocolV1::kBroadcastId) {
    // broadcast condition
    started = true;
    for (i=(uint8_t)0; i<p_motors_->n_drivers_; i++) {
      started = started && p_motors_->drivers_[i]->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
    }
  } else if (p_current_dxl_ != nullptr) {
    // non-broadcast condition
    started = p_current_dxl_->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
  }

  if (started) {
    state_ = kInTransit;
  }
  return started;
}
bool GenericHandle::poll() {
  DxlDriver::Status status;
  uint8_t i;
  bool is_done;
  if (state_ == kTransactionComplete) {
    is_done = true;
  } else if (state_ != kInTransit) {
  is_done = false;
    // out of sequence call, return default value false
  } else if (id_ == DxlProtocolV1::kBroadcastId) {
    is_done = true;
    for (i=(uint8_t)0; i<p_motors_->n_drivers_; i++) {
      // broadcast has no reply, timeout is the expected result
      status = p_motors_->drivers_[i]->poll();
      switch(status) {
        case DxlDriver::kTransmitting:
        case DxlDriver::kReceiving:
        is_done = false;  // not done yet if any of the drivers and still sending/receiving
        break;
        default:
        break;
      }
    }
  } else if (p_current_dxl_ != nullptr) {
    // non-broadcast condition
    status = p_current_dxl_->poll();
    switch(status) {
      case DxlDriver::kReceiving:
      case DxlDriver::kTransmitting:
        is_done = false;
        break;
      default:
        is_done = true;
    }
  }
  if (is_done) {
    state_ = kTransactionComplete;
  }
  return is_done;
}
uint8_t GenericHandle::getMotorId() {
  return (state_ == kTransactionComplete) ? p_current_dxl_->getRxId() : 0xff;
}
uint8_t GenericHandle::getMotorStatus() {
  return (state_ == kTransactionComplete) ? p_current_dxl_->getRxStatusByte() : 0xff;
}

uint8_t GenericHandle::readByte() {
  return (state_ == kTransactionComplete) ? p_current_dxl_->readRxByte() : 0xff;
}
uint8_t GenericHandle::readWord() {
  return (state_ == kTransactionComplete) ? p_current_dxl_->readRxWord() : 0xff;
}

bool GenericHandle::close() {
  if (state_ != kTransactionComplete) {
    // TODO: some cleanup should be done.
  } 
  state_ = kClosed;
  p_motors_->driver_lock_ = false;
  p_current_dxl_ = nullptr;
}


FeedbackHandle::FeedbackHandle(MotorDriver* p_motor_driver) 
: combined_state_(kInitial), p_motors_(p_motor_driver) {
  for (uint8_t id=(uint8_t)0; id<MotorDriver::kMaxMotors; id++) {
    p_motors_->feedback_[id] = MotorFeedbackData{};
  }      
}

FeedbackHandle::~FeedbackHandle() {
  close();
}

bool FeedbackHandle::readAllMotors() {
  bool is_done = false;
  DxlDriver* driver;
  uint8_t id;
  uint8_t idx;
  if (p_motors_ == nullptr) {
    // this is an error, return false
  } else if (combined_state_ != kInitial) {
    // out of sequence call, return default value false
  } else {
    while (1) {
      bool loop_done = true;
      /// Trigger request for each Motor
      for (id=(uint8_t)0; id<MotorDriver::kMaxMotors; id++) {
        if (p_motors_->feedback_[id].status == 0) {
          continue;  // already obtained valid data, do nothing
        } 
        loop_done = false; // getting here means there are unread motors
        
        idx = p_motors_->getDriverIndex(id);
        if (idx >= p_motors_->n_drivers_) {
          continue;  // not a valid id, do not do anything
        } 
        if (states_[idx] == kInTransit) {
          continue;  // in transit, do nothing
        }
        /// Begin read request
        driver = p_motors_->drivers_[idx];
        driver->setTxIns(id, DxlProtocolV1::Ins::kRead);
        driver->writeTxByte(kFirstReg);
        driver->writeTxByte(kByteSize);
        driver->beginTransmission();
        states_[idx] == kInTransit;
      }
      /// Poll each driver
      for (idx=0; idx<p_motors_->n_drivers_; idx++) {
        if (states_[idx] != kInTransit) {
          continue;  // not in transit, no need to poll
        }
        driver = p_motors_->drivers_[idx];
        switch(driver->poll()) {
          case DxlDriver::Status::kDone:
            states_[idx] = kInitial;
            id = driver->getRxId();
            if (id >= p_motors_->n_drivers_) {
              break;  // some error on the reply data!
            }
            p_motors_->feedback_[id].status = driver->getRxStatusByte();
            p_motors_->feedback_[id].position = driver->readRxWord();
            p_motors_->feedback_[id].speed = driver->readRxWord();
            p_motors_->feedback_[id].torque = driver->readRxWord();
            break;
          case DxlDriver::Status::kErrorInvalidReceiveData:
          case DxlDriver::Status::kErrorInvalidTransmitData:
          case DxlDriver::Status::kErrorTimeout:
            states_[idx] = kInitial;
            p_motors_->feedback_[id].status = 0x80;
            break;
          default:
            loop_done = false; // getting here means there are busy drivers
            break;
        }
        if (loop_done) {
          break;  // break out of the for loop
        }
      }
    }
    combined_state_ = kTransactionComplete;
  }
  return is_done;
}
MotorFeedbackData FeedbackHandle::getData(uint8_t id) {
  MotorFeedbackData feedback{};  // return empty data by default
  if (p_motors_ == nullptr) {
    // bad call
  } else if (combined_state_ != kTransactionComplete) {
    // out of sequence call
  } else if (id >= MotorDriver::kMaxMotors) {
    // invalid id
  } else {  
    feedback = p_motors_->feedback_[id];
  }
  return feedback;
}

bool FeedbackHandle::close() {
  uint8_t i;
  if (p_motors_ != nullptr) {
    for (i=(uint8_t)0; i<MotorDriver::kMaxDrivers; i++) {
      if (states_[i] == kInTransit) {
        // TODO: some cleanup should be done.
      }
    }
    p_motors_->driver_lock_ = false;
    p_motors_ = nullptr;
  }
  combined_state_ = kClosed;
}
  

}  // namespace motor_handles