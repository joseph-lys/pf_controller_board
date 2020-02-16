/*
 * DmaInstance.h
 *
 * Created: 7/11/2019 8:30:16 PM
 *  Author: Joseph
 */ 


#ifndef DMAINSTANCE_H_
#define DMAINSTANCE_H_

#include "Arduino.h"
#include "dmac.h"
#include "callback.h"

class DmaInstance {
 public:
  explicit DmaInstance(uint8_t channel, Callback callback_function);
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
  void setupTxDescAny(DmacDescriptor* desc, uint32_t tx_address, uint8_t* tx_buf, uint32_t len);
  
  // Descriptor configuration for Sercom Rx
  void setupRxDescAny(DmacDescriptor* desc, uint32_t rx_address, uint8_t* rx_buf, uint32_t len);
    
  // First descriptor configuration for Sercom Tx
  inline void setupTxDescFirst(uint32_t tx_address, uint8_t* tx_buf, uint32_t len) {
    setupTxDescAny(ch_desc, tx_address, tx_buf, len);
  }
  
  // First descriptor configuration for Sercom Rx
  inline void setupRxDescFirst(uint32_t rx_address, uint8_t* rx_buf, uint32_t len) {
    setupRxDescAny(ch_desc, rx_address, rx_buf, len);
  }
  
  // Start dmac channel
  void start();
  
  // Stop dmac channel
  void stop();
    
  // Trigger a beat
  void triggerBeat();
  
  // Get First Descriptor
  DmacDescriptor* getDescFirst();
  
  // Get Working Descriptor
  DmacDescriptor* getDescWorking();
  
  // Get BCNT value of working descriptor
  uint32_t getWorkingCount();
  
  // Get Tx Address of working descriptor
  uint32_t getWorkingDestAddress();

  // Get Rx Address of working descriptor
  uint32_t getWorkingSrcAddress();
  
  // Get Next Descriptor Address or working descriptor
  uint32_t getWorkingNextDesc();
  
  void suspendChannel();
  
  void resumeChannel();
  
  // returns channel's pending flag
  inline bool isPending() {
    return static_cast<bool>(DMAC->PENDCH.reg & (1u << dma_channel));
  }
  
  inline bool isBusy() {
    return static_cast<bool>(DMAC->BUSYCH.reg & (1u << dma_channel));
  }
  
  private:
  const bool _has_callback;
};



#endif /* DMAINSTANCE_H_ */