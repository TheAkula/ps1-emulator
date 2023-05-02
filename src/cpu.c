#include "cpu.h"

void decode_and_execute(Cpu* cpu, Instruction instr) {
    printf("instr: %x\n", instr);
    uint32_t i = instr_function(instr);
    printf("instr opcode: %x\n", i);
    switch(i) {
    case 0b001111:
	op_lui(cpu, instr);	
	break;
    case 0b001101:
	op_ori(cpu, instr);
	break;
    case 0b101011:
	op_sw(cpu, instr);
	break;
    case 0b000000:
	op_secondary(cpu, instr);    
	break;
    case 0b001001:
	op_addiu(cpu, instr);
	break;
    case 0b000010:
	op_j(cpu, instr);
	break;
    case 0b010000:
	op_cop0(cpu, instr);
	break;
    case 0b000101:
	op_bne(cpu, instr);
	break;
    case 0b001000:
	op_addi(cpu, instr);
	break;
    case 0b100011:
	op_lw(cpu, instr);
	break;
    case 0b101001:
	op_sh(cpu, instr);
	break;
    case 0b000011:
	op_jal(cpu, instr);
	break;
    case 0b001100:
	op_andi(cpu, instr);
	break;
    case 0b101000:
	op_sb(cpu, instr);
	break;
    case 0b100000:
	op_lb(cpu, instr);
	break;
    case 0b000100:
	op_beq(cpu, instr);
	break;
    case 0b000111:
	op_bgtz(cpu, instr);
	break;
    case 0b000110:
	op_blez(cpu, instr);
	break;
    case 0b100100:
	op_lbu(cpu, instr);
	break;
    case 0b000001:
	op_bcondz(cpu, instr);
	break;
    case 0b001010:
	op_slti(cpu, instr);
	break;
    default:
	printf("unknown instruction: %x\n", i);
	exit(1);
    }
}

uint32_t cpu_load32(Cpu* cpu, uint32_t addr) {   
    return intr_load32(cpu->intr, addr);
}

uint8_t cpu_load8(Cpu* cpu, uint32_t addr) {
    return intr_load8(cpu->intr, addr);
}

void cpu_store32(Cpu* cpu, uint32_t addr, uint32_t v) {
    intr_store32(cpu->intr, addr, v);
}

void cpu_store16(Cpu* cpu, uint32_t addr, uint16_t v) {
    intr_store16(cpu->intr, addr, v);
}

void cpu_store8(Cpu* cpu, uint32_t addr, uint8_t v) {
    intr_store8(cpu->intr, addr, v);
}

void run_next_instruction(Cpu* cpu) {
    Instruction instr = cpu->next_instr;

    printf("load to reg %x %x\n", cpu->load[0], cpu->load[1]);
    set_reg(cpu, cpu->load[0], cpu->load[1]);

    cpu->load[0] = 0;
    cpu->load[1] = 0;

    for (int i = 0; i < 32; ++i) {
	cpu->regs[i] = cpu->out[i];
    }
    
    cpu->next_instr = cpu_load32(cpu, cpu->pc);
    cpu->pc = cpu->pc + 4;

    decode_and_execute(cpu, instr);
}

Cpu* initialize_cpu(Interconnect* intr) {
    Cpu* cpu = malloc(sizeof(Cpu));

    // $zero register        
    cpu->regs[0] = 0;
    cpu->out[0] = 0;
    for (int i = 1; i < 32; i++) {
	cpu->regs[i] = GARBAGE_VALUE;
	cpu->out[i] = GARBAGE_VALUE;
    }

    cpu->load[0] = 0;
    cpu->load[1] = 0;

    cpu->sr = 0;
    cpu->next_instr = 0x0;
    cpu->pc = RESET;
    cpu->intr = intr;
    return cpu;
}

void check_overflow(uint32_t v, uint32_t ov) {    
    if ((int)ov > 0 && (int)v > INT_MAX - (int)ov) {
	printf("overflow\n");
	exit(1);
    }
    // TODO: check if this is necessary
    /* if (sv < 0 && sv < INT_MIN - iv) { */
    /* 	printf("%d\n", INT_MIN); */
    /* 	printf("addi underflow %x %x %x %x %d %d\n", t, s, get_reg(cpu, s), i, sv, iv); */
    /* 	printf("addi underflow\n"); */
    /* 	exit(1); */
    /* } */    
}

uint32_t get_reg(Cpu* cpu, uint32_t index) {
    return cpu->regs[index];
}

void set_reg(Cpu* cpu, uint32_t index, uint32_t v) {
    printf("set reg %x value %x\n", index, v);
    cpu->out[index] = v;
    cpu->out[0] = 0;
}

void op_secondary(Cpu* cpu, Instruction instr) {
    uint32_t i = instr_subfunction(instr);

    printf("secondary instr op code: %x\n", i);
    switch(i) {
    case 0b000000:
	op_sll(cpu, instr);
	break;
    case 0b100101:
	op_or(cpu, instr);
	break;
    case 0b101011:
	op_sltu(cpu, instr);
	break;
    case 0b100001:
	op_addu(cpu, instr);
	break;
    case 0b001000:
	op_jr(cpu, instr);
	break;
    case 0b100100:
	op_and(cpu, instr);
	break;
    case 0b100000:
	op_add(cpu, instr);
	break;
    case 0b001001:
	op_jalr(cpu, instr);
	break;
    case 0b100011:
	op_subu(cpu, instr);
	break;
    case 0b000011:
	op_sra(cpu, instr);
	break;
    case 0b011010:
	op_div(cpu, instr);
	break;
    default:
	printf("unknown secondary instruction: %x\n", i);
	exit(1);
    }
}

void op_bcondz(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);

    switch(t) {
    case 0b000000:
	op_bltz(cpu, instr);
	break;
    case 0b000001:
	op_bgez(cpu, instr);
	break;
    case 0b010000:
	op_bltzal(cpu, instr);
	break;
    case 0b010001:
	op_bgezal(cpu, instr);
	break;
    default:
	printf("unknown bcondz instruction: %x\n", t);
    }
}

void op_lui(Cpu* cpu, Instruction instr) {
    uint32_t i = instr_imm(instr);
    uint32_t t = instr_t(instr);

    uint32_t v = i << 16;
    
    set_reg(cpu, t, v);
}

void op_ori(Cpu* cpu, Instruction instr) {
    uint32_t i = instr_imm(instr);
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);

    uint32_t v = get_reg(cpu, s) | i;
    
    set_reg(cpu, t, v);
}

void op_sw(Cpu* cpu, Instruction instr) {
    if ((cpu->sr & 0x10000) != 0) {
	printf("ignore store while cache is isolated\n");
	return;
    }
    
    uint32_t i = instr_imm_se(instr);
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    
    uint32_t addr = get_reg(cpu, s) + i;
    
    uint32_t v = get_reg(cpu, t);

    cpu_store32(cpu, addr, v);
}

void op_sll(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t d = instr_d(instr);
    uint32_t i = instr_shift(instr);
    
    uint32_t v = get_reg(cpu, t) << i;
    
    set_reg(cpu, d, v);
}

void op_addiu(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
   
    uint32_t v = get_reg(cpu, s) + i;        
       
    set_reg(cpu, t, v);
}

void op_j(Cpu* cpu, Instruction instr) {
    uint32_t j = instr_imm_jump(instr);

    cpu->pc = (cpu->pc & 0xf0000000) | (j << 2);
}

void op_or(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t d = instr_d(instr);

    uint32_t v = get_reg(cpu, s) | get_reg(cpu, t);

    set_reg(cpu, d, v);
}

void op_sltu(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t d = instr_d(instr);

    uint32_t v = get_reg(cpu, s) < get_reg(cpu, t);

    set_reg(cpu, d, v);
}

void op_addu(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t d = instr_d(instr);

    uint32_t v = get_reg(cpu, s) + get_reg(cpu, t);

    set_reg(cpu, d, v);
}

void op_and(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t d = instr_d(instr);

    uint32_t v = get_reg(cpu, t) & get_reg(cpu, s);
    
    set_reg(cpu, d, v);
}

void op_add(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t d = instr_d(instr);

    check_overflow(get_reg(cpu, t), get_reg(cpu, s));

    uint32_t v = get_reg(cpu, t) + get_reg(cpu, s);
    
    set_reg(cpu, d, v);
}

void op_jalr(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t d = instr_d(instr);

    set_reg(cpu, d, cpu->pc);
    cpu->pc = get_reg(cpu, s);    
}

void branch(Cpu* cpu, uint32_t offset) {
    uint32_t v = offset << 2;

    cpu->pc += v - 4;
}

void op_bne(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    if (get_reg(cpu, s) != get_reg(cpu, t)) {
	branch(cpu, i);
    }
}

void op_addi(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    check_overflow(get_reg(cpu, s), i);
    
    uint32_t v = get_reg(cpu, s) + i;    
    
    set_reg(cpu, t, v);    
}

void op_lw(Cpu* cpu, Instruction instr) {
    if ((cpu->sr & 0x10000) != 0) {
	printf("ignore lw while cache is isolated\n");
	return;
    }

    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);

    uint32_t addr = get_reg(cpu, s) + i;
    printf("lw from %x - %x to %x - %x\n", s, get_reg(cpu, s), t, addr);
    uint32_t v = cpu_load32(cpu, addr);

    cpu->load[0] = t;
    cpu->load[1] = v;    
}

void op_sh(Cpu* cpu, Instruction instr) {
    if ((cpu->sr & 0x10000) != 0) {
	printf("ignore store while cache is isolated\n");
	return;
    }
    
    uint32_t i = instr_imm_se(instr);
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    
    uint32_t addr = get_reg(cpu, s) + i;    
    uint32_t v = get_reg(cpu, t);

    cpu_store16(cpu, addr, (uint16_t)v);
}

void op_jal(Cpu* cpu, Instruction instr) {
    uint32_t ra = cpu->pc;

    set_reg(cpu, 31, ra);
    
    op_j(cpu, instr);
}

void op_andi(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm(instr);

    uint32_t v = get_reg(cpu, s) & i;
    
    set_reg(cpu, t, v);
}

void op_sb(Cpu* cpu, Instruction instr) {
    if ((cpu->sr & 0x10000) != 0) {
	printf("ignore store while cache is isolated\n");
	return;
    }

    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);

    uint32_t addr = get_reg(cpu, s) + i;    
    uint32_t v = get_reg(cpu, t);

    cpu_store8(cpu, addr, (uint8_t)v);
}

void op_jr(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);

    cpu->pc = get_reg(cpu, s);
}

void op_lb(Cpu* cpu, Instruction instr) {    
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);

    printf("lb %x %x\n", get_reg(cpu, s), i);
    uint32_t addr = get_reg(cpu, s) + i;
    int8_t v = cpu_load8(cpu, addr);

    cpu->load[0] = t;
    cpu->load[1] = (uint32_t)v;
}

void op_beq(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);

    if (get_reg(cpu, s) == get_reg(cpu, t)) {
	branch(cpu, i);
    }
}

void op_bgtz(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    if((int32_t)get_reg(cpu, s) > 0) {
	branch(cpu, i);
    }
}

void op_blez(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);

    if((int32_t)get_reg(cpu, s) <= 0) {
	branch(cpu, i);
    }
}

void op_lbu(Cpu* cpu, Instruction instr) {
    uint32_t t = instr_t(instr);
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    uint32_t addr = get_reg(cpu, s) + i;
    uint8_t v = cpu_load8(cpu, addr);

    cpu->load[0] = t;
    cpu->load[1] = (uint32_t)v;
}

void op_bltz(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    if((int32_t)get_reg(cpu, s) < 0) {
	branch(cpu, i);
    }
}

void op_bgez(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    if((int32_t)get_reg(cpu, s) >= 0) {
	branch(cpu, i);
    }
}

void op_bltzal(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    if((int32_t)get_reg(cpu, s) < 0) {
	set_reg(cpu, 31, cpu->pc);
	
	branch(cpu, i);
    }
}

void op_bgezal(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t i = instr_imm_se(instr);
    
    if((int32_t)get_reg(cpu, s) >= 0) {
	set_reg(cpu, 31, cpu->pc);
	
	branch(cpu, i);
    }
}

void op_slti(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t t = instr_t(instr);
    uint32_t i = instr_imm_se(instr);

    uint32_t v = (int32_t)get_reg(cpu, s) < (int32_t)i;
    set_reg(cpu, t, v);
}

void op_subu(Cpu* cpu, Instruction instr) {
    uint32_t s = instr_s(instr);
    uint32_t t = instr_t(instr);
    uint32_t d = instr_d(instr);

    uint32_t v = get_reg(cpu, s) - get_reg(cpu, t);
    set_reg(cpu, d, v);
}

void op_sra(Cpu* cpu, Instruction instr) {
    uint32_t i = instr_shift(instr);
    uint32_t t = instr_t(instr);
    uint32_t d = instr_d(instr);

    uint32_t v = ((int32_t)get_reg(cpu, t)) >> i;

    set_reg(cpu, d, v);
}

void op_cop0(Cpu* cpu, Instruction instr) {
    uint32_t i = instr_s(instr);
    switch(i) {
    case 0b000100:
	op_mtc0(cpu, instr);
	break;
    case 0b000000:
	op_mfc0(cpu, instr);
	break;
    default:
	printf("unkonwn cop0 instr: %x\n", i);
	exit(1);
    }
}

void op_mtc0(Cpu* cpu, Instruction instr) {
    uint32_t cpu_r = instr_t(instr);
    uint32_t cop_r = instr_d(instr);

    uint32_t v = get_reg(cpu, cpu_r);

    switch(cop_r) {
    case 3:
	if (v != 0) {
	    printf("invalid mtc0 BPC register value: %x\n", v);
	    exit(1);
	}
	break;
    case 5:
	if (v != 0) {
	    printf("invalid mtc0 BDA register value: %x\n", v);
	    exit(1);
	}
	break;
    case 6:
	if (v != 0) {
	    printf("invalid mtc0 6 register value: %x\n", v);
	    exit(1);
	}
	break;
    case 7:
	if (v != 0) {
	    printf("invalid mtc0 DCIC register value: %x\n", v);
	    exit(1);
	}
	break;
    case 9:
	if (v != 0) {
	    printf("invalid mtc0 BDAM register value: %x\n", v);
	    exit(1);
	}
	break;
    case 11:
	if (v != 0) {
	    printf("invalid mtc0 BPCM register value: %x\n", v);
	    exit(1);
	}
	break;
    case 12:
	cpu->sr = v;
	break;
    case 13:
	if (v != 0) {
	    printf("invalid mtc0 cause register value: %x\n", v);
	    exit(1);
	}
	break;
    default:
	printf("unknown mtc0 cop0 register: %x\n", cop_r);
	exit(1);
    }
}

void op_mfc0(Cpu* cpu, Instruction instr) {
    uint32_t cpu_r = instr_t(instr);
    uint32_t cop_r = instr_d(instr);

    uint32_t v;
    
    switch(cop_r) {
    case 12:
	v = cpu->sr;
	break;
    default:
	printf("unhandled mfc0 cop reg\n");
	exit(1);
    }

    cpu->load[0] = cpu_r;
    cpu->load[1] = v;
}
