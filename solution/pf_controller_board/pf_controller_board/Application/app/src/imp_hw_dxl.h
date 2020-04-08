/*
 * IncFile1.h
 *
 * Created: 5/4/2020 4:56:58 PM
 *  Author: josep
 */ 


#ifndef IMP_HW_DXL_H_
#define IMP_HW_DXL_H_

#include "dma_uart.h"
#include "IfHwDxlDriverBase.h"

/// Implementation of IfHwDxlDriverBase
class ImpHwDxl : public IfHwDxlDriverBase {
 private:
  const uint8_t dir_pin_;
  const uint8_t dir_tx_value_;
  const uint8_t dir_rx_value_;
  DmaUart* const p_hw_driver_;
 public:
  ImpHwDxl() = delete;

  ImpHwDxl(DmaUart& dma_uart, uint8_t dir_pin, uint32_t dir_tx_value);  
  /// check if there are any available data on the receive buffer
  
  uint32_t setTxDirection(uint32_t, uint32_t);
  uint32_t setRxDirection(uint32_t, uint32_t);

  size_t available() override;

  /// check if there are any available data on the receive buffer
  bool txIsDone() override;

  /// check if there are any available data on the receive buffer
  bool isTimeout() override;

  /// read one byte from the buffer
  uint8_t read() override;

  /// starts a transmission. [expected_reply_size] is used to estimate the timeout
  void beginTransmission(uint8_t* tx_buf, size_t tx_buf_size, uint8_t* rx_buf, size_t rx_buf_size, size_t expected_reply_size) override;
};



#endif /* IMP_HW_DXL_H_ */