#ifndef DMA_H
#define DMA_H

#include <stdint.h>

#include "channel.h"

#define DMA_RESET 0x07654321

typedef struct Interconnect Interconnect;

typedef struct Dma {
    uint32_t control;
    uint32_t interrupt;

  // 1F80108xh DMA0 channel 0  MDECin  (RAM to MDEC)
  // 1F80109xh DMA1 channel 1  MDECout (MDEC to RAM)
  // 1F8010Axh DMA2 channel 2  GPU (lists + image data)
  // 1F8010Bxh DMA3 channel 3  CDROM   (CDROM to RAM)
  // 1F8010Cxh DMA4 channel 4  SPU
  // 1F8010Dxh DMA5 channel 5  PIO (Expansion Port)
  // 1F8010Exh DMA6 channel 6  OTC (reverse clear OT) (GPU related)
    Channel* channels[7];
} Dma;

Dma* initialize_dma();
void dma_set_reg(Dma* dma, uint32_t offset, uint32_t v, Interconnect* intr);
uint32_t dma_reg(Dma* dma, uint32_t offset);
char dma_irq(Dma* dma);
void dma_set_interrupt(Dma* dma, uint32_t v);
uint32_t dma_get_interrupt(Dma* dma);
DmaChannel channel_from_offset(Dma* dma, uint32_t offset);
void dma_transfer(Dma* dma, DmaChannel dch, Interconnect* intr);
void dma_block(Dma* dma, DmaChannel dch, Interconnect* intr);
void dma_linked_list(Dma* dma, DmaChannel dch, Interconnect* intr);
uint32_t dma_control(Dma* dma);

#endif
