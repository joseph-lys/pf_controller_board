/*
 * dmashared.h
 *
 * Created: 3/11/2019 11:15:33 PM
 *  Author: Joseph
 */ 
#ifndef DMASHARED_H_
#define DMASHARED_H_

#include "Arduino.h"
#include "dmac.h"
#include "callback.h"

class Dma{
  public:
  static DmacDescriptor* firstDesc(uint8_t channel);
  static DmacDescriptor* workingDesc(uint8_t channel);
  static uint8_t getSercomTx(uint8_t sercom_id);
  static uint8_t getSercomRx(uint8_t sercom_id);
  static void registerChannel(uint8_t, Callback*);
  static void irqHandler();
  static void swTrigger(uint8_t channel);
};





#endif /* DMASHARED_H_ */