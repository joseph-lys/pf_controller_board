/*
 * dmashared.h
 *
 * Created: 3/11/2019 11:15:33 PM
 *  Author: Joseph
 */ 
#include "Arduino.h"
#include "dmac.h"
#ifndef DMASHARED_H_
#define DMASHARED_H_


class Dma{
  public:
  static DmacDescriptor* firstDesc(uint8_t channel);
  static DmacDescriptor* workingDesc(uint8_t channel);
  static inline Dmac* getDmac();  // conversion to get rid of macro
};





#endif /* DMASHARED_H_ */