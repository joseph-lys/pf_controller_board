/// configuration.h
///
/// Copyright (c) 2020 Joseph Lee Yuan Sheng
///
/// This file is part of pf_controller_board which is released under MIT license.
/// See LICENSE file or go to https://github.com/joseph-lys/pf_controller_board for full license details.
///
/// Application assets
///


#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_


#include "motor_driver.h"
#include "dma_uart.h"
#include "dma_spi.h"

extern DmaUart dma_uart0;
extern DmaUart dma_uart1;
extern DmaSpiSlave DSPI;
extern MotorHandleFactory Motors;
// Application specific initialization
void initAppComponents();
void SpiEnd_Handler();

#endif /* CONFIGURATION_H_ */