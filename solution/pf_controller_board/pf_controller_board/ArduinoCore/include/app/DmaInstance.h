/*
 * DmaInstance.h
 *
 * Created: 7/11/2019 8:30:16 PM
 *  Author: Joseph
 */ 


#ifndef DMAINSTANCE_H_
#define DMAINSTANCE_H_

#include "dmac.h"
#include "callback.h"
class DmaInstance {
 public:
  explicit DmaInstance(uint8_t channel, Callback* callback_instance);
  virtual ~DmaInstance();
  const uint8_t dma_channel;
  DmacDescriptor* const ch_desc;
  
  // Channel configuration for Sercom Tx
  void setupTxConfig(uint8_t sercom_id);
  void setupTxConfig(uint8_t sercom_id, uint32_t priority);
  
  // Channel configuration for Sercom Rx
  void setupRxConfig(uint8_t sercom_id);
  void setupRxConfig(uint8_t sercom_id, uint32_t priority);
  
  // Descriptor configuration for Sercom Tx
  void setupTxDescFirst(uint32_t tx_address, uint8_t* tx_buf, uint32_t len);
  
  // Descriptor configuration for Sercom Rx
  void setupRxDescFirst(uint32_t rx_address, uint8_t* rx_buf, uint32_t len);
  
  // Descriptor configuration for Sercom Tx
  void setupTxDescAny(DmacDescriptor* desc, uint32_t tx_address, uint8_t* tx_buf, uint32_t len);
  
  // Descriptor configuration for Sercom Rx
  void setupRxDescAny(DmacDescriptor* desc, uint32_t rx_address, uint8_t* rx_buf, uint32_t len);
  
  // Start dmac channel
  void start();
  
  // Stop dmac channel
  void stop();
  
  // Trigger a beat
  void triggerBeat();
  
  // returns channel's pending flag
  bool isPending();
  
  private:
  const bool _has_callback;
};



#endif /* DMAINSTANCE_H_ */