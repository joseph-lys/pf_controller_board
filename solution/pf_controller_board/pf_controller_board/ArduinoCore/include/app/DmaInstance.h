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
    
  // Start dmac channel, does not wait for any sync
  inline void noWaitEnable() {
    __disable_irq();
    DMAC->CHID.reg = DMAC_CHID_ID(dma_channel);
    if (_has_callback) {
      DMAC->CHINTENSET.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;  
    } else {
      DMAC->CHINTENCLR.reg = DMAC->CHINTENSET.reg;
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