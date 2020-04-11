/// imp_hw_dxl.cpp
///
/// Copyright (c) 2020 Joseph Lee Yuan Sheng
///
/// This file is part of pf_controller_board which is released under MIT license.
/// See LICENSE file or go to https://github.com/joseph-lys/pf_controller_board for full license details.
///
/// Application specific implementation of the Hardware Wrapper used by the DxlDriver
///

#include <Arduino.h>
#include "imp_hw_dxl.h"


ImpHwDxl::ImpHwDxl(DmaUart& dma_uart, uint8_t dir_pin, uint32_t dir_tx_value) : 
dir_pin_(dir_pin),
dir_tx_value_(static_cast<uint8_t>(dir_tx_value)),
dir_rx_value_(static_cast<uint8_t>(dir_rx_value_ == HIGH ? LOW : HIGH)),
p_hw_driver_(&dma_uart) {
  p_hw_driver_->setTxDoneCallback(Callback(*this, &ImpHwDxl::doWhenTxDone));
  setRxDirection();
}

void ImpHwDxl::setTxDirection() {
  digitalWrite(static_cast<uint32_t>(dir_pin_), static_cast<uint32_t>(dir_tx_value_));
}

void ImpHwDxl::setRxDirection() {
  digitalWrite(static_cast<uint32_t>(dir_pin_), static_cast<uint32_t>(dir_rx_value_));
}

uint32_t ImpHwDxl::doWhenTxDone(uint32_t, uint32_t) {
  setRxDirection();
  tx_done_ = true;
}

size_t ImpHwDxl::available() {
  return p_hw_driver_->available();
}

bool ImpHwDxl::txIsDone() {
  return tx_done_;
}


bool ImpHwDxl::isTimeout() {
  bool is_timeout = false;
  if (timeout_tx_usec_) {
    is_timeout = timeout_tx_usec_ < (micros() - last_tx_usec_);
  }
  return is_timeout;
}

void ImpHwDxl::beginTransmission(uint8_t* tx_buf, size_t tx_buf_size, size_t expected_reply_size) {
  usec_t x;
  
  x = baud_per_byte_ * static_cast<usec_t>(tx_buf_size + expected_reply_size);
  x *= usec_per_128baud_;
  x += 63ul;  // rounding factor
  x >>= 7;  // divide by 128
  x += usec_delays_;
  timeout_tx_usec_ = x;
  tx_done_ = false;
  last_tx_usec_ = micros();
  setTxDirection();
}
