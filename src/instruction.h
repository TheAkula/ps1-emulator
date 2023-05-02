#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

typedef uint32_t Instruction;

// 31 : 26
uint32_t instr_function(Instruction instr) {    
    return instr >> 26;
}

// 25 : 21
uint32_t instr_s(Instruction instr) {    
    return (instr >> 21) & 0x1f;
}

// 25 : 0
uint32_t instr_imm_jump(Instruction instr) {
    return instr & 0x3ffffff;
}

// 20 : 16
uint32_t instr_t(Instruction instr) {    
    return (instr >> 16) & 0x1f;
}

// 15 : 11
uint32_t instr_d(Instruction instr) {
    return (instr >> 11) & 0x1f;
}

// 15 : 0
uint32_t instr_imm(Instruction instr) {
    return instr & 0xffff;
}

// 15 : 0
uint32_t instr_imm_se(Instruction instr) {
    return (uint32_t)(int16_t)(instr & 0xffff);
}

// 10 : 6
uint32_t instr_shift(Instruction instr) {
    return (instr >> 6) & 0x1f; 
}

// 5 : 0
uint32_t instr_subfunction(Instruction instr) {
    return instr & 0x3f;
}

#endif
