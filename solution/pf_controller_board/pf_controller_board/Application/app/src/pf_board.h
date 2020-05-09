/*
 * pf_board.h
 *
 * Created: 9/5/2020 3:40:00 PM
 *  Author: josep
 */ 


#ifndef PF_BOARD_H_
#define PF_BOARD_H_

#include "configuration.h"

extern DmaUart DmaSerial0;
extern DmaUart DmaSerial1;
extern DxlDriver dxl0;
extern DxlDriver dxl1;
extern DmaSpiSlave DmaSPI;
extern MotorHandleFactory Motors;


#endif /* PF_BOARD_H_ */