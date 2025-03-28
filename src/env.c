#include "env.h"


void load_prog(struct Environment *env, instr *prog, uint64_t p_size){
    for(uint64_t i = 0; i < p_size; i++){
        env->program[i] = prog[i];
    }
    env->prcount = 0;
    env->link = 0;
}

void run(struct Environment *env){
    while(1){
        instr code = env->program[env->prcount];
        env->prcount += 1;
        uint8_t instrtype = INSTR_TYPE(code);
        uint8_t reg1 = INSTR_REG1(code);
        uint8_t reg2 = INSTR_REG2(code);
        uint8_t reg3 = INSTR_REG3(code);
        uint32_t addr = INSTR_ADDR(code);
        switch (instrtype){
            case 0: // EXIT
                goto done;
            break;
            case 0b01110000: // CLC
                env->flag = 0;
            break;
            case 0b00100000: // CMP
                double cmp = env->reg[reg1] - env->reg[reg2];
                env->flag |= (cmp == 0);  // 01 => 0; 10 => +; 00 => -
                env->flag |= ((cmp > 0) << 1);
            break;
            case 0b01100000: // TEST
                double tst = env->reg[reg1];
                env->flag |= (tst == 0);  // 01 => 0; 10 => +; 00 => -
                env->flag |= ((tst > 0) << 1);
            break;
            case 0b00010000: // JUMP
                env->prcount=addr;
            break;
            case 0b00010001: // JZ
                if (env->flag & 1) env->prcount = addr;
            break;
            case 0b00010010: // JNZ
                if (!(env->flag & 1)) env->prcount = addr;
            break;
            case 0b00010011: // JL
                if (!(env->flag & 11)) env->prcount = addr;
            break;
            case 0b00010100: // JG
                if ((env->flag & 10)) env->prcount = addr;
            break;
            case 0b00010101: // JLE
                if (!(env->flag & 10)) env->prcount = addr;
            break;
            case 0b00010110: // JGE
                if (env->flag & 11) env->prcount = addr;
            break;
            case 0b00011000: // CALL
                env->callstack[env->link] = env->prcount;
                env->link++;
                env->prcount=addr;
            break;
            case 0b00011001: // RET
                env->link--;
                env->prcount = env->callstack[env->link];
            break;
            case 0b00001000: // MOVE
                env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001111: // MOVI
                env->reg[reg1] = addr;
            break;
            case 0b00001001: // MOVZ
                if (env->flag & 1) env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001010: // MOVNZ
                if (!(env->flag & 1)) env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001011: // MOVL
                if (!(env->flag & 11)) env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001100: // MOVG
                if ((env->flag & 10)) env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001101: // MOVLE
                if (!(env->flag & 10)) env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001110: // MOVGE
                if (env->flag & 11) env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00000100: // LOAD
                env->reg[reg1] = env->vmem[addr];
            break;
            case 0b00000101: // STORE
                env->vmem[addr] = env->reg[reg1];                
            break;
            case 0b00000010: // PUSH
                env->stack[env->snext + 1] = env->reg[reg1];
            break;
            case 0b00000011: // POP
                env->reg[reg1] = env->stack[env->snext - 1];
            break;
            case 0b10000000:  // NOP
                continue;
            break;
            case 0b10000001:  // ADD
                env->reg[reg1] = env->reg[reg2] + env->reg[reg3];
            break;
            case 0b10000010:  // SUB
                env->reg[reg1] = env->reg[reg2] - env->reg[reg3];
            break;
            case 0b10000011:  // MUL
                env->reg[reg1] = env->reg[reg2] * env->reg[reg3];
            break;
            case 0b10000100: // DIV
                env->reg[reg1] = env->reg[reg2] / env->reg[reg3];
            break;
            case 0b10000101: // MOD
                env->reg[reg1] = env->reg[reg2] % env->reg[reg3];
            break;
            case 0b10000110: // INC
                env->reg[reg1] += 1;
            break;
            case 0b10000111: // DEC
                env->reg[reg1] -= 1;
            break;
            case 0b11000000: // AND
                env->reg[reg1] = env->reg[reg2] & env->reg[reg3];
            break;
            case 0b11000001: // OR
                env->reg[reg1] = env->reg[reg2] | env->reg[reg3];
            break;
            case 0b11000010: // XOR
                env->reg[reg1] = env->reg[reg2] ^ env->reg[reg3];
            break;
            case 0b11000011: // NOT
                env->reg[reg1] = ~env->reg[reg1];
            break;
            case 0b11000100: // SHL
                env->reg[reg1] = env->reg[reg2] << env->reg[reg3];
            break;
            case 0b11000101: // SHR
                env->reg[reg1] = env->reg[reg2] >> env->reg[reg3];
            break;
            default:
            break;
        }
    }
done:
}
