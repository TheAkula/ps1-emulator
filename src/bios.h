#ifndef BIOS_H
#define BIOS_H

#include <stdio.h>
#include <stdint.h>

#include "platform.h"

typedef struct {
     uint8_t data[BIOS_SIZE];
} Bios;

Bios* initialize_bios(const char* path) {
    FILE* fptr;
    fptr = fopen(path, "rb");

    Bios* b = malloc(sizeof(Bios));
    fread(b->data, BIOS_SIZE, 1, fptr);

    fclose(fptr);

    return b;
}

uint32_t bios_load32(Bios* bios, uint32_t offset) {
    uint32_t b0 = bios->data[offset + 0];
    uint32_t b1 = bios->data[offset + 1];
    uint32_t b2 = bios->data[offset + 2];
    uint32_t b3 = bios->data[offset + 3];
    
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

uint8_t bios_load8(Bios* bios, uint32_t offset) {
    return bios->data[offset];
}

#endif
