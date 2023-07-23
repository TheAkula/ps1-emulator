#include "cpu.h"

void decode_and_execute(Cpu* cpu, Instruction instr) {
	/* printf("instr: %x\n", instr); */
	uint32_t i = instr_function(instr);
	/* printf("instr opcode: %x\n", i); */
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
	case 0b001011:
		op_sltiu(cpu, instr);
		break;
	case 0b100101:
		op_lhu(cpu, instr);
		break;
	case 0b100001:
		op_lh(cpu, instr);
		break;
	case 0b001110:
		op_xori(cpu, instr);
		break;
	case 0b100010:
		op_lwl(cpu, instr);
		break;
	case 0b100110:
		op_lwr(cpu, instr);
		break;
	case 0b010001:
		op_cop1(cpu, instr);
		break;
	case 0b010010:
		op_cop2(cpu, instr);
		break;
	case 0b010011:
		op_cop3(cpu, instr);
		break;
	case 0b101010:
		op_swl(cpu, instr);
		break;
	case 0b101110:
		op_swr(cpu, instr);
		break;
	case 0b110000:
		op_lwc0(cpu, instr);
		break;
	case 0b110001:
		op_lwc1(cpu, instr);
		break;
	case 0b110010:
		op_lwc2(cpu, instr);
		break;
	case 0b110011:
		op_lwc3(cpu, instr);
		break;
	case 0b111000:
		op_swc0(cpu, instr);	
		break;
	case 0b111001:
		op_swc1(cpu, instr);	
		break;
	case 0b111010:
		op_swc2(cpu, instr);	
		break;
	case 0b111011:
		op_swc3(cpu, instr);	
		break;
	default:
		op_illegal(cpu, instr);
	}
}

uint32_t cpu_load32(Cpu* cpu, uint32_t addr) {   
	return intr_load32(cpu->intr, addr);
}

uint32_t cpu_load16(Cpu* cpu, uint32_t addr) {
	return intr_load16(cpu->intr, addr);
}

uint8_t cpu_load8(Cpu* cpu, uint32_t addr) {
	return intr_load8(cpu->intr, addr);
}

void cpu_store32(Cpu* cpu, uint32_t addr, uint32_t v) {
	if ((cpu->sr & 0x10000) != 0) {
		/* printf("ignore store while cache is isolated\n"); */
		return;
	}
	intr_store32(cpu->intr, addr, v);
}

void cpu_store16(Cpu* cpu, uint32_t addr, uint16_t v) {
	if ((cpu->sr & 0x10000) != 0) {
		/* printf("ignore store while cache is isolated\n"); */
		return;
	}    
	intr_store16(cpu->intr, addr, v);
}

void cpu_store8(Cpu* cpu, uint32_t addr, uint8_t v) {
	if ((cpu->sr & 0x10000) != 0) {
		/* printf("ignore store while cache is isolated\n"); */
		return;
	}    
	intr_store8(cpu->intr, addr, v);
}

void run_next_instruction(Cpu* cpu) {        
	cpu->curr_pc = cpu->pc;

	if(cpu->curr_pc % 4 != 0) {
		return exception(cpu, LOAD_BUS);
	}

	Instruction instr = cpu_load32(cpu, cpu->pc);
   	       
	cpu->pc = cpu->next_pc;
	cpu->next_pc += 4;
        
	set_reg(cpu, cpu->load[0], cpu->load[1]);
	cpu->load[0] = 0;
	cpu->load[1] = 0;          	

	cpu->delay_slot = cpu->branch;
	cpu->branch = 0;
	
	decode_and_execute(cpu, instr);

	for (int i = 0; i < 32; ++i) {
		cpu->regs[i] = cpu->out[i];		
	}	
}

void exception(Cpu* cpu, Exception cause) {
	uint32_t handler;
    
	if((cpu->sr & (1 << 22)) != 0) {
		handler = 0xbfc00180;
	} else {
		handler = 0x80000080;
	}

	uint32_t mode = cpu->sr & 0x3f;
	cpu->sr &= ~0x3f;
	cpu->sr |= (mode << 2) & 0x3f;

	cpu->cause = cause << 2;
    
	cpu->epc = cpu->curr_pc;

	if(cpu->delay_slot == 1) {
		cpu->epc -= 4;
		cpu->cause |= 1 << 31;
	}
    
	cpu->pc = handler;
	cpu->next_pc = cpu->pc + 4;
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

	cpu->hi = GARBAGE_VALUE;
	cpu->lo = GARBAGE_VALUE;
	cpu->sr = 0;
	cpu->pc = RESET;
	cpu->next_pc = cpu->pc + 4;
	cpu->intr = intr;
	return cpu;
}

char check_overflow(Cpu* cpu, uint32_t v, uint32_t ov) {    
	if ((int)ov > 0 && (int)v > INT_MAX - (int)ov) {
		printf("overflow\n");		
		return 1;
	}    
	// TODO: check if this is necessary
	/* if (ov < 0 && v < INT_MIN - ov) {		 */
	/* 	printf("addi underflow\n"); */
	/* 	exit(1); */
	/* } */

	return 0;
}

uint32_t get_reg(Cpu* cpu, uint32_t index) {
	return cpu->regs[index];
}

void set_reg(Cpu* cpu, uint32_t index, uint32_t v) {
	cpu->out[index] = v;
	cpu->out[0] = 0;
}

void op_secondary(Cpu* cpu, Instruction instr) {
	uint32_t i = instr_subfunction(instr);

	/* printf("secondary instr op code: %x\n", i); */
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
	case 0b010010:
		op_mflo(cpu, instr);
		break;
	case 0b000010:
		op_srl(cpu, instr);
		break;
	case 0b011011:
		op_divu(cpu, instr);
		break;
	case 0b010000:
		op_mfhi(cpu, instr);
		break;
	case 0b101010:
		op_slt(cpu, instr);
		break;
	case 0b001100:
		op_syscall(cpu, instr);
		break;
	case 0b010011:
		op_mtlo(cpu, instr);
		break;
	case 0b010001:
		op_mthi(cpu, instr);
		break;
	case 0b000100:
		op_sllv(cpu, instr);
		break;
	case 0b100111:
		op_nor(cpu, instr);
		break;
	case 0b000111:
		op_srav(cpu, instr);
		break;
	case 0b000110:
		op_srlv(cpu, instr);
		break;
	case 0b011001:
		op_multu(cpu, instr);
		break;
	case 0b100110:
		op_xor(cpu, instr);
		break;
	case 0b001101:
		op_break(cpu, instr);
		break;
	case 0b011000:
		op_mult(cpu, instr);
		break;
	case 0b100010:
		op_sub(cpu, instr);
		break;    
	default:
		op_illegal(cpu, instr);
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
		exit(1);
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
	uint32_t i = instr_imm_se(instr);
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
    
	uint32_t addr = get_reg(cpu, s) + i;

	if(addr % 4 != 0) {
		return exception(cpu, STORE_BUS);	
	}
    
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
	uint32_t j = instr_imm_jump(instr) << 2;

	cpu->branch = 1;
	cpu->next_pc = (cpu->pc & 0xf0000000) | j;
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

	if (check_overflow(cpu, get_reg(cpu, t), get_reg(cpu, s)) == 1) {
		return exception(cpu, OVERFLOW);
	}

	uint32_t v = get_reg(cpu, t) + get_reg(cpu, s);
    
	set_reg(cpu, d, v);
}

void op_jalr(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
	uint32_t d = instr_d(instr);

	set_reg(cpu, d, cpu->next_pc);
	cpu->branch = 1;
	cpu->next_pc = get_reg(cpu, s);    
}

void branch(Cpu* cpu, uint32_t offset) {
	uint32_t v = offset << 2;

	cpu->branch = 1;
	cpu->next_pc = cpu->pc + v;
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
    
	if(check_overflow(cpu, get_reg(cpu, s), i) == 1) {	
		return exception(cpu, OVERFLOW);
	}
    
	uint32_t v = (int32_t)get_reg(cpu, s) + (int32_t)i;    
    
	set_reg(cpu, t, v);    
}

void op_lw(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;

	if(addr % 4 != 0) {
		return exception(cpu, LOAD_BUS);
	}
    
	uint32_t v = cpu_load32(cpu, addr);

	cpu->load[0] = t;
	cpu->load[1] = v;    
}

void op_sh(Cpu* cpu, Instruction instr) {
	uint32_t i = instr_imm_se(instr);
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
    
	uint32_t addr = get_reg(cpu, s) + i;    

	if(addr % 2 != 0) {
		return exception(cpu, STORE_BUS);
	}

	uint32_t v = get_reg(cpu, t);

	cpu_store16(cpu, addr, (uint16_t)v);
}

void op_jal(Cpu* cpu, Instruction instr) {
	uint32_t ra = cpu->next_pc;

	set_reg(cpu, 31, ra);
	
	op_j(cpu, instr);
	cpu->branch = 1;
}

void op_andi(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm(instr);

	uint32_t v = get_reg(cpu, s) & i;
    
	set_reg(cpu, t, v);
}

void op_sb(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;    
	uint32_t v = get_reg(cpu, t);

	cpu_store8(cpu, addr, (uint8_t)v);
}

void op_jr(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);

	cpu->branch = 1;
	cpu->next_pc = get_reg(cpu, s);
}

void op_lb(Cpu* cpu, Instruction instr) {    
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);
    
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
	set_reg(cpu, 31, cpu->next_pc);
	if((int32_t)get_reg(cpu, s) < 0) {	
		branch(cpu, i);
	}
}

void op_bgezal(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);    
	set_reg(cpu, 31, cpu->next_pc);
	if((int32_t)get_reg(cpu, s) >= 0) {
	
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

void op_div(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
	uint32_t t = instr_t(instr);

	int32_t n = get_reg(cpu, s);
	int32_t d = get_reg(cpu, t);

	if(d == 0) {
		cpu->hi = n;

		if(n >= 0) {
	    cpu->lo = 0xffffffff;
		} else {
	    cpu->lo = 1;
		}
	} else if((uint32_t)n == 0x80000000 && d == -1) {
		cpu->hi = 0;
		cpu->lo = 0x80000000;
	} else {
		cpu->hi = (uint32_t)(n % d);
		cpu->lo = (uint32_t)(n / d);
	}    
}

void op_mflo(Cpu* cpu, Instruction instr) {
	uint32_t d = instr_d(instr);

	set_reg(cpu, d, cpu->lo);
}

void op_srl(Cpu* cpu, Instruction instr) {
	uint32_t d = instr_d(instr);
	uint32_t t = instr_t(instr);
	uint32_t i = instr_shift(instr);

	uint32_t v = get_reg(cpu, t) >> i;

	set_reg(cpu, d, v);
}

void op_sltiu(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t v = get_reg(cpu, s) < i;

	set_reg(cpu, t, v);
}

void op_lhu(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;

	if(addr % 2 != 0) {
		return exception(cpu, LOAD_BUS);
	}
    
	uint32_t v = cpu_load16(cpu, addr);

	cpu->load[0] = t;
	cpu->load[1] = v;
}

void op_xori(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm(instr);

	uint32_t v = get_reg(cpu, s) ^ i;

	set_reg(cpu, t, v);
}

void op_lwl(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;

	uint32_t cur_v = cpu->out[t];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = cpu_load32(cpu, aligned_addr);

	uint32_t v;

	switch(addr & 3) {
	case 0:
		v = (cur_v & 0x00ffffff) | (aligned_word << 24);
		break;
	case 1:
		v = (cur_v & 0x0000ffff) | (aligned_word << 16);
		break;
	case 2:
		v = (cur_v & 0x000000ff) | (aligned_word << 8);
		break;
	case 3:
		v = (cur_v & 0x00000000) | (aligned_word << 0);
		break;
	default:
		printf("unknown lwl addr: %x\n", addr & 3);
		exit(1);
	}

	cpu->load[0] = t;
	cpu->load[1] = v;
}

void op_lwr(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;

	uint32_t cur_v = cpu->out[t];

	uint32_t aligned_addr = addr & ~3;
	uint32_t aligned_word = cpu_load32(cpu, aligned_addr);

	uint32_t v;

	switch(addr & 3) {
	case 0:
		v = (cur_v & 0x00000000) | (aligned_word >> 0);
		break;
	case 1:
		v = (cur_v & 0xff000000) | (aligned_word >> 8);
		break;
	case 2:
		v = (cur_v & 0xffff0000) | (aligned_word >> 16);
		break;
	case 3:
		v = (cur_v & 0xffffff00) | (aligned_word >> 24);
		break;
	default:
		printf("unknown lwr addr: %x\n", addr & 3);
		exit(1);
	}

	cpu->load[0] = t;
	cpu->load[1] = v;
}

void op_swl(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;
	uint32_t v = get_reg(cpu, t);

	uint32_t aligned_addr = addr & ~3;        
	uint32_t cur_v = cpu_load32(cpu, aligned_addr);

	uint32_t word;
	switch(addr & 3) {
	case 0:
		word = (cur_v & 0xffffff00) | (v >> 24);
		break;
	case 1:
		word = (cur_v & 0xffff0000) | (v >> 16);
		break;
	case 2:
		word = (cur_v & 0xff000000) | (v >> 8);
		break;
	case 3:
		word = (cur_v & 0x00000000) | (v >> 0);
		break;
	default:
		printf("unknown swl addr: %x\n", addr & 3);
		exit(1);
	}

	cpu_store32(cpu, aligned_addr, word);
}

void op_swr(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;
	uint32_t v = get_reg(cpu, t);

	uint32_t aligned_addr = addr & ~3;        
	uint32_t cur_v = cpu_load32(cpu, aligned_addr);

	uint32_t word;
	switch(addr & 3) {
	case 0:
		word = (cur_v & 0x00000000) | (v << 0);
		break;
	case 1:
		word = (cur_v & 0x000000ff) | (v << 8);
		break;
	case 2:
		word = (cur_v & 0x0000ffff) | (v << 16);
		break;
	case 3:
		word = (cur_v & 0x00ffffff) | (v << 24);
		break;
	default:
		printf("unknown swr addr: %x\n", addr & 3);
		exit(1);
	}

	cpu_store32(cpu, aligned_addr, word);
}

void op_illegal(Cpu* cpu, Instruction instr) {
	exception(cpu, ILLEGAL);
}

void op_divu(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
	uint32_t t = instr_t(instr);

	uint32_t n = get_reg(cpu, s);
	uint32_t d = get_reg(cpu, t);

	if(d == 0) {
		cpu->hi = n;
		cpu->lo = 0xffffffff;
	} else {    
		cpu->hi = (uint32_t)(n % d);
		cpu->lo = (uint32_t)(n / d);
	}
}

void op_mfhi(Cpu* cpu, Instruction instr) {
	uint32_t d = instr_d(instr);

	set_reg(cpu, d, cpu->hi);
}

void op_slt(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
	uint32_t t = instr_t(instr);
	uint32_t d = instr_d(instr);
    
	uint32_t v = (int32_t)get_reg(cpu, s) < (int32_t)get_reg(cpu, t);

	set_reg(cpu, d, v);
}

void op_syscall(Cpu* cpu, Instruction instr) {
	exception(cpu, SYSCALL);
}

void op_mtlo(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
    
	cpu->lo = get_reg(cpu, s);
}

void op_mthi(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);

	cpu->hi = get_reg(cpu, s);
}

void op_rfe(Cpu* cpu, Instruction instr) {

	if (instr & 0x3f != 0b010000) {
		printf("Invalid cop0 instruction: %x\n", instr);
		exit(1);
	}

	uint32_t mode = cpu->sr & 0x3f;	
	cpu->sr &= ~0x3f;
	cpu->sr |= mode >> 2;
}

void op_sllv(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t d = instr_d(instr);

	uint32_t v = get_reg(cpu, t) << (get_reg(cpu, s) & 0x1f);

	set_reg(cpu, d, v);
}

void op_lh(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t i = instr_imm_se(instr);

	uint32_t addr = get_reg(cpu, s) + i;

	int16_t v = cpu_load16(cpu, addr);

	cpu->load[0] = t;
	cpu->load[1] = (uint32_t)v;
}

void op_nor(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t d = instr_d(instr);

	uint32_t v = ~(get_reg(cpu, s) | get_reg(cpu, t));
	set_reg(cpu, d, v);
}

void op_srav(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t d = instr_d(instr);

	uint32_t v = (int32_t)get_reg(cpu, t) >> (get_reg(cpu, s) & 0x1f);
	set_reg(cpu, d, v);
}

void op_srlv(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t d = instr_d(instr);

	uint32_t v = get_reg(cpu, t) >> (get_reg(cpu, s) & 0x1f);
	set_reg(cpu, d, v);
}

void op_multu(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);

	uint64_t v = (uint64_t)get_reg(cpu, t) * (uint64_t)get_reg(cpu, s);

	cpu->hi = (uint32_t)(v >> 32);
	cpu->lo = (uint32_t)v;
}

void op_xor(Cpu* cpu, Instruction instr) {
	uint32_t s = instr_s(instr);
	uint32_t t = instr_t(instr);
	uint32_t d = instr_d(instr);

	uint32_t v = get_reg(cpu, s) ^ get_reg(cpu, t);

	set_reg(cpu, d, v);
}

void op_break(Cpu* cpu, Instruction instr) {
	exception(cpu, BREAK);
}

void op_mult(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);

	uint64_t v = (int64_t)get_reg(cpu, t) * (int64_t)get_reg(cpu, s);

	cpu->hi = (uint32_t)(v >> 32);
	cpu->lo = (uint32_t)v;
}

void op_sub(Cpu* cpu, Instruction instr) {
	uint32_t t = instr_t(instr);
	uint32_t s = instr_s(instr);
	uint32_t d = instr_d(instr);

	if(check_overflow(cpu, get_reg(cpu, s), get_reg(cpu, t)) == 1) {
		return exception(cpu, OVERFLOW);		
	}
    
	uint32_t v = (int32_t)get_reg(cpu, s) - (int32_t)get_reg(cpu, t);

	set_reg(cpu, d, v);
}

void op_cop0(Cpu* cpu, Instruction instr) {
	uint32_t i = instr_s(instr);
	/* printf("cop0 instr code: %x\n", i); */
    
	switch(i) {
	case 0b00100:
		op_mtc0(cpu, instr);
		break;
	case 0b00000:
		op_mfc0(cpu, instr);
		break;
	case 0b10000:
		i = instr_subfunction(instr);

		switch(i) {
		case 0b010000:
	    op_rfe(cpu, instr);
	    break;
		default:
	    printf("unknown cop0 10 instr: %x\n", i);
	    exit(1);
		}
		break;
	default:
		printf("unkonwn cop0 instr: %x\n", i);
		exit(1);
	}
}

void op_cop1(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}

void op_cop2(Cpu* cpu, Instruction instr) {
    
	printf("unhandled gte\n");
	exit(1);
}

void op_cop3(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
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
	case 13:
		v = cpu->cause;
		break;
	case 14:
		v = cpu->epc;
		break;
	default:
		printf("unhandled mfc0 cop reg\n");
		exit(1);
	}

	cpu->load[0] = cpu_r;
	cpu->load[1] = v;
}

void op_lwc0(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}

void op_swc0(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}

void op_lwc1(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}

void op_swc1(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}

void op_lwc2(Cpu* cpu, Instruction instr) {
	printf("uhandled lwc2\n");
	exit(1);
}

void op_swc2(Cpu* cpu, Instruction instr) {
	printf("unhandled swc2\n");
	exit(1);
}

void op_lwc3(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}

void op_swc3(Cpu* cpu, Instruction instr) {
	exception(cpu, COPROCESSOR_ERROR);
}
