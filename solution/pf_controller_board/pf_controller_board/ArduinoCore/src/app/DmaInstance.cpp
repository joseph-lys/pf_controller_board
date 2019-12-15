/*
 * DmaInstance.cpp
 *
 * Created: 7/11/2019 8:30:44 PM
 *  Author: Joseph
 */ 
#include "Arduino.h"
#include "DmaCommon.h"
#include "DmaInstance.h"

DmaInstance::DmaInstance(uint8_t _dma_channel, Callback* callback_instance)
: dma_channel(_dma_channel), ch_desc(Dma::firstDesc(_dma_channel)), _has_callback(callback_instance != nullptr) {
  if (_has_callback) {
    Dma::registerChannel(dma_channel, callback_instance);
  }
}

DmaInstance::~DmaInstance() {}

void DmaInstance::setupTxConfig(uint8_t sercom_id) {
  setupTxConfig(sercom_id, 1);  // use default priority 1
}

void DmaInstance::setupTxConfig(uint8_t sercom_id, uint32_t priority) {
  Dmac* dmac = DMAC;
  
  // set configurations
  DMAC_CHCTRLB_Type ch_ctrl_b;
  ch_ctrl_b.reg = 0;
  ch_ctrl_b.bit.EVACT = DMAC_CHCTRLB_EVACT_NOACT_Val;
  ch_ctrl_b.bit.EVIE = 0;  // no input event
  ch_ctrl_b.bit.EVOE = 0;  // no output event
  ch_ctrl_b.bit.LVL = priority;  // reserve one level for SPI
  ch_ctrl_b.bit.TRIGSRC = Dma::getSercomTx(sercom_id);
  ch_ctrl_b.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;  //
  
  __disable_irq();
  dmac->CHID.bit.ID = dma_channel;
  dmac->CHCTRLB.reg = ch_ctrl_b.reg;
  if (_has_callback) {
    dmac->CHINTENSET.bit.TCMPL = 1;
    dmac->CHINTENSET.bit.TERR = 1;    
  } else {
  dmac->CHINTENCLR.bit.TCMPL = 1;
  dmac->CHINTENCLR.bit.TERR = 1;    
  }
  dmac->CHINTENCLR.bit.SUSP = 1;
  __enable_irq();
}

void DmaInstance::setupRxConfig(uint8_t sercom_id) {
  setupRxConfig(sercom_id, 1);  // use default priority 1
}

void DmaInstance::setupRxConfig(uint8_t sercom_id, uint32_t priority) {
  Dmac* dmac = DMAC;
  
  // set configurations
  DMAC_CHCTRLB_Type ch_ctrl_b;
  ch_ctrl_b.reg = 0;
  ch_ctrl_b.bit.EVACT = DMAC_CHCTRLB_EVACT_NOACT_Val;
  ch_ctrl_b.bit.EVIE = 0;  // no input event
  ch_ctrl_b.bit.EVOE = 0;  // no output event
  ch_ctrl_b.bit.LVL = priority;  
  ch_ctrl_b.bit.TRIGSRC = Dma::getSercomRx(sercom_id);
  ch_ctrl_b.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;  //
  
  __disable_irq();
  dmac->CHID.bit.ID = dma_channel;
  dmac->CHCTRLB.reg = ch_ctrl_b.reg;
  if (_has_callback) {
    dmac->CHINTENSET.bit.TCMPL = 1;
    dmac->CHINTENSET.bit.TERR = 1;
  } else {
    dmac->CHINTENCLR.bit.TCMPL = 1;
    dmac->CHINTENCLR.bit.TERR = 1;
  }
  dmac->CHINTENCLR.bit.SUSP = 1;
  __enable_irq();
}

void DmaInstance::setupTxDescFirst(uint32_t tx_address, uint8_t* buf, uint32_t len) {
  setupTxDescAny(ch_desc, tx_address, buf, len);
}

void DmaInstance::setupTxDescAny(DmacDescriptor* desc, uint32_t tx_address, uint8_t* buf, uint32_t len) {
  // initialize descriptor data
  desc->BTCNT.reg = 0;
  desc->BTCTRL.reg = 0;
  // Set descriptor for TX
  DMAC_BTCTRL_Type btctrl;
  btctrl.reg = 0;
  btctrl.bit.VALID = 1;
  btctrl.bit.EVOSEL = DMAC_BTCTRL_EVOSEL_DISABLE_Val;
  btctrl.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;
  btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  btctrl.bit.SRCINC = 1;
  btctrl.bit.DSTINC = 0;  // writes to fixed sercom->DATA address
  btctrl.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_SRC_Val;
  btctrl.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;
  desc->BTCTRL.reg = btctrl.reg;
  /* Set transfer size, source address and destination address */
  desc->BTCNT.reg = len;
  desc->SRCADDR.reg = reinterpret_cast<uint32_t>(buf) + static_cast<uint32_t>(len);
  desc->DSTADDR.reg = tx_address;
  desc->DESCADDR.reg = 0;
}

void DmaInstance::setupRxDescFirst(uint32_t rx_address, uint8_t* buf, uint32_t len) {
  setupRxDescAny(ch_desc, rx_address, buf, len);
}

void DmaInstance::setupRxDescAny(DmacDescriptor* desc, uint32_t rx_address, uint8_t* buf, uint32_t len) {
  // initialize descriptor data
  desc->BTCNT.reg = 0;
  desc->BTCTRL.reg = 0;
  // Set descriptor for TX
  DMAC_BTCTRL_Type btctrl;
  btctrl.reg = 0;
  btctrl.bit.VALID = 1;
  btctrl.bit.EVOSEL = DMAC_BTCTRL_EVOSEL_DISABLE_Val;
  btctrl.bit.BLOCKACT = DMAC_BTCTRL_BLOCKACT_INT_Val;
  btctrl.bit.BEATSIZE = DMAC_BTCTRL_BEATSIZE_BYTE_Val;
  btctrl.bit.SRCINC = 0;
  btctrl.bit.DSTINC = 1;  // writes to fixed sercom->DATA address
  btctrl.bit.STEPSEL = DMAC_BTCTRL_STEPSEL_DST_Val;
  btctrl.bit.STEPSIZE = DMAC_BTCTRL_STEPSIZE_X1_Val;
  desc->BTCTRL.reg = btctrl.reg;
  /* Set transfer size, source address and destination address */
  desc->BTCNT.reg = len;
  desc->SRCADDR.reg = rx_address;
  desc->DSTADDR.reg = reinterpret_cast<uint32_t>(buf) + static_cast<uint32_t>(len);
  desc->DESCADDR.reg = 0;
}

void DmaInstance::start() {
  Dmac* dmac = DMAC;
  __disable_irq();
  // select DMA channel
  dmac->CHID.bit.ID = dma_channel;
  // enable interrupts
  if (_has_callback) {
    dmac->CHINTENSET.reg = DMAC_CHINTFLAG_TERR | DMAC_CHINTFLAG_TCMPL;  
  }
  // enable dma
  dmac->CHCTRLA.bit.ENABLE = 1;
  __enable_irq();
}

void DmaInstance::stop() {
  Dmac* dmac = DMAC;
  __disable_irq();
  dmac->CHID.reg = DMAC_CHID_ID(dma_channel);
  dmac->CHCTRLB.bit.CMD = DMAC_CHCTRLB_CMD_SUSPEND_Val;  // suspend channel or a lot of weird shit happens
  // channel disable
  dmac->CHCTRLA.bit.ENABLE = 0;
  __enable_irq();
  waitDisabled();

  // sw reset
  __disable_irq();
  dmac->CHID.reg = DMAC_CHID_ID(dma_channel);
  dmac->CHCTRLA.bit.SWRST = 1;
  __enable_irq();
  waitDisabled();
}

void DmaInstance::triggerBeat() {
  Dma::swTrigger(dma_channel);
}

bool DmaInstance::isPending() {
  Dmac* dmac = DMAC;  
  return static_cast<bool>(dmac->PENDCH.vec.PENDCH & (uint32_t(1) << dma_channel));
}