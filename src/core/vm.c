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
        [0x04 ... 0x07]= &&L_DEFAULT,
        [OP_ADDI]      = &&L_OP_ADDI,
        [OP_SUBI]      = &&L_OP_SUBI,
        [OP_ANDI]      = &&L_OP_ANDI,
        [OP_ORI]       = &&L_OP_ORI,
        [OP_XORI]      = &&L_OP_XORI,
        [OP_SLTI]      = &&L_OP_SLTI,
        [OP_SLTIU]     = &&L_OP_SLTIU,
        [0x0F]         = &&L_DEFAULT,
        [OP_SLLI]      = &&L_OP_SLLI,
        [OP_SRLI]      = &&L_OP_SRLI,
        [OP_SRAI]      = &&L_OP_SRAI,
        [0x13 ... 0x17]= &&L_DEFAULT,
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
        [0x24 ... 0x27]= &&L_DEFAULT,
        [OP_BEQ]       = &&L_OP_BEQ,
        [OP_BNE]       = &&L_OP_BNE,
        [OP_BLT]       = &&L_OP_BLT,
        [OP_BGE]       = &&L_OP_BGE,
        [OP_BLTU]      = &&L_OP_BLTU,
        [OP_BGEU]      = &&L_OP_BGEU,
        [0x2E ... 0x2F]= &&L_DEFAULT,
        [OP_JAL]       = &&L_OP_JAL,
        [OP_JALR]      = &&L_OP_JALR,
        [0x32 ... 0x38]= &&L_DEFAULT,
        [OP_FLD]       = &&L_OP_FLD,
        [OP_FSD]       = &&L_OP_FSD,
        [0x3B ... 0x3D]= &&L_DEFAULT,
        [OP_FENCE]     = &&L_OP_FENCE,
        [OP_ECALL]     = &&L_OP_ECALL,
        // ===== UNUSED [0x40-0x7F] =====
        [0x40 ... 0x7F]= &&L_DEFAULT,
        // ===== ALU FUNCT [0x80-0x92] =====
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
        // ===== FPU FUNCT [0xA0-0xD2] =====
        [FUNCT_FADD]   = &&L_FUNCT_FADD,
        [FUNCT_FSUB]   = &&L_FUNCT_FSUB,
        [FUNCT_FMUL]   = &&L_FUNCT_FMUL,
        [FUNCT_FDIV]   = &&L_FUNCT_FDIV,
        [FUNCT_FSQRT]  = &&L_FUNCT_FSQRT,
        [FUNCT_FABS]   = &&L_FUNCT_FABS,
        [FUNCT_FNEG]   = &&L_FUNCT_FNEG,
        [FUNCT_FMIN]   = &&L_FUNCT_FMIN,
        [FUNCT_FMAX]   = &&L_FUNCT_FMAX,
        [0xA9 ... 0xAF]= &&L_DEFAULT,
        [FUNCT_FCVTW]  = &&L_FUNCT_FCVTW,
        [FUNCT_FCVTD]  = &&L_FUNCT_FCVTD,
        [0xB2 ... 0xBF]= &&L_DEFAULT,
        [FUNCT_FMOV]   = &&L_FUNCT_FMOV,
        [0xC1 ... 0xCF]= &&L_DEFAULT,
        [FUNCT_FEQ]    = &&L_FUNCT_FEQ,
        [FUNCT_FLT]    = &&L_FUNCT_FLT,
        [FUNCT_FLE]    = &&L_FUNCT_FLE,
        [0xD3 ... 0xEF]= &&L_DEFAULT,
        // ===== ATOMIC FUNCT [0xF0-0xF8] =====
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

    // ========== BRANCH INSTRUCTIONS ==========
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

    INSTR_CASE(FUNCT_FMOV)
        {
            uint8_t fd = INSTR_RD(instruction);
            core->freg[fd] = core->freg[INSTR_RN(instruction)];
        }
        DISPATCH_NEXT

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

    // ========== ATOMIC FUNCT ==========
    INSTR_CASE(FUNCT_LR)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _v; memcpy(&_v, &env->mem[addr], 8); _v = LE64(_v);
            WRITE_REG(core, rd, _v);
            core->reservation = addr;
            core->reservation_valid = true;
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SC)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            if (core->reservation_valid && core->reservation == addr) {
                uint64_t _sv = LE64(core->reg[INSTR_RM(instruction)]);
                memcpy(&env->mem[addr], &_sv, 8);
                WRITE_REG(core, rd, 0);
            } else {
                WRITE_REG(core, rd, 1);
            }
            core->reservation_valid = false;
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_SWAP)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _old; memcpy(&_old, &env->mem[addr], 8); _old = LE64(_old);
            uint64_t _nv = LE64(core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _old);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_ADD_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _old; memcpy(&_old, &env->mem[addr], 8); _old = LE64(_old);
            uint64_t _nv = LE64(_old + core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _old);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_AND_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _old; memcpy(&_old, &env->mem[addr], 8); _old = LE64(_old);
            uint64_t _nv = LE64(_old & core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _old);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_OR_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _old; memcpy(&_old, &env->mem[addr], 8); _old = LE64(_old);
            uint64_t _nv = LE64(_old | core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _old);
            MEM_UNLOCK(env);
        }
        DISPATCH_NEXT

    INSTR_CASE(FUNCT_XOR_A)
        {
            uint8_t rd = INSTR_RD(instruction);
            address addr = core->reg[INSTR_RN(instruction)];
            if (UNLIKELY(!CHECK_ADDR(addr, 8))) goto L_VM_ERR_INVALID_ADDR;
            MEM_LOCK(env);
            uint64_t _old; memcpy(&_old, &env->mem[addr], 8); _old = LE64(_old);
            uint64_t _nv = LE64(_old ^ core->reg[INSTR_RM(instruction)]);
            memcpy(&env->mem[addr], &_nv, 8);
            WRITE_REG(core, rd, _old);
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
