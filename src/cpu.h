#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <limits.h>

#include "platform.h"
#include "interconnect.h"
#include "instruction.h"

#define RESET 0xbfc00000
#define GARBAGE_VALUE 0xdeadbeef

typedef struct {
    uint32_t pc;    
    uint32_t sr;
    uint32_t next_pc;

    uint32_t hi;
    uint32_t lo;

    uint32_t curr_pc;
    uint32_t cause;
    uint32_t epc;

    char delay_slot;
    char branch;
    
    Interconnect* intr;
    uint32_t regs[32];

    uint32_t out[32];
    uint32_t load[2]; // addr, value
} Cpu;

typedef enum {
    SYSCALL = 0x8,
    OVERFLOW = 0xc,
    LOAD_BUS = 0x4,
    STORE_BUS = 0x5,
    BREAK = 0x9,
    COPROCESSOR_ERROR = 0xb,
    ILLEGAL = 0xa,
} Exception;

Cpu* initialize_cpu();

char check_overflow(Cpu* cpu, uint32_t v, uint32_t ov);
char check_underflow(Cpu* cpu, uint32_t);

uint32_t get_reg(Cpu* cpu, uint32_t index);
void set_reg(Cpu* cpu, uint32_t index, uint32_t v);
void decode_and_execute(Cpu* cpu, Instruction instr);
uint32_t cpu_load32(Cpu* cpu, uint32_t addr);
uint32_t cpu_load16(Cpu* cpu, uint32_t addr);
uint8_t cpu_load8(Cpu* cpu, uint32_t addr);
void cpu_store32(Cpu* cpu, uint32_t addr, uint32_t v);
void cpu_store16(Cpu* cpu, uint32_t addr, uint16_t v);
void cpu_store8(Cpu* cpu, uint32_t addr, uint8_t v);
void run_next_instruction(Cpu* cpu);
void exception(Cpu* cpu, Exception cause);

void op_secondary(Cpu* cpu, Instruction instr);
void op_bcondz(Cpu* cpu, Instruction instr);

void op_lui(Cpu* cpu, Instruction instr);
void op_ori(Cpu* cpu, Instruction instr);
void op_sw(Cpu* cpu, Instruction instr);
void op_addiu(Cpu* cpu, Instruction instr);
void op_j(Cpu* cpu, Instruction instr);
void branch(Cpu*, uint32_t offset);
void op_bne(Cpu* cpu, Instruction instr);
void op_addi(Cpu* cpu, Instruction instr);
void op_lw(Cpu* cpu, Instruction instr);
void op_sh(Cpu* cpu, Instruction instr);
void op_jal(Cpu* cpu, Instruction instr);
void op_andi(Cpu* cpu, Instruction instr);
void op_sb(Cpu* cpu, Instruction instr);
void op_jr(Cpu* cpu, Instruction instr);
void op_lb(Cpu* cpu, Instruction instr);
void op_beq(Cpu* cpu, Instruction instr);
void op_bgtz(Cpu* cpu, Instruction instr);
void op_blez(Cpu* cpu, Instruction instr);
void op_lbu(Cpu* cpu, Instruction instr);
void op_bltz(Cpu* cpu, Instruction instr);
void op_bgez(Cpu* cpu, Instruction instr);
void op_bltzal(Cpu* cpu, Instruction instr);
void op_bgezal(Cpu* cpu, Instruction instr);
void op_slti(Cpu* cpu, Instruction instr);
void op_sltiu(Cpu* cpu, Instruction instr);
void op_lhu(Cpu* cpu, Instruction instr);
void op_xori(Cpu* cpu, Instruction instr);
void op_lwl(Cpu* cpu, Instruction instr);
void op_lwr(Cpu* cpu, Instruction instr);
void op_swl(Cpu* cpu, Instruction instr);
void op_swr(Cpu* cpu, Instruction instr);
void op_illegal(Cpu* cpu, Instruction instr);

void op_sll(Cpu* cpu, Instruction instr);
void op_or(Cpu* cpu, Instruction instr);
void op_sltu(Cpu* cpu, Instruction instr);
void op_addu(Cpu* cpu, Instruction instr);
void op_and(Cpu* cpu, Instruction instr);
void op_add(Cpu* cpu, Instruction instr);
void op_jalr(Cpu* cpu, Instruction instr);
void op_subu(Cpu* cpu, Instruction instr);
void op_sra(Cpu* cpu, Instruction instr);
void op_div(Cpu* cpu, Instruction instr);
void op_mflo(Cpu* cpu, Instruction instr);
void op_srl(Cpu* cpu, Instruction instr);
void op_divu(Cpu* cpu, Instruction instr);
void op_mfhi(Cpu* cpu, Instruction instr);
void op_slt(Cpu* cpu, Instruction instr);
void op_syscall(Cpu* cpu, Instruction instr);
void op_mtlo(Cpu* cpu, Instruction instr);
void op_mthi(Cpu* cpu, Instruction instr);
void op_rfe(Cpu* cpu, Instruction instr);
void op_sllv(Cpu* cpu, Instruction instr);
void op_lh(Cpu* cpu, Instruction instr);
void op_nor(Cpu* cpu, Instruction instr);
void op_srav(Cpu* cpu, Instruction instr);
void op_srlv(Cpu* cpu, Instruction instr);
void op_multu(Cpu* cpu, Instruction instr);
void op_xor(Cpu* cpu, Instruction instr);
void op_break(Cpu* cpu, Instruction instr);
void op_mult(Cpu* cpu, Instruction instr);
void op_sub(Cpu* cpu, Instruction instr);

void op_cop0(Cpu* cpu, Instruction instr);
void op_cop1(Cpu* cpu, Instruction instr);
void op_cop2(Cpu* cpu, Instruction instr);
void op_cop3(Cpu* cpu, Instruction instr);

void op_mtc0(Cpu* cpu, Instruction instr);
void op_mfc0(Cpu* cpu, Instruction instr);
void op_lwc0(Cpu* cpu, Instruction instr);
void op_swc0(Cpu* cpu, Instruction instr);

void op_lwc1(Cpu* cpu, Instruction instr);
void op_swc1(Cpu* cpu, Instruction instr);

void op_lwc2(Cpu* cpu, Instruction instr);
void op_swc2(Cpu* cpu, Instruction instr);

void op_lwc3(Cpu* cpu, Instruction instr);
void op_swc3(Cpu* cpu, Instruction instr);

#endif
