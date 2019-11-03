/*
 * shareddma.cpp
 *
 * Created: 3/11/2019 10:12:42 PM
 *  Author: Joseph
 */
#include "dmashared.h"
#include "dmac.h"

#define DMA_MAX_CHANNELS_USED 10

static DmacDescriptor dma_first_desc[DMA_MAX_CHANNELS_USED] __attribute__((__aligned__(16)));
static DmacDescriptor dma_working_desc[DMA_MAX_CHANNELS_USED] __attribute__((__aligned__(16)));
static Callback* channel_callbacks[DMA_MAX_CHANNELS_USED]{nullptr};

DmacDescriptor* Dma::firstDesc(uint8_t channel) {
  return &(dma_first_desc[channel]);
}

DmacDescriptor* Dma::workingDesc(uint8_t channel) {
  return &(dma_working_desc[channel]);
}

void Dma::registerChannel(uint8_t channel, Callback* ins) {
  channel_callbacks[channel] = ins;
}

static uint8_t getSercomTx(uint8_t sercom_id) {
  return sercom_id * 2;
}

static uint8_t getSercomRx(uint8_t sercom_id) {
  return sercom_id * 2 + 1;
}

void Dma::irqHandler() {
  uint32_t i;
  Dmac* dmac = DMAC;
  const uint32_t chint = dmac->INTSTATUS.vec.CHINT;
  int deferred[DMA_MAX_CHANNELS_USED]{Callback::is_nothing};
  
  for(i = 0; i < DMA_MAX_CHANNELS_USED; i++) {
    if(chint & (1ul << i)) {
      dmac->CHID.bit.ID = i;
      if(dmac->CHINTFLAG.bit.TERR) {
        deferred[i] = Callback::is_error;
      } else if (dmac->CHINTFLAG.bit.TCMPL) {
        deferred[i] = Callback::is_done;
      }
    }
  }
  for(i=0; i<DMA_MAX_CHANNELS_USED; i++) {
    if(deferred[i] != Callback::is_nothing) {
      channel_callbacks[i]->callback(deferred[i]);
    }
  }
}

void Dma::swTrigger(uint8_t channel){
  Dmac* dmac = DMAC;
  switch(channel) {
    case 0:
      dmac->SWTRIGCTRL.bit.SWTRIG0 = 1; break;
    case 1:
      dmac->SWTRIGCTRL.bit.SWTRIG1 = 1; break;
    case 2:
      dmac->SWTRIGCTRL.bit.SWTRIG2 = 1; break;
    case 3:
      dmac->SWTRIGCTRL.bit.SWTRIG3 = 1; break;
    case 4:
      dmac->SWTRIGCTRL.bit.SWTRIG4 = 1; break;
    case 5:
      dmac->SWTRIGCTRL.bit.SWTRIG5 = 1; break;
    case 6:
      dmac->SWTRIGCTRL.bit.SWTRIG6 = 1; break;
    case 7:
      dmac->SWTRIGCTRL.bit.SWTRIG7 = 1; break;
    case 8:
      dmac->SWTRIGCTRL.bit.SWTRIG8 = 1; break;
    case 9:
      dmac->SWTRIGCTRL.bit.SWTRIG9 = 1; break;
    case 10:
      dmac->SWTRIGCTRL.bit.SWTRIG10 = 1; break;
    case 11:
      dmac->SWTRIGCTRL.bit.SWTRIG11 = 1; break;
    default:
      break;
  }
}