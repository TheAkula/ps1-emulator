#ifndef CHANNEL_H
#define CHANNEL_h

#include <stdint.h>

typedef struct {
    uint32_t control;
    uint32_t base_address;
    uint32_t block;    
} Channel;

typedef enum {
    CH_MDEC_IN,
    CH_MDEC_OUT,
    CH_GPU,
    CH_CDROM,
    CH_SPU,
    CH_PIO,
    CH_OTC,
} DmaChannel;

typedef enum {
    MANUAL,
    REQUEST,
    LINKED_LIST,
} ChannelSync;

uint32_t ch_control(Channel* ch) {
    return ch->control;
}

uint32_t ch_base_address(Channel* ch) {
    return ch->base_address;
}

uint32_t ch_block(Channel* ch) {
    return ch->block;
}

void ch_set_control(Channel* ch, uint32_t v) {
    ch->control = v;
}

void ch_set_addr(Channel* ch, uint32_t v) {
    ch->base_address = v & 0xffffff;    
}

void ch_set_block(Channel* ch, uint32_t v) {
    ch->block = v;
}

uint8_t ch_sync(Channel* ch) {
    return (ch->control >> 9) & 0x3;
}

uint8_t ch_trigger(Channel* ch) {
    return (ch->control >> 28) & 0x1;
}

uint8_t ch_enable(Channel* ch) {
    return (ch->control >> 24) & 0x1;
}

char ch_active(Channel* ch) {
    uint8_t tr; 
    switch(ch_sync(ch)) {
    case 0x0:
	tr = ch_trigger(ch);
	break;
    default:
	tr = 0x1;
    }

    return ch_enable(ch) && tr;
}

uint8_t ch_dir(Channel* ch) {
    return ch->control & 0x1;
}

void ch_trigger_clear(Channel* ch) {
    ch->control &= 0xefffffff;
}

void ch_stop(Channel* ch) {
    ch->control &= 0xfeffffff;
}

uint8_t ch_step(Channel* ch) {
    return (ch->control >> 1) & 0x1;
}

uint16_t ch_block_size(Channel* ch) {
    return ch->block & 0xffff;
}

uint16_t ch_block_count(Channel* ch) {
    return ch->block >> 16;
}

#endif
