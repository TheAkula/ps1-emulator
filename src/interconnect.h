#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <stdlib.h>
#include <stdint.h>

#include "platform.h"
#include "bios.h"
#include "range.h"
#include "ram.h"

typedef struct {
    Bios* bios;
    Ram* ram;    
} Interconnect;

Interconnect* initialize_interconnect(Bios* bios, Ram* ram);
uint32_t intr_load32(Interconnect* intr, uint32_t addr);
uint16_t intr_load16(Interconnect* intr, uint32_t addr);
uint8_t intr_load8(Interconnect* intr, uint32_t addr);
void intr_store32(Interconnect* intr, uint32_t addr, uint32_t v);
void intr_store16(Interconnect* intr, uint32_t addr, uint16_t v);
void intr_store8(Interconnect* intr, uint32_t addr, uint8_t v);
uint32_t mask_region(uint32_t addr);

#endif
