#include "env.h"


void load_prog(struct Environment *env, instr *prog, uint64_t p_size){
    memset(env->core, 0, sizeof(struct Core) * CORE_NUM);
    for(uint64_t i = 0; i < p_size; i++){
        env->program[i] = prog[i];
    }
}

void load_value(struct Environment *env, uint64_t *val, address start, address size){
    memcpy(env->vmem + start, val, size * sizeof(uint64_t));
}

void run(struct Environment *env, const uint64_t core_num){
    env->core[core_num].status = 1;
    while(1){
        instr code = env->program[env->core[core_num].prcount];
        env->core[core_num].prcount += 1;

        void* dptr;
        cudaError_t err;
        uint64_t imm;
        double immf;

        const enum InstrCode instrtype = INSTR_TYPE(code);
        const uint8_t reg1 = INSTR_REG1(code);
        const uint8_t reg2 = INSTR_REG2(code);
        const uint8_t reg3 = INSTR_REG3(code);
        const uint32_t addr = INSTR_ADDR(code);

        switch (instrtype){
            case I_STOP: // EXIT
                return;
            case I_EXIT: // EXIT
                env->core[core_num].status = 0;
                return;
            case I_CMALLOC:
                dptr = NULL;
                err = cudaMalloc(&dptr, env->core[core_num].reg[reg1]);
                env->core[core_num].reg[reg1] = (uint64_t) dptr;
            break;
            case I_CFREE:
                err = cudaFree((void*)env->core[core_num].reg[reg1]);
            break;
            case I_CMOVH:
                err = cudaMemcpy(
                    (void*) env->core[core_num].reg[reg2],
                    (void*) &(env->vmem[env->core[core_num].reg[reg1]]),
                    env->core[core_num].reg[reg3], 
                    cudaMemcpyDeviceToHost
                );
            break;
            case I_CMOVD:
                err = cudaMemcpy(
                    (void*) env->core[core_num].reg[reg1],
                    (void*) &(env->vmem[env->core[core_num].reg[reg2]]),
                    env->core[core_num].reg[reg3], 
                    cudaMemcpyHostToDevice
                );
            break;
            case I_SPAWN:
                #pragma omp atomic capture
                {
                    imm = env->core[reg1].status;
                    env->core[reg1].status = 1;
                }
                if(imm){
                    continue;
                }
                env->core[reg1].prcount = addr;
                #pragma omp task
                    run(env, reg1);
            break;
            case I_CLC: // CLC
                env->core[core_num].flag = 0;
            break;
            case I_CMP: // CMP
                env->core[core_num].flag = 0;
                imm = env->core[core_num].reg[reg1] - env->core[core_num].reg[reg2];
                env->core[core_num].flag |= (imm == 0);  // 01 => 0; 10 => -; 00 => +
                env->core[core_num].flag |= (((uint32_t) (imm >> ((uint64_t) 63))) << 1);
            break;
            case I_TEST: // TEST
                env->core[core_num].flag = 0;
                imm = env->core[core_num].reg[reg1];
                env->core[core_num].flag |= (imm == 0);  // 01 => 0; 10 => -; 00 => +
                env->core[core_num].flag |= (((uint32_t) (imm >> ((uint64_t) 63))) << 1);
            break;
            case I_CMPF: // CMPF
                env->core[core_num].flag = 0;
                immf = env->core[core_num].freg[reg1] - env->core[core_num].freg[reg2];
                env->core[core_num].flag |= (immf == 0.0);  // 01 => 0; 10 => -; 00 => +
                env->core[core_num].flag |= (((uint32_t) (immf < 0.0)) << 1);
            break;
            case I_TESTF: // TESTF
                env->core[core_num].flag = 0;
                immf = env->core[core_num].freg[reg1];
                env->core[core_num].flag |= (immf == 0.0);  // 01 => 0; 10 => -; 00 => +
                env->core[core_num].flag |= (((uint32_t) (immf < 0.0)) << 1);
            break;
            case I_JMP: // JUMP
                env->core[core_num].prcount=addr;
            break;
            case I_JZ: // JZ
                if (env->core[core_num].flag & 1) env->core[core_num].prcount = addr;
            break;
            case I_JNZ: // JNZ
                if (!(env->core[core_num].flag & 1)) env->core[core_num].prcount = addr;
            break;
            case I_JL: // JL
                if ((env->core[core_num].flag & 10)) env->core[core_num].prcount = addr;
            break;
            case I_JG: // JG
                if ((env->core[core_num].flag & 11) == 0) env->core[core_num].prcount = addr;
            break;
            case I_JLE: // JLE
                if (env->core[core_num].flag & 11) env->core[core_num].prcount = addr;
            break;
            case I_JGE: // JGE
                if ((env->core[core_num].flag & 10) == 0) env->core[core_num].prcount = addr;
            break;
            case I_JREG: // JUMP
                env->core[core_num].prcount = addr + env->core[core_num].reg[reg1];
            break;
            case I_CALL: // CALL
                env->core[core_num].callstack[env->core[core_num].link] = env->core[core_num].prcount;
                env->core[core_num].link++;
                env->core[core_num].prcount=addr;
            break;
            case I_RET: // RET
                env->core[core_num].link--;
                env->core[core_num].prcount = env->core[core_num].callstack[env->core[core_num].link];
            break;
            case I_MOV: // MOVE
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_MOVI: // MOVI
                env->core[core_num].reg[reg1] = ((uint64_t) addr);
            break;
            case I_MOVZ: // MOVZ
                if (env->core[core_num].flag & 1) env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_MOVNZ: // MOVNZ
                if (!(env->core[core_num].flag & 1)) env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_MOVL: // MOVL
                if ((env->core[core_num].flag & 10)) env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_MOVG: // MOVG
                if ((env->core[core_num].flag & 11) == 0) env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_MOVLE: // MOVLE
                if (env->core[core_num].flag & 11) env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_MOVGE: // MOVGE
                if ((env->core[core_num].flag & 10) == 0) env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2];
            break;
            case I_LOAD: // LOAD
                env->core[core_num].reg[reg1] = env->vmem[addr].i64;
            break;
            case I_STORE: // STORE
                env->vmem[addr].i64 = env->core[core_num].reg[reg1];                
            break;
            case I_PUSH: // PUSH
                env->core[core_num].stack[env->core[core_num].snext].i64 = env->core[core_num].reg[reg1];
                env->core[core_num].snext += 1;
            break;
            case I_POP: // POP
                env->core[core_num].snext -= 1;
                env->core[core_num].reg[reg1] = env->core[core_num].stack[env->core[core_num].snext].i64;
            break;
            case I_MOVF: // MOVF
                env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_MOVFI: // MOVFI
                env->core[core_num].freg[reg1] = addr;// WEWE
            break;
            case I_MOVFZ: // MOVFZ
                if (env->core[core_num].flag & 1) env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_MOVFNZ: // MOVFNZ
                if (!(env->core[core_num].flag & 1)) env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_MOVFL: // MOVFL
                if ((env->core[core_num].flag & 10)) env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_MOVFG: // MOVFG
                if ((env->core[core_num].flag & 11) == 0) env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_MOVFLE: // MOVFLE
                if (env->core[core_num].flag & 11) env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_MOVFGE: // MOVFGE
                if ((env->core[core_num].flag & 10) == 0) env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2];
            break;
            case I_LOADF: // LOAD
                env->core[core_num].freg[reg1] = env->vmem[addr].f64;
            break;
            case I_STOREF: // STORE
                env->vmem[addr].f64 = env->core[core_num].freg[reg1];                
            break;
            case I_PUSHF: // PUSH
                env->core[core_num].stack[env->core[core_num].snext].f64 = env->core[core_num].freg[reg1];
                env->core[core_num].snext += 1;
            break;
            case I_POPF: // POP
                env->core[core_num].snext -= 1;
                env->core[core_num].freg[reg1] = env->core[core_num].stack[env->core[core_num].snext].f64;
            break;

            case I_NOP:  // NOP
            break;

            case I_ADD:  // ADD
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] + env->core[core_num].reg[reg3];
            break;
            case I_SUB:  // SUB
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] - env->core[core_num].reg[reg3];
            break;
            case I_MUL:  // MUL
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] * env->core[core_num].reg[reg3];
            break;
            case I_DIV: // DIV
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] / env->core[core_num].reg[reg3];
            break;
            case I_ADDF:  // ADDF
                env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2] + env->core[core_num].freg[reg3];
            break;
            case I_SUBF:  // SUBF
                env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2] - env->core[core_num].freg[reg3];
            break;
            case I_MULF:  // MUL
                env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2] * env->core[core_num].freg[reg3];
            break;
            case I_DIVF: // DIVF
                env->core[core_num].freg[reg1] = env->core[core_num].freg[reg2] / env->core[core_num].freg[reg3];
            break;
            case I_MOD: // MOD
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] % env->core[core_num].reg[reg3];
            break;
            case I_INC: // INC
                env->core[core_num].reg[reg1] += 1;
            break;
            case I_DEC: // DEC
                env->core[core_num].reg[reg1] -= 1;
            break;

            case I_AND: // AND
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] & env->core[core_num].reg[reg3];
            break;
            case I_OR: // OR
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] | env->core[core_num].reg[reg3];
            break;
            case I_XOR: // XOR
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] ^ env->core[core_num].reg[reg3];
            break;
            case I_NOT: // NOT
                env->core[core_num].reg[reg1] = ~ env->core[core_num].reg[reg1];
            break;
            case I_SHL: // SHL
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] << env->core[core_num].reg[reg3];
            break;
            case I_SHR: // SHR
                env->core[core_num].reg[reg1] = env->core[core_num].reg[reg2] >> env->core[core_num].reg[reg3];
            break;

            case I_CAST: // CAST
                env->core[core_num].reg[reg1] = (uint64_t) env->core[core_num].freg[reg2];
            break;
            case I_CASTF: // CASTF
                env->core[core_num].freg[reg1] = (double) env->core[core_num].reg[reg2];
            break;

            case I_COS: // COS
                env->core[core_num].freg[reg1] = cos(env->core[core_num].freg[reg2]);
            break;
            case I_SIN: // SEN
                env->core[core_num].freg[reg1] = sin(env->core[core_num].freg[reg2]);
            break;
            case I_TAN: // TAN
                env->core[core_num].freg[reg1] = tan(env->core[core_num].freg[reg2]);
            break;
            case I_ACOS: // ACOS
                env->core[core_num].freg[reg1] = acos(env->core[core_num].freg[reg2]);
            break;
            case I_ASIN: // ASEN
                env->core[core_num].freg[reg1] = asin(env->core[core_num].freg[reg2]);
            break;
            case I_ATAN: // ATAN
                env->core[core_num].freg[reg1] = atan(env->core[core_num].freg[reg2]);
            break;
            case I_COSH: // COSH
                env->core[core_num].freg[reg1] = cosh(env->core[core_num].freg[reg2]);
            break;
            case I_SINH: // SENH
                env->core[core_num].freg[reg1] = sinh(env->core[core_num].freg[reg2]);
            break;
            case I_TANH: // TANH
                env->core[core_num].freg[reg1] = tanh(env->core[core_num].freg[reg2]);
            break;
            case I_ACOSH: // ACOSH
                env->core[core_num].freg[reg1] = acosh(env->core[core_num].freg[reg2]);
            break;
            case I_ASINH: // ASENH
                env->core[core_num].freg[reg1] = asinh(env->core[core_num].freg[reg2]);
            break;
            case I_ATANH: // ATANH
                env->core[core_num].freg[reg1] = atanh(env->core[core_num].freg[reg2]);
            break;

            case I_SQRT: // SQRT
                env->core[core_num].freg[reg1] = sqrt(env->core[core_num].freg[reg2]);
            break;
            case I_POW: // POW
                env->core[core_num].freg[reg1] = pow(env->core[core_num].freg[reg2], env->core[core_num].freg[reg3]);
            break;
            case I_EXP: // EXP
                env->core[core_num].freg[reg1] = exp(env->core[core_num].freg[reg2]);
            break;
            case I_LOG: // LOG
                env->core[core_num].freg[reg1] = log(env->core[core_num].freg[reg2]);
            break;
            case I_LN: // LN
                env->core[core_num].freg[reg1] = log2(env->core[core_num].freg[reg2]);
            break;
            case I_LOG10: // LOG10
                env->core[core_num].freg[reg1] = log10(env->core[core_num].freg[reg2]);
            default:
            break;
        }
    }
}
