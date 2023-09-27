#include "dma.h"

#include "interconnect.h"

Dma* initialize_dma() {
	Dma* dma = malloc(sizeof(Dma));
	dma->control = DMA_RESET;

	for(int i = 0; i < 7; ++i) {
		Channel* ch = malloc(sizeof(Channel));
		dma->channels[i] = ch;
	}
    
	return dma;
}

void dma_set_reg(Dma* dma, uint32_t offset, uint32_t v, Interconnect* intr) {
	uint8 r = offset & 0xf;

	if(offset >= 0x70) {
		switch(r) {
		case 0x0:
	    dma->control = v;
	    break;
		case 0x4:
	    dma_set_interrupt(dma, v);
	    break;
		default:
	    printf("unknown dma register: %x\n", r);
	    exit(1);
		}

		return;
	}

	DmaChannel dch = channel_from_offset(dma, offset);
	Channel* ch = dma->channels[dch];
    
	switch(r) {
	case 0x8:
		ch_set_control(ch, v);
		break;
	case 0x0:
		ch_set_addr(ch, v);
		break;
	case 0x4:
		ch_set_block(ch, v);
		break;
	default:
		printf("unknown set dma channel register: %x\n", r);
		exit(1);
	}
    
	if(ch_active(ch) == 1) {	
		dma_transfer(dma, dch, intr);
	}
}

uint32_t dma_reg(Dma* dma, uint32_t offset) {
	uint8_t r = offset & 0xf;

	if(offset >= 0x70) {		
		switch(r) {
		case 0x0:
	    return dma_control(dma);
		case 0x4:
	    return dma_get_interrupt(dma);
		default:
	    printf("unknown dma register: %x\n", r);
	    exit(1);
		}
	}

	DmaChannel dch = channel_from_offset(dma, offset);
	Channel* ch = dma->channels[dch];
    
	switch(r) {
	case 0x8:
		return ch_control(ch);	
	default:
		printf("unknwon get dma channel register: %x\n", r);
		exit(1);
	}
}

uint32_t dma_control(Dma* dma) {
	return dma->control;
}

char dma_irq(Dma* dma) {    
	uint8_t ch_irq = ((dma->interrupt >> 24) & 0x3f) & ((dma->interrupt >> 16) & 0x7f);

	return ((dma->interrupt >> 15) & 0x1)
		|| (((dma->interrupt >> 23) & 0x1) && (ch_irq != 0));    
}

void dma_set_interrupt(Dma* dma, uint32_t v) {
	uint8_t ack = (v >> 24) & 0x3f;

	dma->interrupt &= 0x80ffffff | (~ack << 24);
}

uint32_t dma_get_interrupt(Dma* dma) {    
	return (dma->interrupt & 0x7fffffff) | (dma_irq(dma) << 7);    
}

DmaChannel channel_from_offset(Dma* dma, uint32_t offset) {    
	uint8_t ch = (offset & 0xf0) >> 4;
	switch(ch) {
	case 0:
		return CH_MDEC_IN;
	case 1:
		return CH_MDEC_OUT;
	case 2:
		return CH_GPU;
	case 3:
		return CH_CDROM;
	case 4:
		return CH_SPU;
	case 5:
		return CH_PIO;
	case 6:
		return CH_OTC;
	default:
		printf("invalid channel: %x\n", ch);
		exit(1);
	}    
}

void dma_transfer(Dma* dma, DmaChannel dch, Interconnect* intr) {    
	Channel* ch = dma->channels[dch];
    
	ch_trigger_clear(ch);
    
	switch(ch_sync(ch)) {
	case 2:
		dma_linked_list(dma, dch, intr);
		break;
	default:
		dma_block(dma, dch, intr);
	}

	ch_stop(ch);
}

void dma_block(Dma* dma, DmaChannel dch, Interconnect* intr) {
	Channel* ch = dma->channels[dch];

	uint8_t dir = ch_dir(ch);
	int8_t s = ch_step(ch);
	uint32_t addr = ch_base_address(ch);

	uint32_t transfer_size = ch_transfer_size(ch);	

	int8_t step;

	switch(s) {	
	case 0x0:
		step = 4;
		break;
	case 0x1:
		step = -4;
		break;
	default:
		printf("invalid step size: %x\n", s);
		exit(1);
	}

	/* printf("dma block start: %x\n", transfer_size); */
	while (transfer_size > 0) {
		transfer_size -= 1;
		uint32_t cur_addr = addr & 0x1ffffc;
	    
		switch(dir) {
		case 0x1: {
			uint32_t word = ram_load32(intr->ram, cur_addr);
		
			switch(dch) {
			case CH_GPU:
				gpu_gp0_command(intr->gpu, word);		
				break;
			default:
				printf("unhandled peripheral transfer: %d\n", dch);
				exit(1);
			}		
		}
			break;
		case 0x0: {
			uint32_t word;		
		
			switch(dch) {
			case CH_OTC:
				if(transfer_size == 0) {
					word = 0xffffff;
				} else {
					word = (addr - 4) & 0x1fffff;
				}
				break;
			case CH_GPU:
				word = gpu_gpuread(intr->gpu);
				break;
			default:
				printf("unhandled peripheral transfer: %d\n", dch);
				exit(1);
			}
		
			ram_store32(intr->ram, cur_addr, word);
		}
			break;
		default:
			printf("invalid transfer direction %x\n", dir);
			exit(1);
		}

		addr += step;
	}
}

void dma_linked_list(Dma* dma, DmaChannel dch, Interconnect* intr) {
	Channel* ch = dma->channels[dch];

	uint8_t dir = ch_dir(ch);

	/* printf("linked list\n"); */
    
	uint32_t addr = ch_base_address(ch) & 0x1ffffc;

	if(dch != CH_GPU) {
		printf("unhandled linked list peripheral: %d\n", dch);
		exit(1);
	}

	if(dir != 1) {
		printf("unhandled linked list direction: %d\n", dir);
		exit(1);
	}	
        
	while(1) {	  
		uint32_t header = ram_load32(intr->ram, addr);

		uint8_t size = header >> 24;
	
		for(int i = 0; i < size; ++i) {
	    addr = (addr + 4) & 0x1ffffc;
	    uint32_t packet = ram_load32(intr->ram, addr);
	    gpu_gp0_command(intr->gpu, packet);
		}

		if((header & 0x800000) != 0) 
	    break;

		addr = header & 0x1ffffc;
	}				
}

