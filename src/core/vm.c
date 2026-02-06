#include "vm.h"


// ===== VM MEMORY ACCESS MACROS =====
// Tutte le macro usano goto L_VM_ERR_INVALID_ADDR per error path.
// Zero overhead nel fast path (solo UNLIKELY branch).

#define CHECK_ADDR(addr, size) (((addr) + (size)) <= MEM_SIZE)

#define store8(vm, addr, val) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 1))) goto L_VM_ERR_INVALID_ADDR; \
    (vm)->mem[(addr)] = (val); \
} while(0)

#define store16(vm, addr, val) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 2))) goto L_VM_ERR_INVALID_ADDR; \
    uint16_t _v = LE16(val); \
    memcpy(&(vm)->mem[(addr)], &_v, 2); \
} while(0)

#define store32(vm, addr, val) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 4))) goto L_VM_ERR_INVALID_ADDR; \
    uint32_t _v = LE32(val); \
    memcpy(&(vm)->mem[(addr)], &_v, 4); \
} while(0)

#define store64(vm, addr, val) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 8))) goto L_VM_ERR_INVALID_ADDR; \
    uint64_t _v = LE64(val); \
    memcpy(&(vm)->mem[(addr)], &_v, 8); \
} while(0)

#define load8(vm, addr, out) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 1))) goto L_VM_ERR_INVALID_ADDR; \
    *(out) = (vm)->mem[(addr)]; \
} while(0)

#define load16(vm, addr, out) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 2))) goto L_VM_ERR_INVALID_ADDR; \
    memcpy((out), &(vm)->mem[(addr)], 2); \
    *(out) = LE16(*(out)); \
} while(0)

#define load32(vm, addr, out) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 4))) goto L_VM_ERR_INVALID_ADDR; \
    memcpy((out), &(vm)->mem[(addr)], 4); \
    *(out) = LE32(*(out)); \
} while(0)

#define load64(vm, addr, out) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 8))) goto L_VM_ERR_INVALID_ADDR; \
    memcpy((out), &(vm)->mem[(addr)], 8); \
    *(out) = LE64(*(out)); \
} while(0)

// Float load/store — single bounds+alignment check, nessun doppio controllo
#define store_double(vm, addr, val) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 8) || ((addr) & 7))) goto L_VM_ERR_INVALID_ADDR; \
    uint64_t _fb; \
    memcpy(&_fb, &(val), 8); \
    _fb = LE64(_fb); \
    memcpy(&(vm)->mem[(addr)], &_fb, 8); \
} while(0)

#define load_double(vm, addr, out) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 8) || ((addr) & 7))) goto L_VM_ERR_INVALID_ADDR; \
    uint64_t _fb; \
    memcpy(&_fb, &(vm)->mem[(addr)], 8); \
    _fb = LE64(_fb); \
    memcpy((out), &_fb, 8); \
} while(0)

#define store_float(vm, addr, fval) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 4) || ((addr) & 3))) goto L_VM_ERR_INVALID_ADDR; \
    float _fv = (float)(fval); \
    uint32_t _fb; \
    memcpy(&_fb, &_fv, 4); \
    _fb = LE32(_fb); \
    memcpy(&(vm)->mem[(addr)], &_fb, 4); \
} while(0)

#define load_float(vm, addr, out) do { \
    if (UNLIKELY(!CHECK_ADDR((addr), 4) || ((addr) & 3))) goto L_VM_ERR_INVALID_ADDR; \
    uint32_t _fb; \
    memcpy(&_fb, &(vm)->mem[(addr)], 4); \
    _fb = LE32(_fb); \
    float _fv; \
    memcpy(&_fv, &_fb, 4); \
    *(out) = (double)_fv; \
} while(0)


static force_inline uint64_t clz64(uint64_t x) {
    if (x == 0) return 64;
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    return (uint64_t)__builtin_clzll(x);
#else
    uint64_t n = 0;
    if (!(x & 0xFFFFFFFF00000000ULL)) { n += 32; x <<= 32; }
    if (!(x & 0xFFFF000000000000ULL)) { n += 16; x <<= 16; }
    if (!(x & 0xFF00000000000000ULL)) { n +=  8; x <<=  8; }
    if (!(x & 0xF000000000000000ULL)) { n +=  4; x <<=  4; }
    if (!(x & 0xC000000000000000ULL)) { n +=  2; x <<=  2; }
    if (!(x & 0x8000000000000000ULL)) { n +=  1; }
    return n;
#endif
}

static force_inline uint64_t ctz64(uint64_t x) {
    if (x == 0) return 64;
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    return (uint64_t)__builtin_ctzll(x);
#else
    uint64_t n = 0;
    if (!(x & 0x00000000FFFFFFFFULL)) { n += 32; x >>= 32; }
    if (!(x & 0x000000000000FFFFULL)) { n += 16; x >>= 16; }
    if (!(x & 0x00000000000000FFULL)) { n +=  8; x >>=  8; }
    if (!(x & 0x000000000000000FULL)) { n +=  4; x >>=  4; }
    if (!(x & 0x0000000000000003ULL)) { n +=  2; x >>=  2; }
    if (!(x & 0x0000000000000001ULL)) { n +=  1; }
    return n;
#endif
}

static force_inline uint64_t popcnt64(uint64_t x) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    return (uint64_t)__builtin_popcountll(x);
#else
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (x * 0x0101010101010101ULL) >> 56;
#endif
}

static force_inline uint64_t brev64(uint64_t x) {
    x = ((x & 0x5555555555555555ULL) << 1)  | ((x >> 1)  & 0x5555555555555555ULL);
    x = ((x & 0x3333333333333333ULL) << 2)  | ((x >> 2)  & 0x3333333333333333ULL);
    x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((x >> 4)  & 0x0F0F0F0F0F0F0F0FULL);
    return BSWAP64(x);
}

static force_inline uint64_t fclass_double(double d) {
    uint64_t bits;
    memcpy(&bits, &d, 8);
    uint64_t sign = bits >> 63;
    uint64_t exp  = (bits >> 52) & 0x7FF;
    uint64_t frac = bits & 0x000FFFFFFFFFFFFFULL;

    if (exp == 0x7FF) {
        if (frac == 0) return sign ? FCLASS_NEG_INF : FCLASS_POS_INF;
        return (frac & (1ULL << 51)) ? FCLASS_QNAN : FCLASS_SNAN;
    }
    if (exp == 0) {
        if (frac == 0) return sign ? FCLASS_NEG_ZERO : FCLASS_POS_ZERO;
        return sign ? FCLASS_NEG_SUBN : FCLASS_POS_SUBN;
    }
    return sign ? FCLASS_NEG_NORM : FCLASS_POS_NORM;
}

#define fetch(env, core) do { \
    address _fpc = (core)->pc; \
    load32((env), _fpc, &instruction); \
    (core)->pc += 4; \
    (core)->instret++; \
} while(0)

// Ensure r0 is always zero
#define WRITE_REG(core, rd, val) do { if ((rd) != REG_ZERO) (core)->reg[rd] = (val); } while(0)

// Atomic lock helpers
#define MEM_LOCK(env)   do { while (atomic_flag_test_and_set(&(env)->mem_lock)) cpu_pause(); } while(0)
#define MEM_UNLOCK(env) atomic_flag_clear(&(env)->mem_lock)

// ===== DISPATCH MACROS =====
// Computed goto: flat table[256], opcode e funct nella stessa tabella
// Switch: stesso switch, OP_ALU/FPU/ATOMIC fanno goto L_SWITCH_ENTRY con funct come key
#if COMPUTED_GOTO_SUPPORTED
    #define DISPATCH_START  core->cycles++; fetch(env, core); goto *dispatch_table[INSTR_OPCODE(instruction)];
    #define DISPATCH_NEXT   DISPATCH_START
    #define INSTR_CASE(op)  L_##op:
    #define INSTR_DEFAULT   L_DEFAULT:
    #define END_SYSCALL     DISPATCH_NEXT
#else
// Fetch instruction at the beginning of the loop for the switch version.
    #define DISPATCH_START  L_START_EXECUTION: while(1) { fetch(env, core); switch(INSTR_OPCODE(instruction)) {
    #define DISPATCH_NEXT   break;
    #define INSTR_CASE(op)  case op:
    #define INSTR_DEFAULT   default:
    #define END_SYSCALL     core->cycles++; goto L_START_EXECUTION
#endif

void vm_init(struct VM *vm) {
    memset(vm, 0, sizeof(struct VM));
    for (int i = 0; i < CORE_NUM; i++) {
        vm->core[i].reg[REG_SP] = MEM_SIZE - (i * STACK_SIZE);
        vm->core[i].pc = 0;
        vm->core[i].status = VM_OK;
        vm->core[i].cycles = 0;
    }
}

void run(struct VM *env, uint64_t core_num) {
    struct Core *core = &env->core[core_num];
    instr instruction;
#if COMPUTED_GOTO_SUPPORTED
    static void* dispatch_table[256] = {
        // ===== OPCODES [0x00-0x3F] =====
        [OP_NOP]       = &&L_OP_NOP,
        [OP_HALT]      = &&L_OP_HALT,
        [OP_SYSCALL]   = &&L_OP_SYSCALL,
        [OP_BREAK]     = &&L_OP_BREAK,
        [OP_CPUID]     = &&L_OP_CPUID,
        [OP_AUIPC]     = &&L_OP_AUIPC,
        [OP_MOVZ]      = &&L_OP_MOVZ,
        [OP_MOVK]      = &&L_OP_MOVK,
        [OP_ADDI]      = &&L_OP_ADDI,
        [OP_SUBI]      = &&L_OP_SUBI,
        [OP_ANDI]      = &&L_OP_ANDI,
        [OP_ORI]       = &&L_OP_ORI,
        [OP_XORI]      = &&L_OP_XORI,
        [OP_SLTI]      = &&L_OP_SLTI,
        [OP_SLTIU]     = &&L_OP_SLTIU,
        [OP_MULI]      = &&L_OP_MULI,
        [OP_SLLI]      = &&L_OP_SLLI,
        [OP_SRLI]      = &&L_OP_SRLI,
        [OP_SRAI]      = &&L_OP_SRAI,
        [OP_ADDWI]     = &&L_OP_ADDWI,
        [OP_SUBWI]     = &&L_OP_SUBWI,
        [OP_SLLWI]     = &&L_OP_SLLWI,
        [OP_SRLWI]     = &&L_OP_SRLWI,
        [OP_SRAWI]     = &&L_OP_SRAWI,
        [OP_LB]        = &&L_OP_LB,
        [OP_LBU]       = &&L_OP_LBU,
        [OP_LH]        = &&L_OP_LH,
        [OP_LHU]       = &&L_OP_LHU,
        [OP_LW]        = &&L_OP_LW,
        [OP_LWU]       = &&L_OP_LWU,
        [OP_LD]        = &&L_OP_LD,
        [OP_LUI]       = &&L_OP_LUI,
        [OP_SB]        = &&L_OP_SB,
        [OP_SH]        = &&L_OP_SH,
        [OP_SW]        = &&L_OP_SW,
        [OP_SD]        = &&L_OP_SD,
        [OP_FLW]       = &&L_OP_FLW,
        [OP_FSW]       = &&L_OP_FSW,
        [OP_BEQZ]      = &&L_OP_BEQZ,
        [OP_BNEZ]      = &&L_OP_BNEZ,
        [OP_BEQ]       = &&L_OP_BEQ,
        [OP_BNE]       = &&L_OP_BNE,
        [OP_BLT]       = &&L_OP_BLT,
        [OP_BGE]       = &&L_OP_BGE,
        [OP_BLTU]      = &&L_OP_BLTU,
        [OP_BGEU]      = &&L_OP_BGEU,
        [OP_BGTZ]      = &&L_OP_BGTZ,
        [OP_BLEZ]      = &&L_OP_BLEZ,
        [OP_JAL]       = &&L_OP_JAL,
        [OP_JALR]      = &&L_OP_JALR,
        [OP_ALU]       = &&L_OP_ALU,
        [OP_FPU]       = &&L_OP_FPU,
        [OP_ATOMIC]    = &&L_OP_ATOMIC,
        [OP_ADDUI]     = &&L_OP_ADDUI,
        [OP_LEA]       = &&L_OP_LEA,
        [OP_PCREL]     = &&L_OP_PCREL,
        [OP_NORI]      = &&L_OP_NORI,
        [OP_FLD]       = &&L_OP_FLD,
        [OP_FSD]       = &&L_OP_FSD,
        [OP_BLTZ]      = &&L_OP_BLTZ,
        [OP_BGEZ]      = &&L_OP_BGEZ,
        [OP_JALI]      = &&L_OP_JALI,
        [OP_FENCE]     = &&L_OP_FENCE,
        [OP_ECALL]     = &&L_OP_ECALL,
        // ===== FUNCT CODES [0x40-0xFF] =====
        // Word ALU [0x40-0x49]
        [FUNCT_ADDW]   = &&L_FUNCT_ADDW,
        [FUNCT_SUBW]   = &&L_FUNCT_SUBW,
        [FUNCT_MULW]   = &&L_FUNCT_MULW,
        [FUNCT_DIVW]   = &&L_FUNCT_DIVW,
        [FUNCT_DIVUW]  = &&L_FUNCT_DIVUW,
        [FUNCT_REMW]   = &&L_FUNCT_REMW,
        [FUNCT_REMUW]  = &&L_FUNCT_REMUW,
        [FUNCT_SLLW]   = &&L_FUNCT_SLLW,
        [FUNCT_SRLW]   = &&L_FUNCT_SRLW,
        [FUNCT_SRAW]   = &&L_FUNCT_SRAW,
        [0x4A ... 0x4F]= &&L_DEFAULT,
        // Bit Manipulation [0x50-0x5F]
        [FUNCT_NOT]    = &&L_FUNCT_NOT,
        [FUNCT_NEG]    = &&L_FUNCT_NEG,
        [FUNCT_CLZ]    = &&L_FUNCT_CLZ,
        [FUNCT_CTZ]    = &&L_FUNCT_CTZ,
        [FUNCT_POPCNT] = &&L_FUNCT_POPCNT,
        [FUNCT_BSWAP]  = &&L_FUNCT_BSWAP,
        [FUNCT_ROL]    = &&L_FUNCT_ROL,
        [FUNCT_ROR]    = &&L_FUNCT_ROR,
        [FUNCT_SEXTB]  = &&L_FUNCT_SEXTB,
        [FUNCT_SEXTH]  = &&L_FUNCT_SEXTH,
        [FUNCT_SEXTW]  = &&L_FUNCT_SEXTW,
        [FUNCT_ZEXTH]  = &&L_FUNCT_ZEXTH,
        [FUNCT_ZEXTW]  = &&L_FUNCT_ZEXTW,
        [FUNCT_BREV]   = &&L_FUNCT_BREV,
        [FUNCT_BEXT]   = &&L_FUNCT_BEXT,
        [FUNCT_BDEP]   = &&L_FUNCT_BDEP,
        // Conditional/MinMax [0x60-0x6C]
        [FUNCT_CMOVZ]  = &&L_FUNCT_CMOVZ,
        [FUNCT_CMOVNZ] = &&L_FUNCT_CMOVNZ,
        [FUNCT_CMOVLT] = &&L_FUNCT_CMOVLT,
        [FUNCT_CMOVGE] = &&L_FUNCT_CMOVGE,
        [FUNCT_MIN]    = &&L_FUNCT_MIN,
        [FUNCT_MINU]   = &&L_FUNCT_MINU,
        [FUNCT_MAX]    = &&L_FUNCT_MAX,
        [FUNCT_MAXU]   = &&L_FUNCT_MAXU,
        [FUNCT_ABS]    = &&L_FUNCT_ABS,
        [FUNCT_SEQ]    = &&L_FUNCT_SEQ,
        [FUNCT_SNE]    = &&L_FUNCT_SNE,
        [FUNCT_SGT]    = &&L_FUNCT_SGT,
        [FUNCT_SGTU]   = &&L_FUNCT_SGTU,
        [0x6D ... 0x6F]= &&L_DEFAULT,
        // Wide multiply / bit extra [0x70-0x77]
        [FUNCT_MULHSW] = &&L_FUNCT_MULHSW,
        [FUNCT_MULHUW] = &&L_FUNCT_MULHUW,
        [FUNCT_MADD]   = &&L_FUNCT_MADD,
        [FUNCT_MSUB]   = &&L_FUNCT_MSUB,
        [FUNCT_BCLR]   = &&L_FUNCT_BCLR,
        [FUNCT_BINV]   = &&L_FUNCT_BINV,
        [FUNCT_ANDN]   = &&L_FUNCT_ANDN,
        [FUNCT_ORN]    = &&L_FUNCT_ORN,
        [0x78 ... 0x7F]= &&L_DEFAULT,
        // ALU core [0x80-0x92]
        [FUNCT_ADD]    = &&L_FUNCT_ADD,
        [FUNCT_SUB]    = &&L_FUNCT_SUB,
        [FUNCT_MUL]    = &&L_FUNCT_MUL,
        [FUNCT_DIV]    = &&L_FUNCT_DIV,
        [FUNCT_DIVU]   = &&L_FUNCT_DIVU,
        [FUNCT_REM]    = &&L_FUNCT_REM,
        [FUNCT_REMU]   = &&L_FUNCT_REMU,
        [FUNCT_AND]    = &&L_FUNCT_AND,
        [FUNCT_OR]     = &&L_FUNCT_OR,
        [FUNCT_XOR]    = &&L_FUNCT_XOR,
        [FUNCT_NOR]    = &&L_FUNCT_NOR,
        [FUNCT_SLL]    = &&L_FUNCT_SLL,
        [FUNCT_SRL]    = &&L_FUNCT_SRL,
        [FUNCT_SRA]    = &&L_FUNCT_SRA,
        [FUNCT_SLT]    = &&L_FUNCT_SLT,
        [FUNCT_SLTU]   = &&L_FUNCT_SLTU,
        [FUNCT_MOV]    = &&L_FUNCT_MOV,
        [FUNCT_MULH]   = &&L_FUNCT_MULH,
        [FUNCT_MULHU]  = &&L_FUNCT_MULHU,
        [0x93 ... 0x9F]= &&L_DEFAULT,
        // FPU Arithmetic [0xA0-0xAF]
        [FUNCT_FADD]   = &&L_FUNCT_FADD,
        [FUNCT_FSUB]   = &&L_FUNCT_FSUB,
        [FUNCT_FMUL]   = &&L_FUNCT_FMUL,
        [FUNCT_FDIV]   = &&L_FUNCT_FDIV,
        [FUNCT_FSQRT]  = &&L_FUNCT_FSQRT,
        [FUNCT_FABS]   = &&L_FUNCT_FABS,
        [FUNCT_FNEG]   = &&L_FUNCT_FNEG,
        [FUNCT_FMIN]   = &&L_FUNCT_FMIN,
        [FUNCT_FMAX]   = &&L_FUNCT_FMAX,
        [FUNCT_FMADD]  = &&L_FUNCT_FMADD,
        [FUNCT_FMSUB]  = &&L_FUNCT_FMSUB,
        [FUNCT_FNMADD] = &&L_FUNCT_FNMADD,
        [FUNCT_FNMSUB] = &&L_FUNCT_FNMSUB,
        [FUNCT_FCOPYSIGN] = &&L_FUNCT_FCOPYSIGN,
        [FUNCT_FROUND] = &&L_FUNCT_FROUND,
        [FUNCT_FFLOOR] = &&L_FUNCT_FFLOOR,
        // FPU Conversion [0xB0-0xB9]
        [FUNCT_FCVTW]  = &&L_FUNCT_FCVTW,
        [FUNCT_FCVTD]  = &&L_FUNCT_FCVTD,
        [FUNCT_FCVTUW] = &&L_FUNCT_FCVTUW,
        [FUNCT_FCVTDU] = &&L_FUNCT_FCVTDU,
        [FUNCT_FCVTW32]= &&L_FUNCT_FCVTW32,
        [FUNCT_FCVTD32]= &&L_FUNCT_FCVTD32,
        [FUNCT_FCEIL]  = &&L_FUNCT_FCEIL,
        [FUNCT_FTRUNC] = &&L_FUNCT_FTRUNC,
        [FUNCT_FCLASS] = &&L_FUNCT_FCLASS,
        [FUNCT_FRINT]  = &&L_FUNCT_FRINT,
        [0xBA ... 0xBF]= &&L_DEFAULT,
        // FPU Move/Transfer [0xC0-0xC4]
        [FUNCT_FMOV]   = &&L_FUNCT_FMOV,
        [FUNCT_FMVTX]  = &&L_FUNCT_FMVTX,
        [FUNCT_FMVFX]  = &&L_FUNCT_FMVFX,
        [FUNCT_FSGNJN] = &&L_FUNCT_FSGNJN,
        [FUNCT_FSGNJX] = &&L_FUNCT_FSGNJX,
        [0xC5 ... 0xCF]= &&L_DEFAULT,
        // FPU Compare [0xD0-0xD7]
        [FUNCT_FEQ]    = &&L_FUNCT_FEQ,
        [FUNCT_FLT]    = &&L_FUNCT_FLT,
        [FUNCT_FLE]    = &&L_FUNCT_FLE,
        [FUNCT_FGT]    = &&L_FUNCT_FGT,
        [FUNCT_FGE]    = &&L_FUNCT_FGE,
        [FUNCT_FNEQ]   = &&L_FUNCT_FNEQ,
        [FUNCT_FORD]   = &&L_FUNCT_FORD,
        [FUNCT_FUNORD] = &&L_FUNCT_FUNORD,
        [0xD8 ... 0xEF]= &&L_DEFAULT,
        // Atomic [0xF0-0xF8]
        [FUNCT_LR]     = &&L_FUNCT_LR,
        [FUNCT_SC]     = &&L_FUNCT_SC,
        [FUNCT_SWAP]   = &&L_FUNCT_SWAP,
        [FUNCT_ADD_A]  = &&L_FUNCT_ADD_A,
        [FUNCT_AND_A]  = &&L_FUNCT_AND_A,
        [FUNCT_OR_A]   = &&L_FUNCT_OR_A,
        [FUNCT_XOR_A]  = &&L_FUNCT_XOR_A,
        [FUNCT_MAX_A]  = &&L_FUNCT_MAX_A,
        [FUNCT_MIN_A]  = &&L_FUNCT_MIN_A,
        [0xF9 ... 0xFF]= &&L_DEFAULT,
    };
#endif    
DISPATCH_START
    // ========== SYSTEM INSTRUCTIONS ==========
    INSTR_CASE(OP_NOP)
        core->cycles++;
        DISPATCH_NEXT

    INSTR_CASE(OP_HALT)
        goto L_VM_ERR_HALTED;

    INSTR_CASE(OP_SYSCALL)
        goto L_VM_SYSCALL;

    INSTR_CASE(OP_BREAK)
        goto L_VM_ERR_BREAKPOINT;

    INSTR_CASE(OP_CPUID)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint16_t sel = INSTR_IMM16(instruction);
            uint64_t val = 0;
            switch (sel) {
                case CPUID_VERSION:  val = 0x00020000; break; // v2.0
                case CPUID_REG_NUM:  val = REG_NUM; break;
                case CPUID_FREG_NUM: val = FREG_NUM; break;
                case CPUID_MEM_SIZE: val = MEM_SIZE; break;
                case CPUID_CORE_NUM: val = CORE_NUM; break;
                case CPUID_FEATURES: val = 0x07; break; // ALU+FPU+ATOMIC
                case CPUID_ENDIAN:   val = ENDIAN; break;
            }
            WRITE_REG(core, rd, val);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_AUIPC)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t imm = SIGN_EXT21(INSTR_IMM21(instruction));
            WRITE_REG(core, rd, (core->pc - 4) + (imm << 16));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_MOVZ)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t hw = INSTR_RN(instruction) & 0x03;
            uint64_t val = (uint64_t)INSTR_IMM16(instruction) << (hw * 16);
            WRITE_REG(core, rd, val);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_MOVK)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t hw = INSTR_RN(instruction) & 0x03;
            uint8_t shift = hw * 16;
            uint64_t mask = ~(0xFFFFULL << shift);
            uint64_t val = (core->reg[rd] & mask) | ((uint64_t)INSTR_IMM16(instruction) << shift);
            WRITE_REG(core, rd, val);
        }
        DISPATCH_NEXT

    // ========== ALU IMMEDIATE ==========
    INSTR_CASE(OP_ADDI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, core->reg[rn] + imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SUBI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, core->reg[rn] - imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_ANDI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint64_t imm = INSTR_IMM16(instruction);
            WRITE_REG(core, rd, core->reg[rn] & imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_ORI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint64_t imm = INSTR_IMM16(instruction);
            WRITE_REG(core, rd, core->reg[rn] | imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_XORI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint64_t imm = INSTR_IMM16(instruction);
            WRITE_REG(core, rd, core->reg[rn] ^ imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SLTI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, ((int64_t)core->reg[rn] < imm) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SLTIU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint64_t imm = INSTR_IMM16(instruction);
            WRITE_REG(core, rd, (core->reg[rn] < imm) ? 1 : 0);
        }
        DISPATCH_NEXT
    
    INSTR_CASE(OP_MULI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, (uint64_t)((int64_t)core->reg[rn] * imm));
        }
        DISPATCH_NEXT

    // ========== SHIFT IMMEDIATE ==========
    INSTR_CASE(OP_SLLI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint8_t shamt = INSTR_IMM16(instruction) & 0x3F;
            WRITE_REG(core, rd, core->reg[rn] << shamt);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SRLI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint8_t shamt = INSTR_IMM16(instruction) & 0x3F;
            WRITE_REG(core, rd, core->reg[rn] >> shamt);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SRAI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint8_t shamt = INSTR_IMM16(instruction) & 0x3F;
            WRITE_REG(core, rd, (uint64_t)((int64_t)core->reg[rn] >> shamt));
        }
        DISPATCH_NEXT
    
    // ========== WORD (32-BIT) IMMEDIATE ==========
    INSTR_CASE(OP_ADDWI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[rn] + (uint32_t)imm));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SUBWI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[rn] - (uint32_t)imm));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SLLWI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint8_t shamt = INSTR_IMM16(instruction) & 0x1F;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[rn] << shamt));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SRLWI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint8_t shamt = INSTR_IMM16(instruction) & 0x1F;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[rn] >> shamt));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SRAWI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint8_t shamt = INSTR_IMM16(instruction) & 0x1F;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)((int32_t)(uint32_t)core->reg[rn] >> shamt)));
        }
        DISPATCH_NEXT

    // ========== LOAD INSTRUCTIONS ==========
    INSTR_CASE(OP_LB)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint8_t tmp;
            load8(env, addr, &tmp);
            WRITE_REG(core, rd, (uint64_t)(int64_t)(int8_t)tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LBU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint8_t tmp;
            load8(env, addr, &tmp);
            WRITE_REG(core, rd, (uint64_t)tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LH)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint16_t tmp;
            load16(env, addr, &tmp);
            WRITE_REG(core, rd, (uint64_t)(int64_t)(int16_t)tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LHU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint16_t tmp;
            load16(env, addr, &tmp);
            WRITE_REG(core, rd, (uint64_t)tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint32_t tmp;
            load32(env, addr, &tmp);
            WRITE_REG(core, rd, (uint64_t)(int64_t)(int32_t)tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LWU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint32_t tmp;
            load32(env, addr, &tmp);
            WRITE_REG(core, rd, (uint64_t)tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LD)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            uint64_t tmp;
            load64(env, addr, &tmp);
            WRITE_REG(core, rd, tmp);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LUI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t imm = (uint64_t)INSTR_IMM16(instruction) << 16;
            WRITE_REG(core, rd, imm);
        }
        DISPATCH_NEXT

    // ========== STORE INSTRUCTIONS ==========
    INSTR_CASE(OP_SB)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            store8(env, addr, (uint8_t)core->reg[rd]);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SH)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            store16(env, addr, (uint16_t)core->reg[rd]);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            store32(env, addr, (uint32_t)core->reg[rd]);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_SD)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            store64(env, addr, core->reg[rd]);
        }
        DISPATCH_NEXT

    // ========== FLOAT LOAD/STORE (single precision) ==========
    INSTR_CASE(OP_FLW)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            load_float(env, addr, &core->freg[fd]);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_FSW)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            store_float(env, addr, core->freg[fd]);
        }
        DISPATCH_NEXT

    // ========== BRANCH INSTRUCTIONS ==========

    // ========== COMPACT BRANCH (compare to zero) ==========
    INSTR_CASE(OP_BEQZ)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] == 0)
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BNEZ)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] != 0)
                core->pc += offset - 4;
        }
        DISPATCH_NEXT


    INSTR_CASE(OP_BEQ)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] == core->reg[rn])
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BNE)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] != core->reg[rn])
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BLT)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if ((int64_t)core->reg[rd] < (int64_t)core->reg[rn])
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BGE)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if ((int64_t)core->reg[rd] >= (int64_t)core->reg[rn])
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BLTU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] < core->reg[rn])
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BGEU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] >= core->reg[rn])
                core->pc += offset - 4;
        }
        DISPATCH_NEXT
    
    INSTR_CASE(OP_BGTZ)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if ((int64_t)core->reg[rd] > 0)
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BLEZ)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if ((int64_t)core->reg[rd] <= 0)
                core->pc += offset - 4;
        }
        DISPATCH_NEXT

    // ========== JUMP INSTRUCTIONS ==========
    INSTR_CASE(OP_JAL)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t offset = SIGN_EXT21(INSTR_IMM21(instruction)) << 2;
            WRITE_REG(core, rd, core->pc);
            core->pc += offset - 4;
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_JALR)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address target = (core->reg[rn] + offset) & ~1ULL;
            WRITE_REG(core, rd, core->pc);
            core->pc = target;
        }
        DISPATCH_NEXT

    // ========== EXTENDED OPCODES ==========
    INSTR_CASE(OP_ADDUI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint64_t imm = INSTR_IMM16(instruction);
            WRITE_REG(core, rd, core->reg[rn] + imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LEA)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t imm = SIGN_EXT16(INSTR_IMM16(instruction));
            WRITE_REG(core, rd, core->reg[rn] + imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_PCREL)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t imm = SIGN_EXT21(INSTR_IMM21(instruction));
            WRITE_REG(core, rd, (core->pc - 4) + imm);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_NORI)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            uint64_t imm = INSTR_IMM16(instruction);
            WRITE_REG(core, rd, ~(core->reg[rn] | imm));
        }
        DISPATCH_NEXT

    // ========== FLOAT LOAD/STORE ==========
    INSTR_CASE(OP_FLD)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            load_double(env, addr, &core->freg[fd]);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_FSD)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            store_double(env, addr, core->freg[fd]);
        }
        DISPATCH_NEXT

    // ========== MISC ==========
    INSTR_CASE(OP_FENCE)
        atomic_fence_seq_cst();
        DISPATCH_NEXT

    INSTR_CASE(OP_ECALL)
        goto L_VM_SYSCALL;

    // ========== ALU FUNCT ==========
    INSTR_CASE(FUNCT_ADDW)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[INSTR_RN(instruction)] + (uint32_t)core->reg[INSTR_RM(instruction)]));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SUBW)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[INSTR_RN(instruction)] - (uint32_t)core->reg[INSTR_RM(instruction)]));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MULW)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)((int32_t)(uint32_t)core->reg[INSTR_RN(instruction)] * (int32_t)(uint32_t)core->reg[INSTR_RM(instruction)])));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_DIVW)
        {
            uint8_t rd = INSTR_RD(instruction);
            int32_t b = (int32_t)(uint32_t)core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)((int32_t)(uint32_t)core->reg[INSTR_RN(instruction)] / b)));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_DIVUW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint32_t b = (uint32_t)core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[INSTR_RN(instruction)] / b));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_REMW)
        {
            uint8_t rd = INSTR_RD(instruction);
            int32_t b = (int32_t)(uint32_t)core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)((int32_t)(uint32_t)core->reg[INSTR_RN(instruction)] % b)));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_REMUW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint32_t b = (uint32_t)core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[INSTR_RN(instruction)] % b));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SLLW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t shamt = core->reg[INSTR_RM(instruction)] & 0x1F;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[INSTR_RN(instruction)] << shamt));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SRLW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t shamt = core->reg[INSTR_RM(instruction)] & 0x1F;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)core->reg[INSTR_RN(instruction)] >> shamt));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SRAW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t shamt = core->reg[INSTR_RM(instruction)] & 0x1F;
            WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)((int32_t)(uint32_t)core->reg[INSTR_RN(instruction)] >> shamt)));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ADD)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] + core->reg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SUB)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] - core->reg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MUL)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] * core->reg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_DIV)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t b = core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, (uint64_t)((int64_t)core->reg[INSTR_RN(instruction)] / (int64_t)b));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_DIVU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t b = core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] / b);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_REM)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t b = core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, (uint64_t)((int64_t)core->reg[INSTR_RN(instruction)] % (int64_t)b));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_REMU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t b = core->reg[INSTR_RM(instruction)];
            if (UNLIKELY(!b)) goto L_VM_ERR_DIV_BY_ZERO;
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] % b);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_AND)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] & core->reg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_OR)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] | core->reg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_XOR)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] ^ core->reg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_NOR)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, ~(core->reg[INSTR_RN(instruction)] | core->reg[INSTR_RM(instruction)]));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SLL)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] << (core->reg[INSTR_RM(instruction)] & 0x3F));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SRL)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] >> (core->reg[INSTR_RM(instruction)] & 0x3F));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SRA)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (uint64_t)((int64_t)core->reg[INSTR_RN(instruction)] >> (core->reg[INSTR_RM(instruction)] & 0x3F)));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SLT)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, ((int64_t)core->reg[INSTR_RN(instruction)] < (int64_t)core->reg[INSTR_RM(instruction)]) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SLTU)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (core->reg[INSTR_RN(instruction)] < core->reg[INSTR_RM(instruction)]) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MOV)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MULH)
        {
            uint8_t rd = INSTR_RD(instruction);
            __int128 r = (__int128)(int64_t)core->reg[INSTR_RN(instruction)] *
                         (__int128)(int64_t)core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (uint64_t)(r >> 64));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MULHU)
        {
            uint8_t rd = INSTR_RD(instruction);
            __uint128_t r = (__uint128_t)core->reg[INSTR_RN(instruction)] *
                            (__uint128_t)core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (uint64_t)(r >> 64));
        }
        DISPATCH_NEXT
    
     // ======== WIDE MULTIPLY / BIT EXTRA FUNCT ========
    INSTR_CASE(FUNCT_MULHSW)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t r = (int64_t)(int32_t)(uint32_t)core->reg[INSTR_RN(instruction)] *
                        (int64_t)(int32_t)(uint32_t)core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (uint64_t)(r >> 32));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MULHUW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t r = (uint64_t)(uint32_t)core->reg[INSTR_RN(instruction)] *
                         (uint64_t)(uint32_t)core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, r >> 32);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MADD)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t product = core->reg[INSTR_RN(instruction)] * core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, core->reg[rd] + product);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MSUB)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t product = core->reg[INSTR_RN(instruction)] * core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, core->reg[rd] - product);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_BCLR)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t bit = core->reg[INSTR_RM(instruction)] & 0x3F;
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] & ~(1ULL << bit));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_BINV)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t bit = core->reg[INSTR_RM(instruction)] & 0x3F;
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] ^ (1ULL << bit));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ANDN)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] & ~core->reg[INSTR_RM(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ORN)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] | ~core->reg[INSTR_RM(instruction)]); }
        DISPATCH_NEXT

    // ======== BIT MANIPULATION FUNCT ========
    INSTR_CASE(FUNCT_NOT)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, ~core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_NEG)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (uint64_t)(-(int64_t)core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_CLZ)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, clz64(core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_CTZ)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, ctz64(core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_POPCNT)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, popcnt64(core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_BSWAP)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, BSWAP64(core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ROL)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t val = core->reg[INSTR_RN(instruction)];
            uint8_t shamt = core->reg[INSTR_RM(instruction)] & 0x3F;
            WRITE_REG(core, rd, (val << shamt) | (val >> (64 - shamt)));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ROR)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t val = core->reg[INSTR_RN(instruction)];
            uint8_t shamt = core->reg[INSTR_RM(instruction)] & 0x3F;
            WRITE_REG(core, rd, (val >> shamt) | (val << (64 - shamt)));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SEXTB)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (uint64_t)(int64_t)(int8_t)(uint8_t)core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SEXTH)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (uint64_t)(int64_t)(int16_t)(uint16_t)core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SEXTW)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (uint64_t)SEXT32(core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ZEXTH)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] & 0xFFFF); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ZEXTW)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] & 0xFFFFFFFF); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_BREV)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, brev64(core->reg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_BEXT)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t bit = core->reg[INSTR_RM(instruction)] & 0x3F;
            WRITE_REG(core, rd, (core->reg[INSTR_RN(instruction)] >> bit) & 1);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_BDEP)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t bit = core->reg[INSTR_RM(instruction)] & 0x3F;
            WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)] | (1ULL << bit));
        }
        DISPATCH_NEXT

    // ======== CONDITIONAL MOVE / MINMAX / COMPARE FUNCT ========
    INSTR_CASE(FUNCT_CMOVZ)
        { uint8_t rd = INSTR_RD(instruction); if (core->reg[INSTR_RM(instruction)] == 0) WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_CMOVNZ)
        { uint8_t rd = INSTR_RD(instruction); if (core->reg[INSTR_RM(instruction)] != 0) WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_CMOVLT)
        { uint8_t rd = INSTR_RD(instruction); if ((int64_t)core->reg[INSTR_RM(instruction)] < 0) WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_CMOVGE)
        { uint8_t rd = INSTR_RD(instruction); if ((int64_t)core->reg[INSTR_RM(instruction)] >= 0) WRITE_REG(core, rd, core->reg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MIN)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t a = (int64_t)core->reg[INSTR_RN(instruction)];
            int64_t b = (int64_t)core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (uint64_t)(a < b ? a : b));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MINU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t a = core->reg[INSTR_RN(instruction)];
            uint64_t b = core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, a < b ? a : b);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MAX)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t a = (int64_t)core->reg[INSTR_RN(instruction)];
            int64_t b = (int64_t)core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (uint64_t)(a > b ? a : b));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MAXU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t a = core->reg[INSTR_RN(instruction)];
            uint64_t b = core->reg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, a > b ? a : b);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ABS)
        {
            uint8_t rd = INSTR_RD(instruction);
            int64_t v = (int64_t)core->reg[INSTR_RN(instruction)];
            WRITE_REG(core, rd, (uint64_t)(v < 0 ? -v : v));
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SEQ)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (core->reg[INSTR_RN(instruction)] == core->reg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SNE)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (core->reg[INSTR_RN(instruction)] != core->reg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SGT)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, ((int64_t)core->reg[INSTR_RN(instruction)] > (int64_t)core->reg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SGTU)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (core->reg[INSTR_RN(instruction)] > core->reg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    // ========== FPU FUNCT ==========
    INSTR_CASE(FUNCT_FADD)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = core->freg[INSTR_RN(instruction)] + core->freg[INSTR_RM(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FSUB)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = core->freg[INSTR_RN(instruction)] - core->freg[INSTR_RM(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMUL)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = core->freg[INSTR_RN(instruction)] * core->freg[INSTR_RM(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FDIV)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = core->freg[INSTR_RN(instruction)] / core->freg[INSTR_RM(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FSQRT)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = sqrt(core->freg[INSTR_RN(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FABS)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = fabs(core->freg[INSTR_RN(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FNEG)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = -core->freg[INSTR_RN(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMIN)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = fmin(core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMAX)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = fmax(core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMADD)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = fma(core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)], core->freg[fd]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMSUB)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = fma(core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)], -core->freg[fd]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FNMADD)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = fma(-core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)], core->freg[fd]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FNMSUB)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = fma(-core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)], -core->freg[fd]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCOPYSIGN)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = copysign(core->freg[INSTR_RN(instruction)], core->freg[INSTR_RM(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FROUND)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = round(core->freg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FFLOOR)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = floor(core->freg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    // ======== FPU CONVERSION FUNCT ========
    INSTR_CASE(FUNCT_FCVTW)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (uint64_t)(int64_t)core->freg[INSTR_RN(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCVTD)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = (double)(int64_t)core->reg[INSTR_RN(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCVTUW)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (uint64_t)core->freg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCVTDU)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = (double)core->reg[INSTR_RN(instruction)]; }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCVTW32)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (uint64_t)SEXT32((uint32_t)(int32_t)core->freg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCVTD32)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = (double)(int32_t)(uint32_t)core->reg[INSTR_RN(instruction)]; }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCEIL)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = ceil(core->freg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FTRUNC)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = trunc(core->freg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FCLASS)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, fclass_double(core->freg[INSTR_RN(instruction)])); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FRINT)
        { uint8_t fd = INSTR_RD(instruction); core->freg[fd] = rint(core->freg[INSTR_RN(instruction)]); }
        DISPATCH_NEXT

    // ======== FPU MOVE/TRANSFER FUNCT ========

    INSTR_CASE(FUNCT_FMOV)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = core->freg[INSTR_RN(instruction)];
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMVTX)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint64_t bits;
            memcpy(&bits, &core->freg[INSTR_RN(instruction)], 8);
            WRITE_REG(core, rd, bits);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FMVFX)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint64_t bits = core->reg[INSTR_RN(instruction)];
            memcpy(&core->freg[fd], &bits, 8);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FSGNJN)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = copysign(core->freg[INSTR_RN(instruction)], -core->freg[INSTR_RM(instruction)]);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FSGNJX)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint64_t a, b;
            memcpy(&a, &core->freg[INSTR_RN(instruction)], 8);
            memcpy(&b, &core->freg[INSTR_RM(instruction)], 8);
            uint64_t result = (a & ~(1ULL << 63)) | ((a ^ b) & (1ULL << 63));
            memcpy(&core->freg[fd], &result, 8);
        }
        DISPATCH_NEXT

    // ======== FPU COMPARE FUNCT ========

    INSTR_CASE(FUNCT_FEQ)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (core->freg[INSTR_RN(instruction)] == core->freg[INSTR_RM(instruction)]) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FLT)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (core->freg[INSTR_RN(instruction)] < core->freg[INSTR_RM(instruction)]) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FLE)
        {
            uint8_t rd = INSTR_RD(instruction);
            WRITE_REG(core, rd, (core->freg[INSTR_RN(instruction)] <= core->freg[INSTR_RM(instruction)]) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FGT)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (core->freg[INSTR_RN(instruction)] > core->freg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FGE)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (core->freg[INSTR_RN(instruction)] >= core->freg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FNEQ)
        { uint8_t rd = INSTR_RD(instruction); WRITE_REG(core, rd, (core->freg[INSTR_RN(instruction)] != core->freg[INSTR_RM(instruction)]) ? 1 : 0); }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FORD)
        {
            uint8_t rd = INSTR_RD(instruction);
            double a = core->freg[INSTR_RN(instruction)];
            double b = core->freg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (!isnan(a) && !isnan(b)) ? 1 : 0);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_FUNORD)
        {
            uint8_t rd = INSTR_RD(instruction);
            double a = core->freg[INSTR_RN(instruction)];
            double b = core->freg[INSTR_RM(instruction)];
            WRITE_REG(core, rd, (isnan(a) || isnan(b)) ? 1 : 0);
        }
        DISPATCH_NEXT

    // ========== ATOMIC FUNCT ==========
    INSTR_CASE(FUNCT_LR)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            uint64_t val; load64(env, addr, &val);
            WRITE_REG(core, rd, val);
            core->reservation = addr;
            core->reservation_valid = true;
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SC)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (core->reservation_valid && core->reservation == addr) {
                store64(env, addr, core->reg[INSTR_RM(instruction)]);
                WRITE_REG(core, rd, 0);
            } else {
                WRITE_REG(core, rd, 1);
            }
            core->reservation_valid = false;
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SWAP)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            uint64_t _nv = LE64(core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _raw);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ADD_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            uint64_t _nv = LE64(_raw + core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _raw);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_AND_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            uint64_t _nv = LE64(_raw & core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _raw);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_OR_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            uint64_t _nv = LE64(_raw | core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _raw);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_XOR_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            uint64_t _nv = LE64(_raw ^ core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _raw);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MAX_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            int64_t _old = (int64_t)_raw;
            int64_t _val = (int64_t)core->reg[INSTR_RM(instruction)];
            uint64_t _nv = LE64((uint64_t)(_old > _val ? _old : _val));
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, (uint64_t)_old);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_MIN_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _raw; memcpy(&_raw, &env->mem[addr], 8); _raw = LE64(_raw);
            int64_t _old = (int64_t)_raw;
            int64_t _val = (int64_t)core->reg[INSTR_RM(instruction)];
            uint64_t _nv = LE64((uint64_t)(_old < _val ? _old : _val));
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, (uint64_t)_old);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_DEFAULT
        goto L_VM_ERR_INVALID_OPCODE;

#if !COMPUTED_GOTO_SUPPORTED
        }
    }
#endif

    return;

L_VM_SYSCALL:
    {
        uint64_t syscall_num = core->reg[17];  // a7 register
        if (env->syscall_handler) {
            int result = env->syscall_handler(env, core_num, syscall_num);
            WRITE_REG(core, 10, (uint64_t)result);
        } else {
            switch (syscall_num) {
                case SYS_EXIT:
                    goto L_VM_ERR_HALTED;
                case SYS_YIELD:
                    thread_yield();
                    break;
                case SYS_GETPID:
                    WRITE_REG(core, 10, core_num);
                    break;
                default:
                    WRITE_REG(core, 10, (uint64_t)-1);
                    break;
            }
        }
    }
    END_SYSCALL

L_VM_ERR_HALTED:
    core->status = VM_ERR_HALTED;
    return;
L_VM_ERR_BREAKPOINT:
    core->status = VM_ERR_BREAKPOINT;
    return;
L_VM_ERR_DIV_BY_ZERO:
    core->status = VM_ERR_DIV_BY_ZERO;
    return;
L_VM_ERR_INVALID_OPCODE:
    core->status = VM_ERR_INVALID_OPCODE;
    return;
L_VM_ERR_INVALID_ADDR:
    core->status = VM_ERR_INVALID_ADDR;
    return;
}

// ========== VM LOAD PROGRAM ==========
int vm_load_program(struct VM *vm, const uint8_t *program, size_t size, address load_addr) {
    if (load_addr + size > MEM_SIZE) return -1;
    memcpy(&vm->mem[load_addr], program, size);
    return 0;
}

// ========== VM SET PC ==========
void vm_set_pc(struct VM *vm, uint64_t core_num, address pc) {
    if (core_num < CORE_NUM) vm->core[core_num].pc = pc;
}
