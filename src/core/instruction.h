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
#define SEXT32(x)        ((int64_t)(int32_t)(x))

typedef enum {
    // Sistema
    OP_NOP      = 0x00,
    OP_HALT     = 0x01,
    OP_SYSCALL  = 0x02,
    OP_BREAK    = 0x03,
    OP_CPUID    = 0x04,
    OP_AUIPC    = 0x05,
    OP_MOVZ     = 0x06,
    OP_MOVK     = 0x07,

    // ALU Immediate
    OP_ADDI     = 0x08,
    OP_SUBI     = 0x09,
    OP_ANDI     = 0x0A,
    OP_ORI      = 0x0B,
    OP_XORI     = 0x0C,
    OP_SLTI     = 0x0D,
    OP_SLTIU    = 0x0E,
    OP_MULI     = 0x0F,
    OP_SLLI     = 0x10,
    OP_SRLI     = 0x11,
    OP_SRAI     = 0x12,
    OP_ADDWI    = 0x13,
    OP_SUBWI    = 0x14,
    OP_SLLWI    = 0x15,
    OP_SRLWI    = 0x16,
    OP_SRAWI    = 0x17,

    // Load
    OP_LB       = 0x18,
    OP_LBU      = 0x19,
    OP_LH       = 0x1A,
    OP_LHU      = 0x1B,
    OP_LW       = 0x1C,
    OP_LWU      = 0x1D,
    OP_LD       = 0x1E,
    OP_LUI      = 0x1F,

    // Store
    OP_SB       = 0x20,
    OP_SH       = 0x21,
    OP_SW       = 0x22,
    OP_SD       = 0x23,

    // Floating Point Load/Store
    OP_FLW      = 0x24,
    OP_FSW      = 0x25,

    // Branch
    OP_BEQZ     = 0x26,
    OP_BNEZ     = 0x27,
    OP_BEQ      = 0x28,
    OP_BNE      = 0x29,
    OP_BLT      = 0x2A,
    OP_BGE      = 0x2B,
    OP_BLTU     = 0x2C,
    OP_BGEU     = 0x2D,
    OP_BGTZ     = 0x2E,
    OP_BLEZ     = 0x2F,

    // Jump
    OP_JAL      = 0x30,
    OP_JALR     = 0x31,
    OP_ALU      = 0x32,
    OP_FPU      = 0x33,
    OP_ATOMIC   = 0x34,
    OP_ADDUI    = 0x35,
    OP_LEA      = 0x36,
    OP_PCREL    = 0x37,
    OP_NORI     = 0x38,
    OP_FLD      = 0x39,
    OP_FSD      = 0x3A,
    OP_BLTZ     = 0x3B,
    OP_BGEZ     = 0x3C,
    OP_JALI     = 0x3D,
    OP_FENCE    = 0x3E,
    OP_ECALL    = 0x3F,

    // ALU FUNCT codes (per OP_ALU)
    FUNCT_ADDW      = 0x40,
    FUNCT_SUBW      = 0x41,
    FUNCT_MULW      = 0x42,
    FUNCT_DIVW      = 0x43,
    FUNCT_DIVUW     = 0x44,
    FUNCT_REMW      = 0x45,
    FUNCT_REMUW     = 0x46,
    FUNCT_SLLW      = 0x47,
    FUNCT_SRLW      = 0x48,
    FUNCT_SRAW      = 0x49,

    FUNCT_NOT       = 0x50,
    FUNCT_NEG       = 0x51,
    FUNCT_CLZ       = 0x52,
    FUNCT_CTZ       = 0x53,
    FUNCT_POPCNT    = 0x54,
    FUNCT_BSWAP     = 0x55,
    FUNCT_ROL       = 0x56,
    FUNCT_ROR       = 0x57,
    FUNCT_SEXTB     = 0x58,
    FUNCT_SEXTH     = 0x59,
    FUNCT_SEXTW     = 0x5A,
    FUNCT_ZEXTH     = 0x5B,
    FUNCT_ZEXTW     = 0x5C,
    FUNCT_BREV      = 0x5D,
    FUNCT_BEXT      = 0x5E,
    FUNCT_BDEP      = 0x5F,

    FUNCT_CMOVZ     = 0x60,
    FUNCT_CMOVNZ    = 0x61,
    FUNCT_CMOVLT    = 0x62,
    FUNCT_CMOVGE    = 0x63,
    FUNCT_MIN       = 0x64,
    FUNCT_MINU      = 0x65,
    FUNCT_MAX       = 0x66,
    FUNCT_MAXU      = 0x67,
    FUNCT_ABS       = 0x68,
    FUNCT_SEQ       = 0x69,
    FUNCT_SNE       = 0x6A,
    FUNCT_SGT       = 0x6B,
    FUNCT_SGTU      = 0x6C,

    FUNCT_MULHSW    = 0x70,
    FUNCT_MULHUW    = 0x71,
    FUNCT_MADD      = 0x72,
    FUNCT_MSUB      = 0x73,
    FUNCT_BCLR      = 0x74,
    FUNCT_BINV      = 0x75,
    FUNCT_ANDN      = 0x76,
    FUNCT_ORN       = 0x77,

    // ALU core
    FUNCT_ADD       = 0x80,
    FUNCT_SUB       = 0x81,
    FUNCT_MUL       = 0x82,
    FUNCT_DIV       = 0x83,
    FUNCT_DIVU      = 0x84,
    FUNCT_REM       = 0x85,
    FUNCT_REMU      = 0x86,
    FUNCT_AND       = 0x87,
    FUNCT_OR        = 0x88,
    FUNCT_XOR       = 0x89,
    FUNCT_NOR       = 0x8A,
    FUNCT_SLL       = 0x8B,
    FUNCT_SRL       = 0x8C,
    FUNCT_SRA       = 0x8D,
    FUNCT_SLT       = 0x8E,
    FUNCT_SLTU      = 0x8F,
    FUNCT_MOV       = 0x90,
    FUNCT_MULH      = 0x91,
    FUNCT_MULHU     = 0x92,

    // FPU Arithmetic
    FUNCT_FADD      = 0xA0,
    FUNCT_FSUB      = 0xA1,
    FUNCT_FMUL      = 0xA2,
    FUNCT_FDIV      = 0xA3,
    FUNCT_FSQRT     = 0xA4,
    FUNCT_FABS      = 0xA5,
    FUNCT_FNEG      = 0xA6,
    FUNCT_FMIN      = 0xA7,
    FUNCT_FMAX      = 0xA8,
    FUNCT_FMADD     = 0xA9,
    FUNCT_FMSUB     = 0xAA,
    FUNCT_FNMADD    = 0xAB,
    FUNCT_FNMSUB    = 0xAC,
    FUNCT_FCOPYSIGN = 0xAD,
    FUNCT_FROUND    = 0xAE,
    FUNCT_FFLOOR    = 0xAF,

    // FPU Conversion
    FUNCT_FCVTW     = 0xB0,
    FUNCT_FCVTD     = 0xB1,
    FUNCT_FCVTUW    = 0xB2,
    FUNCT_FCVTDU    = 0xB3,
    FUNCT_FCVTW32   = 0xB4,
    FUNCT_FCVTD32   = 0xB5,
    FUNCT_FCEIL     = 0xB6,
    FUNCT_FTRUNC    = 0xB7,
    FUNCT_FCLASS    = 0xB8,
    FUNCT_FRINT     = 0xB9,

    // FPU Move/Transfer
    FUNCT_FMOV      = 0xC0,
    FUNCT_FMVTX     = 0xC1,
    FUNCT_FMVFX     = 0xC2,
    FUNCT_FSGNJN    = 0xC3,
    FUNCT_FSGNJX    = 0xC4,

    // FPU Compare
    FUNCT_FEQ       = 0xD0,
    FUNCT_FLT       = 0xD1,
    FUNCT_FLE       = 0xD2,
    FUNCT_FGT       = 0xD3,
    FUNCT_FGE       = 0xD4,
    FUNCT_FNEQ      = 0xD5,
    FUNCT_FORD      = 0xD6,
    FUNCT_FUNORD    = 0xD7,

    // Atomic
    FUNCT_LR        = 0xF0,
    FUNCT_SC        = 0xF1,
    FUNCT_SWAP      = 0xF2,
    FUNCT_ADD_A     = 0xF3,
    FUNCT_AND_A     = 0xF4,
    FUNCT_OR_A      = 0xF5,
    FUNCT_XOR_A     = 0xF6,
    FUNCT_MAX_A     = 0xF7,
    FUNCT_MIN_A     = 0xF8,
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


typedef enum {
    FCLASS_NEG_INF = 0,
    FCLASS_NEG_NORM = 1,
    FCLASS_NEG_SUBN = 2,
    FCLASS_NEG_ZERO = 3,
    FCLASS_POS_ZERO = 4,
    FCLASS_POS_SUBN = 5,
    FCLASS_POS_NORM = 6,
    FCLASS_POS_INF = 7,
    FCLASS_SNAN = 8,
    FCLASS_QNAN = 9
} FClass;

typedef enum {
    CPUID_VERSION = 0,
    CPUID_REG_NUM = 1,
    CPUID_FREG_NUM = 2,
    CPUID_MEM_SIZE = 3,
    CPUID_CORE_NUM = 4,
    CPUID_FEATURES = 5,
    CPUID_ENDIAN = 6
} CpuId;

#endif
