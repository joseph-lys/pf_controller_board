/*
 * IfHwDxlImp.cpp
 *
 * Created: 5/4/2020 5:20:38 PM
 *  Author: josep
 */ 


#include "imp_hw_dxl.h"


ImpHwDxl::ImpHwDxl(DmaUart& dma_uart, uint8_t dir_pin, uint32_t dir_tx_value) 
: p_hw_driver_(&dma_uart), 
dir_pin_(dir_pin),
dir_tx_value_(static_cast<uint8_t>(dir_tx_value)),
dir_rx_value_(static_cast<uint8_t>(dir_rx_value_ == HIGH ? LOW : HIGH)) {
  p_hw_driver_->setTxDoneCallback(Callback(&this, &setTxDirection))
  setRxDirection(0, 0);
}

uint32_t ImpHwDxl::setTxDirection(uint32_t, uint32_t) {
  digitalWrite(static_cast<uint32_t>(dir_pin_), static_cast<uint32_t>(dir_tx_value_));
}

uint32_t ImpHwDxl::setRxDirection(uint32_t, uint32_t) {
  digitalWrite(static_cast<uint32_t>(dir_pin_), static_cast<uint32_t>(dir_rx_value_));
}

size_t ImpHwDxl::available() {
  return p_hw_driver_->available();
}

bool ImpHwDxl::txIsDone() {
  
}

void beginTransmission() {
}
