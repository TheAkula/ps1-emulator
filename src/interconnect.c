#include "interconnect.h"

#include "dma.h"

const uint32_t REGION_MASK[8] = {
	// KUSEG: 2048MB
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	// KSEG0: 512MB
	0x7fffffff,
	// KSEG1: 512MB
	0x1fffffff,
	// KSEG2: 1024MB
	0xffffffff, 0xffffffff,
};

Range RAM_RANGE = { 0x00000000, RAM_SIZE };
Range BIOS_RANGE = { 0x1fc00000, BIOS_SIZE };
Range MEM_CONTROL = { 0x1f801000, 36 };
Range RAM_CONF_SIZE = { 0x1f801060, 4 };
Range CACHE_CONTROL = { 0xfffe0130, 4 };
Range SPU_RANGE = { 0x1f801c00, 640 };
Range EXPANSION_1 = { 0x1f000000, 1024 * 1024 * 8 };
Range EXPANSION_2 = { 0x1f802000, 66 };
Range SCRATCHPAD = { 0x1f800000, 1024 };
Range IRQ_CONTROL = { 0x1f801070, 8 };
// TODO: check real value
Range TIMERS = { 0x1f801100, 50 };
Range DMA = { 0x1f801080, 0x80 };
Range GPU = {  0x1f801810, 8 };

Interconnect* initialize_interconnect(Bios* bios, Ram* ram, Dma* dma, Gpu* gpu) {
	Interconnect* intr = malloc(sizeof(Interconnect));
	intr->bios = bios;
	intr->ram = ram;
	intr->dma = dma;
	intr->gpu = gpu;
	return intr;
}

uint32_t intr_load32(Interconnect* intr, uint32_t addr) {
	addr = mask_region(addr);

	if (range_contains(BIOS_RANGE, addr) == 1) {
		uint32_t offset = range_offset(BIOS_RANGE, addr);	

		return bios_load32(intr->bios, offset);
	}
    
	if (range_contains(RAM_RANGE, addr) == 1) {
		uint32_t offset = range_offset(RAM_RANGE, addr);

		return ram_load32(intr->ram, offset);
	}

	if(range_contains(IRQ_CONTROL, addr) == 1) {
		/* printf("unhandled irq load\n"); */
		return 0;
	}

	if(range_contains(DMA, addr) == 1) {	
		uint32_t offset = range_offset(DMA, addr);	
	
		return dma_reg(intr->dma, offset);
	}
    
	if(range_contains(GPU, addr) == 1) {
		uint32_t offset = range_offset(GPU, addr);

		if(offset == 4) {
	    uint32_t v = gpu_gpustat(intr->gpu);
	    return v;	    
		}
	
		if(offset == 0) {
	    return gpu_gpuread(intr->gpu);
		}
	
		printf("unhandled gpu access: %x\n", addr);
		exit(1);
	}

	if(range_contains(TIMERS, addr) == 1) {
		return 0x0;
	}
    
	printf("unhandled intr_load32 at address %x\n", addr);
	exit(1);
}

uint16_t intr_load16(Interconnect* intr, uint32_t addr) {    
	addr = mask_region(addr);

	if(range_contains(SPU_RANGE, addr) == 1) {
		/* printf("unhandled spu load\n"); */
		return 0;
	}

	if(range_contains(RAM_RANGE, addr) == 1) {
		uint32_t offset = range_offset(RAM_RANGE, addr);
	
		return ram_load16(intr->ram, offset);
	}    

	if(range_contains(IRQ_CONTROL, addr) == 1) {
		return 0;
	}
	
	printf("unhandled load16: %x\n", addr);
	exit(1);
}

uint8_t intr_load8(Interconnect* intr, uint32_t addr) {    
	addr = mask_region(addr);

	if(range_contains(BIOS_RANGE, addr) == 1) {
		uint32_t offset = range_offset(BIOS_RANGE, addr);

		return bios_load8(intr->bios, offset);
	}

	if(range_contains(RAM_RANGE, addr) == 1) {
		uint32_t offset = range_offset(RAM_RANGE, addr);

		return ram_load8(intr->ram, offset);
	}

	if(range_contains(EXPANSION_1, addr) == 1) {
		// TODO: implement expansion ?
		return 0xff;
	}

	printf("unhandled intr_load8 at address %x\n", addr);
	exit(1);
}

void intr_store32(Interconnect* intr, uint32_t addr, uint32_t v) {
	addr = mask_region(addr);
    
	// something related to RAM configuration    
	if (range_contains(RAM_CONF_SIZE, addr) == 1) {
		return;
	}
    
	if (range_contains(CACHE_CONTROL, addr) == 1) {
		return;
	}
        
	if (range_contains(MEM_CONTROL, addr) == 1) {
		uint32_t offset = range_offset(MEM_CONTROL, addr);
	
		switch(offset) {
		case 0: // Expansion 1 base address
	    if (v != 0x1f000000) {
				printf("bad value for expansion 1 address: %x\n", v);
				exit(1);
	    }
	    break;
		case 4: // Expansion 2 base address
	    if (v != 0x1f802000) {
				printf("bad value for expansion 2 address: %x\n", v);
				exit(1);
	    }
	    break;	
		}

		return;
	}

    
	if(range_contains(RAM_RANGE, addr) == 1) {
		uint32_t offset = range_offset(RAM_RANGE, addr);

		return ram_store32(intr->ram, offset, v);
	}

	if(range_contains(IRQ_CONTROL, addr) == 1) {
		/* printf("unhandled irq store\n"); */
		return;
	}

	if(range_contains(DMA, addr) == 1) {
		uint32_t offset = range_offset(DMA, addr);	
		return dma_set_reg(intr->dma, offset, v, intr);
	}    
    
	if(range_contains(GPU, addr) == 1) {
		uint32_t offset = range_offset(GPU, addr);

		if(offset == 0) {	    
	    gpu_gp0_command(intr->gpu, v);
	    return;
		}

		if(offset == 4) {	    
	    gpu_gp1_command(intr->gpu, v);	    
	    return;
		}
	
		printf("unhandled store32 gpu: %x\n", addr);
		exit(1);
	}

	if(range_contains(TIMERS, addr) == 1) {
		return;
	}
	
	printf("unhandled store32: %x\n", addr);
	exit(1);
}

void intr_store16(Interconnect* intr, uint32_t addr, uint16_t v) {
	addr = mask_region(addr);

	if(range_contains(SPU_RANGE, addr) == 1) {
		/* printf("unhandled write to spu register\n"); */
		return;
	}

	if(range_contains(TIMERS, addr) == 1) {
	
		/* printf("unhandled write to timers register\n"); */
		return;
	}

	if(range_contains(RAM_RANGE, addr) == 1) {
		uint32_t offset = range_offset(RAM_RANGE, addr);
	
		return ram_store16(intr->ram, offset, v);
	}

	if(range_contains(IRQ_CONTROL, addr) == 1) {
		return;
	}
    
	printf("unhandled store16: %x\n", addr);
	exit(1);
}

void intr_store8(Interconnect* intr, uint32_t addr, uint8_t v) {
	addr = mask_region(addr);

	if(range_contains(EXPANSION_2, addr) == 1) {
		/* printf("unhandled store to expansion2\n"); */

		return;
	}

	if(range_contains(RAM_RANGE, addr) == 1) {
		uint32_t offset = range_offset(RAM_RANGE, addr);

		ram_store8(intr->ram, offset, v);
	    
		return;
	}
    
	printf("unhandled store8: %x\n", addr);
	exit(1);
}

uint32_t mask_region(uint32_t addr) {
	int index = addr >> 29;

	uint32_t mask = REGION_MASK[index];
    
	return addr & mask;
}
