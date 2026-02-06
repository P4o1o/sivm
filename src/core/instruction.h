#ifndef SIVM_INSTRUCTION_H
#define SIVM_INSTRUCTION_H

#include "../lib/macros.h"
#include <stdint.h>

// Formato R: opcode(6) | rd(5) | rn(5) | rm(5) | funct(11)
// Formato I: opcode(6) | rd(5) | rn(5) | imm(16)
// Formato J: opcode(6) | imm(26)
// Formato F: opcode(6) | fd(5) | fn(5) | fm(5) | funct(11)

#define INSTR_OPCODE(i)  ((uint8_t)((i) >> 26))
#define INSTR_RD(i)      ((uint8_t)(((i) >> 21) & 0x1F))
#define INSTR_RN(i)      ((uint8_t)(((i) >> 16) & 0x1F))
#define INSTR_RM(i)      ((uint8_t)(((i) >> 11) & 0x1F))
#define INSTR_FUNCT(i)   ((uint16_t)((i) & 0x7FF))
#define INSTR_IMM16(i)   ((uint16_t)((i) & 0xFFFF))
#define INSTR_IMM21(i)   ((uint32_t)((i) & 0x1FFFFF))  // For J-format with rd
#define INSTR_IMM26(i)   ((uint32_t)((i) & 0x3FFFFFF))

// Sign extension helpers
#define SIGN_EXT16(x)    ((int64_t)(int16_t)(x))
#define SIGN_EXT21(x)    (((int64_t)(x) << 43) >> 43)  // For J-format
#define SIGN_EXT26(x)    (((int64_t)(x) << 38) >> 38)

typedef enum {
    // Sistema
    OP_NOP      = 0x00,
    OP_HALT     = 0x01,
    OP_SYSCALL  = 0x02,
    OP_BREAK    = 0x03,

    // ALU Immediate
    OP_ADDI     = 0x08,
    OP_SUBI     = 0x09,
    OP_ANDI     = 0x0A,
    OP_ORI      = 0x0B,
    OP_XORI     = 0x0C,
    OP_SLTI     = 0x0D,  // Set Less Than Immediate
    OP_SLTIU    = 0x0E,  // Set Less Than Immediate Unsigned

    // Shift Immediate
    OP_SLLI     = 0x10,  // Shift Left Logical Immediate
    OP_SRLI     = 0x11,  // Shift Right Logical Immediate
    OP_SRAI     = 0x12,  // Shift Right Arithmetic Immediate

    // Load
    OP_LB       = 0x18,  // Load Byte (sign-extended)
    OP_LBU      = 0x19,  // Load Byte Unsigned
    OP_LH       = 0x1A,  // Load Halfword
    OP_LHU      = 0x1B,  // Load Halfword Unsigned
    OP_LW       = 0x1C,  // Load Word
    OP_LWU      = 0x1D,  // Load Word Unsigned
    OP_LD       = 0x1E,  // Load Doubleword
    OP_LUI      = 0x1F,  // Load Upper Immediate

    // Store
    OP_SB       = 0x20,  // Store Byte
    OP_SH       = 0x21,  // Store Halfword
    OP_SW       = 0x22,  // Store Word
    OP_SD       = 0x23,  // Store Doubleword

    // Branch
    OP_BEQ      = 0x28,  // Branch if Equal
    OP_BNE      = 0x29,  // Branch if Not Equal
    OP_BLT      = 0x2A,  // Branch if Less Than
    OP_BGE      = 0x2B,  // Branch if Greater or Equal
    OP_BLTU     = 0x2C,  // Branch if Less Than Unsigned
    OP_BGEU     = 0x2D,  // Branch if Greater or Equal Unsigned

    // Jump
    OP_JAL      = 0x30,  // Jump and Link
    OP_JALR     = 0x31,  // Jump and Link Register

    // Floating Point
    OP_FLD      = 0x39,  // Float Load Double
    OP_FSD      = 0x3A,  // Float Store Double

    // Misc
    OP_FENCE    = 0x3E,
    OP_ECALL    = 0x3F,

    // ALU FUNCT codes (per OP_ALU)
    FUNCT_ADD   = 0x80,
    FUNCT_SUB   = 0x81,
    FUNCT_MUL   = 0x82,
    FUNCT_DIV   = 0x83,
    FUNCT_DIVU  = 0x84,
    FUNCT_REM   = 0x85,
    FUNCT_REMU  = 0x86,
    FUNCT_AND   = 0x87,
    FUNCT_OR    = 0x88,
    FUNCT_XOR   = 0x89,
    FUNCT_NOR   = 0x8A,
    FUNCT_SLL   = 0x8B,  // Shift Left Logical
    FUNCT_SRL   = 0x8C,  // Shift Right Logical
    FUNCT_SRA   = 0x8D,  // Shift Right Arithmetic
    FUNCT_SLT   = 0x8E,  // Set Less Than
    FUNCT_SLTU  = 0x8F,  // Set Less Than Unsigned
    FUNCT_MOV   = 0x90,  // Move register
    FUNCT_MULH  = 0x91,  // Multiply High (signed)
    FUNCT_MULHU = 0x92,  // Multiply High (unsigned)

    // FPU FUNCT codes (per OP_FPU)
    FUNCT_FADD  = 0xA0,
    FUNCT_FSUB  = 0xA1,
    FUNCT_FMUL  = 0xA2,
    FUNCT_FDIV  = 0xA3,
    FUNCT_FSQRT = 0xA4,
    FUNCT_FABS  = 0xA5,
    FUNCT_FNEG  = 0xA6,
    FUNCT_FMIN  = 0xA7,
    FUNCT_FMAX  = 0xA8,
    FUNCT_FCVTW = 0xB0,  // Convert to int
    FUNCT_FCVTD = 0xB1,  // Convert from int
    FUNCT_FMOV  = 0xC0,  // Move float register
    FUNCT_FEQ   = 0xD0,  // Float Equal
    FUNCT_FLT   = 0xD1,  // Float Less Than
    FUNCT_FLE   = 0xD2,  // Float Less or Equal

    // Atomic FUNCT codes (per OP_ATOMIC)
    FUNCT_LR    = 0xF0,  // Load Reserved
    FUNCT_SC    = 0xF1,  // Store Conditional
    FUNCT_SWAP  = 0xF2,  // Atomic Swap
    FUNCT_ADD_A = 0xF3,  // Atomic Add
    FUNCT_AND_A = 0xF4,  // Atomic And
    FUNCT_OR_A  = 0xF5,  // Atomic Or
    FUNCT_XOR_A = 0xF6,  // Atomic Xor
    FUNCT_MAX_A = 0xF7,  // Atomic Max
    FUNCT_MIN_A = 0xF8,  // Atomic Min
} Opcode;

typedef enum {
    SYS_EXIT    = 0,
    SYS_READ    = 1,
    SYS_WRITE   = 2,
    SYS_OPEN    = 3,
    SYS_CLOSE   = 4,
    SYS_BRK     = 5,
    SYS_MMAP    = 6,
    SYS_YIELD   = 10,
    SYS_GETPID  = 20,
    SYS_TIME    = 30,
} Syscall;

#endif
