/// imp_hw_dxl.h
///
/// Copyright (c) 2020 Joseph Lee Yuan Sheng
///
/// This file is part of pf_controller_board which is released under MIT license.
/// See LICENSE file or go to https://github.com/joseph-lys/pf_controller_board for full license details.
///
/// Application specific implementation of the Hardware Wrapper used by the DxlDriver
///


#ifndef IMP_HW_DXL_H_
#define IMP_HW_DXL_H_

#include "dma_uart.h"
#include "IfHwDxlDriverBase.h"

/// Implementation of IfHwDxlDriverBase
class ImpHwDxl : public IfHwDxlDriverBase {
 public:
  /// No default constructor
  ImpHwDxl() = delete;

  /// Construct 
  /// @param dma_uart underlying DMA UART driver to use
  /// @param dir_pin pin number for controlling the half-duplex flow direction
  /// @param dir_tx_value direction pin state when in TX flow direction, either HIGH or LOW
  ImpHwDxl(DmaUart& dma_uart, uint8_t dir_pin, uint32_t dir_tx_value);
  
  /// Set the half-duplex line to TX flow direction
  void setTxDirection();
 
  /// Set the half-duplex line to RX flow direction
  void setRxDirection();

  /// Callback Action when TX is complete
  /// @param a Unused argument to match Callback function signature
  /// @param b Unused argument to match Callback function signature
  /// @return always returns 0
  uint32_t doWhenTxDone(uint32_t a, uint32_t b);

  /// Get the number of bytes available to receive 
  /// @return number of bytes available
  size_t available() override;

  /// Check if transmission is complete
  /// @return true if transmission complete, false otherwise
  bool txIsDone() override;

  /// Check if communications has timeout
  /// @return true if communications timeout, false otherwise
  bool isTimeout() override;

  /// Read one byte from the DMA UART driver
  /// @return a byte of data, return -1 if error
  int read() override;

  /// Starts a transmission.
  /// @param tx_buf buffer containing the data source used to send
  /// @param tx_buf_size number of bytes to send
  /// @param expected_reply_size number of bytes that is expected to be received, this can be used to estimate timeout duration
  void beginTransmission(uint8_t* tx_buf, size_t tx_buf_size, size_t expected_reply_size) override;

 private:
  typedef unsigned long usec_t;  /// Typedef for native microseconds format
  bool tx_done_ = true;
  const uint8_t dir_pin_;
  const uint8_t dir_tx_value_;
  const uint8_t dir_rx_value_;
  DmaUart* const p_hw_driver_;
  usec_t last_tx_usec_ = 0;
  usec_t timeout_tx_usec_ = 0;
  
  /// Hardcoded for now
  const usec_t usec_per_128baud_ = (1000000ull * 128ull + 500000ull) / 1000000ull;  // (1000000 * 128 + (BAUD_PER_SEC / 2)) / BAUD_PER_SEC
  const usec_t baud_per_byte_ = 20;  // number of bauds per byte of transfer, including some safety factor
  const usec_t usec_transmission_delay_ = 1000;  // constant representing the sum of all transmission delay (transfer delay, motor processing time, etc)
  const usec_t usec_reception_delay_ = 10000;  // constant representing the sum of all receiving delay (motor response delay, etc)

};



#endif /* IMP_HW_DXL_H_ */