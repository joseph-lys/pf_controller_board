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
    
  // Start dmac channel, does not wait for any sync
  inline void noWaitEnable() {
    __disable_irq();
    DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
    if (_has_callback) {
      DMAC->CHINTENSET.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;  
    }
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
    __enable_irq();
  }
  
  // wait for dmac channel to be enabled
  inline void waitEnabled() {
    __disable_irq();
    DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
    while (!DMAC->CHCTRLA.bit.ENABLE) { }
    __enable_irq();
  }
  
  // Stop dmac channel
  void stop();
  
  // Disable dmac channel, does not wait
  inline void noWaitDisable() {
    __disable_irq();
    DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
    DMAC->CHCTRLA.bit.ENABLE = 0;
    __enable_irq();
  }
  
  // Software reset dmac channel, does not wait
  inline void noWaitSoftwareReset() {
    __disable_irq();
    DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    __enable_irq();
  }
  
  // wait for dmac channel to be disabled/reseted
  inline void waitDisabled() {
    uint32_t cha;
    for(;;) {
      __disable_irq();
      DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
      cha = DMAC->CHCTRLA.reg | DMAC->CHSTATUS.reg;
      __enable_irq();  
      if(cha == 0) {
        break;
      }
    }
  }
    
  // Trigger a beat
  void triggerBeat();
  
  // returns channel's pending flag
  bool isPending();
  
  
  private:
  const bool _has_callback;
};



#endif /* DMAINSTANCE_H_ */