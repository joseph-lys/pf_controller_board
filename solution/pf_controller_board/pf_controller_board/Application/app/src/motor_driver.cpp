/*
 * motor_driver.cpp
 *
 * Created: 6/4/2020 7:51:57 PM
 *  Author: josep
 */ 

#include "motor_driver.h"
#include "DxlProtocolV1.h"

static enum TransactionState : uint8_t {
  kUninitialized = 0xff,
  kInitial = 0,
  kInTransit,
  kTransactionComplete,
  kClosed,
};

MotorDriver::init() {
  uint8_t i, id;
  for (i=0; i<3; i++) {  // multiple tries to avoid missing motors
    for (id=0; id<kMaxMotors; id++) {
      pingMotor(id);
    }
  }
}

bool MotorDriver::pingMotor(uint8_t id) {
  bool is_found = false;
  uint8_t i;
  DxlDriver* driver;
  DxlDriver::Status status;
  if (id < kMaxMotors) {
    for (i=0; i<n_drivers; i++) {
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
  SyncWriteHandle handle{};
  if (!driver_lock_) {
    driver_lock_ = true;
    handle = SyncWriteHandle(this);
  }
  return handle;
}

motor_handles::GenericHandle MotorDriver::createWriteHandle(uint8_t id) {
  GenericHandle handle{};
  if (!driver_lock_) {
    driver_lock_ = true;
    handle = GenericHandle(this, id);
    handle.setInstruction(DxlProtocolV1::Ins::kWrite);
  }
  return handle;
}

motor_handles::FeedbackHandle MotorDriver::createFeedbackHandle(uint8_t reg_adr, uint8_t n_bytes) {
  
}

namespace motor_handles {
SyncWriteHandle::SyncWriteHandle(MotorDriver* p_motor_driver) 
: state_(kInitial), p_motors_(p_motor_driver) { }

SyncWriteHandle::~SyncWriteHandle() {
  close();
}

bool SyncWriteHandle::toMotor(uint8_t id) {
  bool success = false;
  uint8_t driver_idx = p_motors_->getDxlDriver();
  if (state_ != kInitial) {
    // out of sequence call
  } else if (id == DxlProtocolV1::kBroadcastId) {
    // not a valid id!
  } else if (driver_idx < p_motors_->n_drivers) {
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

void SyncWriteHandle::writeWord(uint16_t value) {
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
    for (i = 0; i<p_motors_->n_drivers; i++) {
      if (initialized_[i]) {
        started &&= p_motors_->drivers_[i]->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
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
    for (i=0; i<p_motors_->n_drivers; i++) {
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
  } else if (id_ = DxlProtocolV1::kBroadcastId) {
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

void GenericHandle::writeByte(uint8_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (p_current_dxl_ != nullptr) {
    success = p_current_dxl_->writeTxByte(value);
  }
  return success;
}

void GenericHandle::writeWord(uint16_t value) {
  bool success = false;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
    } else if (p_current_dxl_ != nullptr) {
    success = p_current_dxl_->writeTxWord(value);
  }
  return success;  
}

void GenericHandle::startTransmission() {
  bool started = false;
  uint8_t i;
  if (state_ != kInitial) {
    // out of sequence call, return default value false
  } else if (id_ = DxlProtocolV1::kBroadcastId) {
    // broadcast condition
    started = true;
    for (i = 0; i<p_motors_->n_drivers; i++) {
      started &&= p_motors_->drivers_[i]->beginTransmission() != DxlDriver::kErrorInvalidTransmitData;
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
    for (i=0; i<p_motors_->n_drivers; i++) {
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



}  // namespace motor_handles