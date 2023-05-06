#include "interconnect.h"
#include "bios.h"

#include <stdint.h>

#include "cpu.c"
#include "interconnect.c"
#include "ram.h"

int main(int argc, char* argv[]) {
    Bios* bios = initialize_bios("scph1001.bin");
    Ram* ram = initialize_ram();
    Interconnect* intr = initialize_interconnect(bios, ram);
    Cpu* cpu = initialize_cpu(intr);

    while (1) {
	run_next_instruction(cpu);
    }

    return 0;
}
