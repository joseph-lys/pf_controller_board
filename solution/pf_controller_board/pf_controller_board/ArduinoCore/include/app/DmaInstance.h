/*
 * DmaInstance.h
 *
 * Created: 7/11/2019 8:30:16 PM
 *  Author: Joseph
 */ 


#ifndef DMAINSTANCE_H_
#define DMAINSTANCE_H_

#include "dmac.h"

class DmaInstance {
 public:
  explicit DmaInstance(uint8_t channel);
  virtual ~DmaInstance();
  const uint8_t dma_channel;
  void startDmac();
  void stopDmac();
  // setup config 
  void setupChannelConfig();
  void setupFirstDescriptor();
  
};



#endif /* DMAINSTANCE_H_ */