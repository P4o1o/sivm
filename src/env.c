#include "env.h"


void load_prog(struct Environment *env, instr *prog, uint64_t p_size){
    for(uint64_t i = 0; i < p_size; i++){
        env->program[i] = prog[i];
    }
    env->prcount = 0;
    env->link = 0;
    env->snext = 0;
}

void load_value(struct Environment *env, uint64_t *val, address start, address size){
    memcpy(env->vmem + start, val, size * sizeof(uint64_t));
}

void run(struct Environment *env){
    while(1){
        instr code = env->program[env->prcount];
        env->prcount += 1;
        uint64_t imm;
        double immf;
        uint8_t instrtype = INSTR_TYPE(code);
        uint8_t reg1 = INSTR_REG1(code);
        uint8_t reg2 = INSTR_REG2(code);
        uint8_t reg3 = INSTR_REG3(code);
        uint32_t addr = INSTR_ADDR(code);
        switch (instrtype){
            case I_EXT: // EXIT
                goto done;
            break;
            case I_CLC: // CLC
                env->flag = 0;
            break;
            case I_CMP: // CMP
                env->flag = 0;
                imm = env->reg[reg1] - env->reg[reg2];
                env->flag |= (imm == 0);  // 01 => 0; 10 => -; 00 => +
                env->flag |= (((uint32_t) (imm >> ((uint64_t) 63))) << 1);
            break;
            case I_TEST: // TEST
                env->flag = 0;
                imm = env->reg[reg1];
                env->flag |= (imm == 0);  // 01 => 0; 10 => -; 00 => +
                env->flag |= (((uint32_t) (imm >> ((uint64_t) 63))) << 1);
            break;
            case I_CMPF: // CMPF
                env->flag = 0;
                immf = env->freg[reg1] - env->freg[reg2];
                env->flag |= (immf == 0.0);  // 01 => 0; 10 => -; 00 => +
                env->flag |= (((uint32_t) (immf < 0.0)) << 1);
            break;
            case I_TESTF: // TESTF
                env->flag = 0;
                immf = env->freg[reg1];
                env->flag |= (immf == 0.0);  // 01 => 0; 10 => -; 00 => +
                env->flag |= (((uint32_t) (immf < 0.0)) << 1);
            break;
            case I_JMP: // JUMP
                env->prcount=addr;
            break;
            case I_JZ: // JZ
                if (env->flag & 1) env->prcount = addr;
            break;
            case I_JNZ: // JNZ
                if (!(env->flag & 1)) env->prcount = addr;
            break;
            case I_JL: // JL
                if ((env->flag & 10)) env->prcount = addr;
            break;
            case I_JG: // JG
                if ((env->flag & 11) == 0) env->prcount = addr;
            break;
            case I_JLE: // JLE
                if (env->flag & 11) env->prcount = addr;
            break;
            case I_JGE: // JGE
                if ((env->flag & 10) == 0) env->prcount = addr;
            break;
            case I_JREG: // JUMP
                env->prcount = addr + env->reg[reg1];
            break;
            case I_CALL: // CALL
                env->callstack[env->link] = env->prcount;
                env->link++;
                env->prcount=addr;
            break;
            case I_RET: // RET
                env->link--;
                env->prcount = env->callstack[env->link];
            break;
            case I_MOV: // MOVE
                env->reg[reg1] = env->reg[reg2];
            break;
            case I_MOVI: // MOVI
                env->reg[reg1] = ((uint64_t) addr);
            break;
            case I_MOVZ: // MOVZ
                if (env->flag & 1) env->reg[reg1] = env->reg[reg2];
            break;
            case I_MOVNZ: // MOVNZ
                if (!(env->flag & 1)) env->reg[reg1] = env->reg[reg2];
            break;
            case I_MOVL: // MOVL
                if ((env->flag & 10)) env->reg[reg1] = env->reg[reg2];
            break;
            case I_MOVG: // MOVG
                if ((env->flag & 11) == 0) env->reg[reg1] = env->reg[reg2];
            break;
            case I_MOVLE: // MOVLE
                if (env->flag & 11) env->reg[reg1] = env->reg[reg2];
            break;
            case I_MOVGE: // MOVGE
                if ((env->flag & 10) == 0) env->reg[reg1] = env->reg[reg2];
            break;
            case I_LOAD: // LOAD
                env->reg[reg1] = env->vmem[addr].i64;
            break;
            case I_STORE: // STORE
                env->vmem[addr].i64 = env->reg[reg1];                
            break;
            case I_PUSH: // PUSH
                env->stack[env->snext].i64 = env->reg[reg1];
                env->snext += 1;
            break;
            case I_POP: // POP
                env->snext -= 1;
                env->reg[reg1] = env->stack[env->snext].i64;
            break;
            case I_MOVF: // MOVF
                env->freg[reg1] = env->freg[reg2];
            break;
            case I_MOVFI: // MOVFI
                env->freg[reg1] = addr;// WEWE
            break;
            case I_MOVFZ: // MOVFZ
                if (env->flag & 1) env->freg[reg1] = env->freg[reg2];
            break;
            case I_MOVFNZ: // MOVFNZ
                if (!(env->flag & 1)) env->freg[reg1] = env->freg[reg2];
            break;
            case I_MOVFL: // MOVFL
                if ((env->flag & 10)) env->freg[reg1] = env->freg[reg2];
            break;
            case I_MOVFG: // MOVFG
                if ((env->flag & 11) == 0) env->freg[reg1] = env->freg[reg2];
            break;
            case I_MOVFLE: // MOVFLE
                if (env->flag & 11) env->freg[reg1] = env->freg[reg2];
            break;
            case I_MOVFGE: // MOVFGE
                if ((env->flag & 10) == 0) env->freg[reg1] = env->freg[reg2];
            break;
            case I_LOADF: // LOAD
                env->freg[reg1] = env->vmem[addr].f64;
            break;
            case I_STOREF: // STORE
                env->vmem[addr].f64 = env->freg[reg1];                
            break;
            case I_PUSHF: // PUSH
                env->stack[env->snext].f64 = env->freg[reg1];
                env->snext += 1;
            break;
            case I_POPF: // POP
                env->snext -= 1;
                env->freg[reg1] = env->stack[env->snext].f64;
            break;

            case I_NOP:  // NOP
                continue;
            break;

            case I_ADD:  // ADD
                env->reg[reg1] = env->reg[reg2] + env->reg[reg3];
            break;
            case I_SUB:  // SUB
                env->reg[reg1] = env->reg[reg2] - env->reg[reg3];
            break;
            case I_MUL:  // MUL
                env->reg[reg1] = env->reg[reg2] * env->reg[reg3];
            break;
            case I_DIV: // DIV
                env->reg[reg1] = env->reg[reg2] / env->reg[reg3];
            break;
            case I_ADDF:  // ADDF
                env->freg[reg1] = env->freg[reg2] + env->freg[reg3];
            break;
            case I_SUBF:  // SUBF
                env->freg[reg1] = env->freg[reg2] - env->freg[reg3];
            break;
            case I_MULF:  // MUL
                env->freg[reg1] = env->freg[reg2] * env->freg[reg3];
            break;
            case I_DIVF: // DIVF
                env->freg[reg1] = env->freg[reg2] / env->freg[reg3];
            break;
            case I_MOD: // MOD
                env->reg[reg1] = env->reg[reg2] % env->reg[reg3];
            break;
            case I_INC: // INC
                env->reg[reg1] += 1;
            break;
            case I_DEC: // DEC
                env->reg[reg1] -= 1;
            break;

            case I_AND: // AND
                env->reg[reg1] = env->reg[reg2] & env->reg[reg3];
            break;
            case I_OR: // OR
                env->reg[reg1] = env->reg[reg2] | env->reg[reg3];
            break;
            case I_XOR: // XOR
                env->reg[reg1] = env->reg[reg2] ^ env->reg[reg3];
            break;
            case I_NOT: // NOT
                env->reg[reg1] = ~ env->reg[reg1];
            break;
            case I_SHL: // SHL
                env->reg[reg1] = env->reg[reg2] << env->reg[reg3];
            break;
            case I_SHR: // SHR
                env->reg[reg1] = env->reg[reg2] >> env->reg[reg3];
            break;

            case I_CAST: // CAST
                env->reg[reg1] = (uint64_t) env->freg[reg2];
            break;
            case I_CASTF: // CASTF
                env->freg[reg1] = (double) env->reg[reg2];
            break;

            case I_COS: // COS
                env->freg[reg1] = cos(env->freg[reg2]);
            break;
            case I_SIN: // SEN
                env->freg[reg1] = sin(env->freg[reg2]);
            break;
            case I_TAN: // TAN
                env->freg[reg1] = tan(env->freg[reg2]);
            break;
            case I_ACOS: // ACOS
                env->freg[reg1] = acos(env->freg[reg2]);
            break;
            case I_ASIN: // ASEN
                env->freg[reg1] = asin(env->freg[reg2]);
            break;
            case I_ATAN: // ATAN
                env->freg[reg1] = atan(env->freg[reg2]);
            break;
            case I_COSH: // COSH
                env->freg[reg1] = cosh(env->freg[reg2]);
            break;
            case I_SINH: // SENH
                env->freg[reg1] = sinh(env->freg[reg2]);
            break;
            case I_TANH: // TANH
                env->freg[reg1] = tanh(env->freg[reg2]);
            break;

            case I_SQRT: // SQRT
                env->freg[reg1] = sqrt(env->freg[reg2]);
            break;
            case I_POW: // POW
                env->freg[reg1] = pow(env->freg[reg2], env->freg[reg3]);
            break;
            case I_EXP: // EXP
                env->freg[reg1] = exp(env->freg[reg2]);
            break;
            case I_LOG: // LOG
                env->freg[reg1] = log(env->freg[reg2]);
            break;
            case I_LN: // LN
                env->freg[reg1] = log2(env->freg[reg2]);
            break;
            case I_LOG10: // LOG10
                env->freg[reg1] = log10(env->freg[reg2]);
            default:
            break;
        }
    }
done:
    return;
}
