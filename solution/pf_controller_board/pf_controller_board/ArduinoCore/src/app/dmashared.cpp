/*
 * shareddma.cpp
 *
 * Created: 3/11/2019 10:12:42 PM
 *  Author: Joseph
 */
#include "dmashared.h"
#include "dmac.h"

#define DMA_MAX_CHANNELS_USED 10

DmacDescriptor dma_first_desc[DMA_MAX_CHANNELS_USED] __attribute__((__aligned__(16)));
DmacDescriptor dma_working_desc[DMA_MAX_CHANNELS_USED] __attribute__((__aligned__(16)));


DmacDescriptor* Dma::firstDesc(uint8_t channel) {
  return &(dma_first_desc[channel]);
}


DmacDescriptor* Dma::workingDesc(uint8_t channel) {
  return &(dma_working_desc[channel]);
}

Dmac* Dma::getDmac(){
  return DMAC;
}