#ifndef RANGE_H
#define RANGE_H

#include "platform.h"
#include "ram.h"

typedef uint32_t Range[2];

char range_contains(Range r, uint32_t addr) {
    if (r[0] <= addr && addr <= (r[1] + r[0])) {
	return 1;
    } else {
	return 0;
    }
}

uint32_t range_offset(Range r, uint32_t addr) {
    return addr - r[0];
}

#endif
