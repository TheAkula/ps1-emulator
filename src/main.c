#include "interconnect.h"
#include "bios.h"

#include <stdint.h>

#include "glad.c"

#include "cpu.c"
#include "interconnect.c"
#include "dma.c"
#include "gpu.c"
#include "ram.h"
#include "dma.h"

int main(int argc, char* argv[]) {
	Bios* bios = initialize_bios("SCPH1001.BIN");
	Ram* ram = initialize_ram();
	Dma* dma = initialize_dma();
	Gpu* gpu = initialize_gpu();
	Interconnect* intr = initialize_interconnect(bios, ram, dma, gpu);
	Cpu* cpu = initialize_cpu(intr);        
    
	while (1) {
		run_next_instruction(cpu);
	}

	return 0;
}
