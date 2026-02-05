#include "vm.h"

// Forward declarations for helper functions
static void execute_alu(struct VM *env, struct Core *core, instr instruction);
static void execute_fpu(struct VM *env, struct Core *core, instr instruction);
static void execute_atomic(struct VM *env, struct Core *core, instr instruction);
static void handle_syscall(struct VM *env, uint64_t core_num);

static force_inline bool check_addr(address addr, size_t size) {
    return (addr + size) <= MEM_SIZE;
}

static force_inline void store8(VM *vm, address addr, uint8_t val) {
    if (UNLIKELY(!check_addr(addr, 1))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    vm->mem[addr] = val;
}

static force_inline void store16(VM *vm, address addr, uint16_t val) {
    if (UNLIKELY(!check_addr(addr, 2))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    #ifdef ENDIAN_LITTLE
        memcpy(&vm->mem[addr], &val, 2);
    #else
        uint16_t le = BSWAP16(val);
        memcpy(&vm->mem[addr], &le, 2);
    #endif
}

static force_inline void store32(VM *vm, address addr, uint32_t val) {
    if (UNLIKELY(!check_addr(addr, 4))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    #ifdef ENDIAN_LITTLE
        memcpy(&vm->mem[addr], &val, 4);
    #else
        uint32_t le = BSWAP32(val);
        memcpy(&vm->mem[addr], &le, 4);
    #endif
}

static force_inline void store64(VM *vm, address addr, uint64_t val) {
    if (UNLIKELY(!check_addr(addr, 8))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    #ifdef ENDIAN_LITTLE
        memcpy(&vm->mem[addr], &val, 8);
    #else
        uint64_t le = BSWAP64(val);
        memcpy(&vm->mem[addr], &le, 8);
    #endif
}

static force_inline uint8_t load8(VM *vm, address addr) {
    if (UNLIKELY(!check_addr(addr, 1))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    return vm->mem[addr];
}

static force_inline uint16_t load16(VM *vm, address addr) {
    if (UNLIKELY(!check_addr(addr, 2))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    uint16_t val;
    #ifdef ENDIAN_LITTLE
        memcpy(&val, &vm->mem[addr], 2);
    #else
        uint16_t le;
        memcpy(&le, &vm->mem[addr], 2);
        val = BSWAP16(le);
    #endif
    return val;
}

static force_inline uint32_t load32(VM *vm, address addr) {
    if (UNLIKELY(!check_addr(addr, 4))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    uint32_t val;
    #ifdef ENDIAN_LITTLE
        memcpy(&val, &vm->mem[addr], 4);
    #else
        uint32_t le;
        memcpy(&le, &vm->mem[addr], 4);
        val = BSWAP32(le);
    #endif
    return val;
}

static force_inline uint64_t load64(VM *vm, address addr) {
    if (UNLIKELY(!check_addr(addr, 8))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    uint64_t val;
    #ifdef ENDIAN_LITTLE
        memcpy(&val, &vm->mem[addr], 8);
    #else
        uint64_t le;
        memcpy(&le, &vm->mem[addr], 8);
        val = BSWAP64(le);
    #endif
    return val;
}

static force_inline void store_double(VM *vm, address addr, double val) {
    if (UNLIKELY(!check_addr(addr, 8) || (addr & 7))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    uint64_t bits;
    memcpy(&bits, &val, 8);
    store64(vm, addr, bits);
}

static force_inline double load_double(VM *vm, address addr) {
    if (UNLIKELY(!check_addr(addr, 8) || (addr & 7))) longjmp(vm->core[0].jmp, VM_ERR_INVALID_ADDR);
    uint64_t bits = load64(vm, addr);
    double val;
    memcpy(&val, &bits, 8);
    return val;
}

static force_inline instr fetch(struct VM *env, struct Core *core) {
    address pc = core->pc;
    if (UNLIKELY(!check_addr(pc, 4))) longjmp(core->jmp, VM_ERR_INVALID_ADDR);
    instr instruction = 0;
    #ifdef ENDIAN_LITTLE
        memcpy(&instruction, &env->mem[pc], 4);
    #else
        instr le_instr = 0;
        memcpy(&le_instr, &env->mem[pc], 4);
        instruction = BSWAP32(le_instr);
    #endif
    core->pc += 4;
    core->instret++;
    return instruction;
}

// Ensure r0 is always zero
#define WRITE_REG(core, rd, val) do { if ((rd) != REG_ZERO) (core)->reg[rd] = (val); } while(0)

#if COMPUTED_GOTO_SUPPORTED
    #define DISPATCH_START goto *dispatch_table[INSTR_OPCODE(instruction)];
    #define DISPATCH_NEXT instruction = fetch(env, core); goto *dispatch_table[INSTR_OPCODE(instruction)];
    #define INSTR_CASE(op) L_##op:
    #define INSTR_DEFAULT L_DEFAULT:
#else
    #define DISPATCH_START switch(INSTR_OPCODE(instruction)) {
    #define DISPATCH_NEXT instruction = fetch(env, core); break;
    #define INSTR_CASE(op) case op:
    #define INSTR_DEFAULT default:
#endif

void vm_init(struct VM *vm) {
    memset(vm, 0, sizeof(struct VM));
    for (int i = 0; i < CORE_NUM; i++) {
        vm->core[i].reg[REG_SP] = MEM_SIZE - (i * STACK_SIZE);
        vm->core[i].pc = 0;
    }
    vm->running = true;
}

void run(struct VM *env, uint64_t core_num) {
    struct Core *core = &env->core[core_num];
    instr instruction;

#if COMPUTED_GOTO_SUPPORTED
    static void* dispatch_table[64] = {
        [OP_NOP]     = &&L_OP_NOP,
        [OP_HALT]    = &&L_OP_HALT,
        [OP_SYSCALL] = &&L_OP_SYSCALL,
        [OP_BREAK]   = &&L_OP_BREAK,
        [OP_ALU]     = &&L_OP_ALU,
        [0x05 ... 0x07] = &&L_DEFAULT,
        [OP_ADDI]    = &&L_OP_ADDI,
        [OP_SUBI]    = &&L_OP_SUBI,
        [OP_ANDI]    = &&L_OP_ANDI,
        [OP_ORI]     = &&L_OP_ORI,
        [OP_XORI]    = &&L_OP_XORI,
        [OP_SLTI]    = &&L_OP_SLTI,
        [OP_SLTIU]   = &&L_OP_SLTIU,
        [0x0F]       = &&L_DEFAULT,
        [OP_SLLI]    = &&L_OP_SLLI,
        [OP_SRLI]    = &&L_OP_SRLI,
        [OP_SRAI]    = &&L_OP_SRAI,
        [0x13 ... 0x17] = &&L_DEFAULT,
        [OP_LB]      = &&L_OP_LB,
        [OP_LBU]     = &&L_OP_LBU,
        [OP_LH]      = &&L_OP_LH,
        [OP_LHU]     = &&L_OP_LHU,
        [OP_LW]      = &&L_OP_LW,
        [OP_LWU]     = &&L_OP_LWU,
        [OP_LD]      = &&L_OP_LD,
        [OP_LUI]     = &&L_OP_LUI,
        [OP_SB]      = &&L_OP_SB,
        [OP_SH]      = &&L_OP_SH,
        [OP_SW]      = &&L_OP_SW,
        [OP_SD]      = &&L_OP_SD,
        [0x24 ... 0x27] = &&L_DEFAULT,
        [OP_BEQ]     = &&L_OP_BEQ,
        [OP_BNE]     = &&L_OP_BNE,
        [OP_BLT]     = &&L_OP_BLT,
        [OP_BGE]     = &&L_OP_BGE,
        [OP_BLTU]    = &&L_OP_BLTU,
        [OP_BGEU]    = &&L_OP_BGEU,
        [0x2E ... 0x2F] = &&L_DEFAULT,
        [OP_JAL]     = &&L_OP_JAL,
        [OP_JALR]    = &&L_OP_JALR,
        [0x32 ... 0x37] = &&L_DEFAULT,
        [OP_FPU]     = &&L_OP_FPU,
        [OP_FLD]     = &&L_OP_FLD,
        [OP_FSD]     = &&L_OP_FSD,
        [0x3B]       = &&L_DEFAULT,
        [OP_ATOMIC]  = &&L_OP_ATOMIC,
        [0x3D]       = &&L_DEFAULT,
        [OP_FENCE]   = &&L_OP_FENCE,
        [OP_ECALL]   = &&L_OP_ECALL,
    };
#endif

L_START_EXECUTION:
    if ((core->error = setjmp(core->jmp)) == 0) {
        instruction = fetch(env, core);
        
#if COMPUTED_GOTO_SUPPORTED
        DISPATCH_START
#else
        while(1) {
            DISPATCH_START
#endif

    // ========== SYSTEM INSTRUCTIONS ==========
    INSTR_CASE(OP_NOP)
        core->cycles++;
        DISPATCH_NEXT

    INSTR_CASE(OP_HALT)
        core->flags |= FLAG_HALTED;
        longjmp(core->jmp, VM_ERR_HALTED);

    INSTR_CASE(OP_SYSCALL)
        longjmp(core->jmp, VM_SYSCALL);

    INSTR_CASE(OP_BREAK)
        longjmp(core->jmp, VM_ERR_BREAKPOINT);

    // ========== ALU REGISTER-REGISTER ==========
    INSTR_CASE(OP_ALU)
        execute_alu(env, core, instruction);
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
            int8_t val = (int8_t)load8(env, addr);
            WRITE_REG(core, rd, (uint64_t)(int64_t)val);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LBU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            WRITE_REG(core, rd, load8(env, addr));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LH)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            int16_t val = (int16_t)load16(env, addr);
            WRITE_REG(core, rd, (uint64_t)(int64_t)val);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LHU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            WRITE_REG(core, rd, load16(env, addr));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LW)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            int32_t val = (int32_t)load32(env, addr);
            WRITE_REG(core, rd, (uint64_t)(int64_t)val);
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LWU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            WRITE_REG(core, rd, load32(env, addr));
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_LD)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            WRITE_REG(core, rd, load64(env, addr));
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
            if (core->reg[rd] == core->reg[rn]) {
                core->pc += offset - 4;  // -4 because we already incremented
            }
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BNE)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] != core->reg[rn]) {
                core->pc += offset - 4;
            }
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BLT)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if ((int64_t)core->reg[rd] < (int64_t)core->reg[rn]) {
                core->pc += offset - 4;
            }
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BGE)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if ((int64_t)core->reg[rd] >= (int64_t)core->reg[rn]) {
                core->pc += offset - 4;
            }
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BLTU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] < core->reg[rn]) {
                core->pc += offset - 4;
            }
        }
        DISPATCH_NEXT

    INSTR_CASE(OP_BGEU)
        {
            uint8_t rd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction)) << 2;
            if (core->reg[rd] >= core->reg[rn]) {
                core->pc += offset - 4;
            }
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

    // ========== FLOATING POINT ==========
    INSTR_CASE(OP_FPU)
        execute_fpu(env, core, instruction);
        DISPATCH_NEXT

    INSTR_CASE(OP_FLD)
        {
            uint8_t fd = INSTR_RD(instruction);
            uint8_t rn = INSTR_RN(instruction);
            int64_t offset = SIGN_EXT16(INSTR_IMM16(instruction));
            address addr = core->reg[rn] + offset;
            core->freg[fd] = load_double(env, addr);
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

    // ========== ATOMIC ==========
    INSTR_CASE(OP_ATOMIC)
        execute_atomic(env, core, instruction);
        DISPATCH_NEXT

    // ========== MISC ==========
    INSTR_CASE(OP_FENCE)
        atomic_fence_seq_cst();
        DISPATCH_NEXT

    INSTR_CASE(OP_ECALL)
        longjmp(core->jmp, VM_SYSCALL);

    INSTR_DEFAULT
        longjmp(core->jmp, VM_ERR_INVALID_OPCODE);

#if !COMPUTED_GOTO_SUPPORTED
            }
        }
#endif
    } else if (core->error == VM_SYSCALL) {
        handle_syscall(env, core_num);
        goto L_START_EXECUTION;
    } else {
        // Error handling - core->error contains the error code
        env->running = false;
    }
}

// ========== ALU OPERATIONS ==========
static force_inline void execute_alu(struct VM *env, struct Core *core, instr instruction) {
    uint8_t rd = INSTR_RD(instruction);
    uint8_t rn = INSTR_RN(instruction);
    uint8_t rm = INSTR_RM(instruction);
    uint16_t funct = INSTR_FUNCT(instruction);
    
    uint64_t a = core->reg[rn];
    uint64_t b = core->reg[rm];
    uint64_t result = 0;
    
    switch (funct) {
        case FUNCT_ADD:
            result = a + b;
            break;
        case FUNCT_SUB:
            result = a - b;
            break;
        case FUNCT_MUL:
            result = a * b;
            break;
        case FUNCT_DIV:
            if (b == 0) longjmp(core->jmp, VM_ERR_DIV_BY_ZERO);
            result = (uint64_t)((int64_t)a / (int64_t)b);
            break;
        case FUNCT_DIVU:
            if (b == 0) longjmp(core->jmp, VM_ERR_DIV_BY_ZERO);
            result = a / b;
            break;
        case FUNCT_REM:
            if (b == 0) longjmp(core->jmp, VM_ERR_DIV_BY_ZERO);
            result = (uint64_t)((int64_t)a % (int64_t)b);
            break;
        case FUNCT_REMU:
            if (b == 0) longjmp(core->jmp, VM_ERR_DIV_BY_ZERO);
            result = a % b;
            break;
        case FUNCT_AND:
            result = a & b;
            break;
        case FUNCT_OR:
            result = a | b;
            break;
        case FUNCT_XOR:
            result = a ^ b;
            break;
        case FUNCT_NOR:
            result = ~(a | b);
            break;
        case FUNCT_SLL:
            result = a << (b & 0x3F);
            break;
        case FUNCT_SRL:
            result = a >> (b & 0x3F);
            break;
        case FUNCT_SRA:
            result = (uint64_t)((int64_t)a >> (b & 0x3F));
            break;
        case FUNCT_SLT:
            result = ((int64_t)a < (int64_t)b) ? 1 : 0;
            break;
        case FUNCT_SLTU:
            result = (a < b) ? 1 : 0;
            break;
        case FUNCT_MOV:
            result = a;
            break;
        case FUNCT_MULH: {
            // Signed multiply high
            __int128 res = (__int128)(int64_t)a * (__int128)(int64_t)b;
            result = (uint64_t)(res >> 64);
            break;
        }
        case FUNCT_MULHU: {
            // Unsigned multiply high
            __uint128_t res = (__uint128_t)a * (__uint128_t)b;
            result = (uint64_t)(res >> 64);
            break;
        }
        default:
            longjmp(core->jmp, VM_ERR_INVALID_OPCODE);
    }
    
    WRITE_REG(core, rd, result);
    (void)env;
}

// ========== FPU OPERATIONS ==========
static force_inline void execute_fpu(struct VM *env, struct Core *core, instr instruction) {
    uint8_t fd = INSTR_RD(instruction);
    uint8_t fn = INSTR_RN(instruction);
    uint8_t fm = INSTR_RM(instruction);
    uint16_t funct = INSTR_FUNCT(instruction);
    
    double a = core->freg[fn];
    double b = core->freg[fm];
    double result = 0.0;
    
    switch (funct) {
        case FUNCT_FADD:
            result = a + b;
            core->freg[fd] = result;
            break;
        case FUNCT_FSUB:
            result = a - b;
            core->freg[fd] = result;
            break;
        case FUNCT_FMUL:
            result = a * b;
            core->freg[fd] = result;
            break;
        case FUNCT_FDIV:
            result = a / b;
            core->freg[fd] = result;
            break;
        case FUNCT_FSQRT:
            result = sqrt(a);
            core->freg[fd] = result;
            break;
        case FUNCT_FABS:
            result = fabs(a);
            core->freg[fd] = result;
            break;
        case FUNCT_FNEG:
            result = -a;
            core->freg[fd] = result;
            break;
        case FUNCT_FMIN:
            result = fmin(a, b);
            core->freg[fd] = result;
            break;
        case FUNCT_FMAX:
            result = fmax(a, b);
            core->freg[fd] = result;
            break;
        case FUNCT_FCVTW: {
            // Float to int (result in integer register)
            int64_t ival = (int64_t)a;
            WRITE_REG(core, fd, (uint64_t)ival);
            break;
        }
        case FUNCT_FCVTD: {
            // Int to float (source from integer register)
            int64_t ival = (int64_t)core->reg[fn];
            core->freg[fd] = (double)ival;
            break;
        }
        case FUNCT_FMOV:
            core->freg[fd] = a;
            break;
        case FUNCT_FEQ:
            WRITE_REG(core, fd, (a == b) ? 1 : 0);
            break;
        case FUNCT_FLT:
            WRITE_REG(core, fd, (a < b) ? 1 : 0);
            break;
        case FUNCT_FLE:
            WRITE_REG(core, fd, (a <= b) ? 1 : 0);
            break;
        default:
            longjmp(core->jmp, VM_ERR_INVALID_OPCODE);
    }
    (void)env;
}

// ========== ATOMIC OPERATIONS ==========
static force_inline void execute_atomic(struct VM *env, struct Core *core, instr instruction) {
    uint8_t rd = INSTR_RD(instruction);
    uint8_t rn = INSTR_RN(instruction);
    uint8_t rm = INSTR_RM(instruction);
    uint16_t funct = INSTR_FUNCT(instruction);
    
    address addr = core->reg[rn];
    
    // Acquire lock for atomic operation
    while (atomic_flag_test_and_set(&env->mem_lock)) {
        cpu_pause();
    }
    
    switch (funct) {
        case FUNCT_LR: {
            // Load Reserved
            uint64_t val = load64(env, addr);
            WRITE_REG(core, rd, val);
            core->reservation = addr;
            core->reservation_valid = true;
            break;
        }
        case FUNCT_SC: {
            // Store Conditional
            if (core->reservation_valid && core->reservation == addr) {
                store64(env, addr, core->reg[rm]);
                WRITE_REG(core, rd, 0);  // Success
            } else {
                WRITE_REG(core, rd, 1);  // Failure
            }
            core->reservation_valid = false;
            break;
        }
        case FUNCT_SWAP: {
            uint64_t old = load64(env, addr);
            store64(env, addr, core->reg[rm]);
            WRITE_REG(core, rd, old);
            break;
        }
        case FUNCT_ADD_A: {
            uint64_t old = load64(env, addr);
            store64(env, addr, old + core->reg[rm]);
            WRITE_REG(core, rd, old);
            break;
        }
        case FUNCT_AND_A: {
            uint64_t old = load64(env, addr);
            store64(env, addr, old & core->reg[rm]);
            WRITE_REG(core, rd, old);
            break;
        }
        case FUNCT_OR_A: {
            uint64_t old = load64(env, addr);
            store64(env, addr, old | core->reg[rm]);
            WRITE_REG(core, rd, old);
            break;
        }
        case FUNCT_XOR_A: {
            uint64_t old = load64(env, addr);
            store64(env, addr, old ^ core->reg[rm]);
            WRITE_REG(core, rd, old);
            break;
        }
        case FUNCT_MAX_A: {
            int64_t old = (int64_t)load64(env, addr);
            int64_t val = (int64_t)core->reg[rm];
            store64(env, addr, (uint64_t)(old > val ? old : val));
            WRITE_REG(core, rd, (uint64_t)old);
            break;
        }
        case FUNCT_MIN_A: {
            int64_t old = (int64_t)load64(env, addr);
            int64_t val = (int64_t)core->reg[rm];
            store64(env, addr, (uint64_t)(old < val ? old : val));
            WRITE_REG(core, rd, (uint64_t)old);
            break;
        }
        default:
            atomic_flag_clear(&env->mem_lock);
            longjmp(core->jmp, VM_ERR_INVALID_OPCODE);
    }
    
    atomic_flag_clear(&env->mem_lock);
}

// ========== SYSCALL HANDLER ==========
static force_inline void handle_syscall(struct VM *env, uint64_t core_num) {
    struct Core *core = &env->core[core_num];
    uint64_t syscall_num = core->reg[17];  // a7 register (like RISC-V)
    
    if (env->syscall_handler) {
        int result = env->syscall_handler(env, core_num, syscall_num);
        WRITE_REG(core, 10, (uint64_t)result);  // a0 register for return value
    } else {
        // Default syscall handling
        switch (syscall_num) {
            case SYS_EXIT:
                core->flags |= FLAG_HALTED;
                env->running = false;
                break;
            case SYS_YIELD:
                thread_yield();
                break;
            case SYS_GETPID:
                WRITE_REG(core, 10, core_num);
                break;
            default:
                // Unknown syscall - set error
                WRITE_REG(core, 10, (uint64_t)-1);
                break;
        }
    }
}

// ========== VM LOAD PROGRAM ==========
int vm_load_program(struct VM *vm, const uint8_t *program, size_t size, address load_addr) {
    if (load_addr + size > MEM_SIZE) {
        return -1;
    }
    memcpy(&vm->mem[load_addr], program, size);
    return 0;
}

// ========== VM SET PC ==========
void vm_set_pc(struct VM *vm, uint64_t core_num, address pc) {
    if (core_num < CORE_NUM) {
        vm->core[core_num].pc = pc;
    }
}