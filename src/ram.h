#ifndef RAM_H
#define RAM_H

#include <stdint.h>

#define RAM_SIZE ((uint32_t)(2097152))
#define RAM_GARBAGE 0xca

typedef struct {
    uint32_t data[RAM_SIZE];
} Ram;

Ram* initialize_ram() {
    Ram* ram = malloc(sizeof(Ram));

    for (int i = 0; i < RAM_SIZE; ++i) {
	ram->data[i] = RAM_GARBAGE;
    }

    return ram;
}

uint32_t ram_load32(Ram* ram, uint32_t offset) {
    uint32_t b0 = ram->data[offset + 0];
    uint32_t b1 = ram->data[offset + 1];
    uint32_t b2 = ram->data[offset + 2];
    uint32_t b3 = ram->data[offset + 3];
    
    return b0 | ( b1 << 8 ) | ( b2 << 16 ) | ( b3 << 24);
}

uint16_t ram_load16(Ram* ram, uint32_t offset) {
    uint16_t b0 = ram->data[offset + 0];
    uint16_t b1 = ram->data[offset + 1];

    return b0 | (b1 << 8);
}

uint8_t ram_load8(Ram* ram, uint32_t offset) {
    return ram->data[offset];
}

void ram_store32(Ram* ram, uint32_t offset, uint32_t value) {
    ram->data[offset + 0] = (uint8_t)(value);
    ram->data[offset + 1] = (uint8_t)(value >> 8);
    ram->data[offset + 2] = (uint8_t)(value >> 16);
    ram->data[offset + 3] = (uint8_t)(value >> 24);
}

void ram_store16(Ram* ram, uint32_t offset, uint32_t value) {
    ram->data[offset + 0] = (uint8_t)(value);
    ram->data[offset + 1] = (uint8_t)(value >> 8);
}

void ram_store8(Ram* ram, uint32_t offset, uint8_t value) {
    ram->data[offset] = value;
}

#endif
