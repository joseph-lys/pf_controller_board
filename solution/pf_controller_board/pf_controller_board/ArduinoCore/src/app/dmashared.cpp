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

uint8_t Dma::getSercomTx(uint8_t sercom_id) {
  return sercom_id * 2 + 2;
}

uint8_t Dma::getSercomRx(uint8_t sercom_id) {
  return sercom_id * 2 + 1;
}

void Dma::irqHandler() {
  uint32_t i;
  uint32_t int_flag;
  Dmac* dmac = DMAC;
  const uint32_t chint = dmac->INTSTATUS.vec.CHINT;
  int deferred[DMA_MAX_CHANNELS_USED]{Callback::is_nothing};
  
  for(i = 0; i < DMA_MAX_CHANNELS_USED; i++) {
    if(chint & (1ul << i)) {
      __disable_irq();
      dmac->CHID.bit.ID = i;
      int_flag = dmac->CHINTFLAG.reg;
      __enable_irq();
      channel_callbacks[i]->callback(int_flag);
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

void Dma::init() {
  Dmac* dmac = DMAC;
  /// configure power management
  PM->AHBMASK.bit.DMAC_ = 1;
  
  /// configure NVIC
  NVIC_EnableIRQ(DMAC_IRQn);
  NVIC_SetPriority(DMAC_IRQn, 1);
  
  /// Configure common DMAC stuff
  dmac->CTRL.bit.DMAENABLE = 0;
  while (0 != dmac->CTRL.bit.DMAENABLE) {}
  dmac->CTRL.bit.SWRST = 1;
  while (0 != dmac->CTRL.bit.SWRST) {}
  
  dmac->BASEADDR.reg = reinterpret_cast<uint32_t>(&(dma_first_desc[0]));  
  dmac->WRBADDR.reg = reinterpret_cast<uint32_t>(&(dma_working_desc[0]));
  dmac->CTRL.bit.LVLEN0 = 1;
  dmac->CTRL.bit.LVLEN1 = 1;
  dmac->CTRL.bit.LVLEN2 = 1;
  dmac->CTRL.bit.LVLEN3 = 1;
  dmac->CTRL.bit.DMAENABLE = 1;
  while (1 != dmac->CTRL.bit.DMAENABLE) {}
}

// attach irq handler
void DMAC_Handler() {
  Dma::irqHandler();
}