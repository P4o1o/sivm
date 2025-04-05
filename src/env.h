#ifndef SIVM_ENV_H
#define SIVM_ENV_H

#include <stdint.h>
#include <string.h>
#include "instruction_set.h"

#define REG_NUM 16
#define FREG_NUM 16
#define CALLSTACK_SIZE 1024
#define STACK_SIZE 2048
#define MEM_SIZE 8192
#define PROG_SIZE 4096

typedef uint64_t instr;
typedef uint32_t address;
//  0000 0000       0000 0000       0000 0000        0000 0000      0000 0000       0000 0000       0000 0000        0000 0000
//  REG-6/ADDR      REG-5/ADDR      REG-4/ADDR       REG-3/ADDR       REG-1           REG-2           REG-8            INSTR
#define INSTR_TYPE(instr) ((uint8_t) instr)
#define INSTR_ADDR(instr) (((address) (instr >> 32)))
#define INSTR_REG1(instr) (((uint8_t) (instr >> 24)))
#define INSTR_REG2(instr) (((uint8_t) (instr >> 16)))
#define INSTR_REG3(instr) (((uint8_t) (instr >> 32)))
#define INSTR_REG4(instr) (((uint8_t) (instr >> 40)))
#define INSTR_REG5(instr) (((uint8_t) (instr >> 48)))
#define INSTR_REG6(instr) (((uint8_t) (instr >> 56)))
#define INSTR_REG7(instr) (((uint8_t) (instr >> 8)))

union memblock{
    uint64_t i64;
    double f64;
};

struct Environment{
    uint64_t reg[REG_NUM];
    double freg[FREG_NUM];
    uint32_t flag;
    address prcount;
    address snext;
    address link;
    address callstack[CALLSTACK_SIZE];
    instr program[PROG_SIZE];
    union memblock stack[STACK_SIZE];
    union memblock vmem[MEM_SIZE];
};

void load_value(struct Environment *env, uint64_t *val, address start, address size);
void load_prog(struct Environment *env, instr *prog, uint64_t p_size);
void run(struct Environment *env);

#endif