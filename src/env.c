#include "env.h"

void run(struct Environment *env){
    while(1){
        address now = env->prcount;
        env->prcount += 1;
        instr code = env->program[now];
        uint8_t instrtype = INSTR_TYPE(code);
        uint8_t reg1 = INSTR_REG1(code);
        uint8_t reg2 = INSTR_REG2(code);
        uint8_t reg3 = INSTR_REG3(code);
        uint32_t addr = INSTR_ADDR(code);
        switch (instrtype){
            case 0: // EXIT
                goto done;
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
            case 0b00001000: // MOVE
                env->reg[reg1] = env->reg[reg2];
            break;
            case 0b00001001: // MOVI
                env->reg[reg1] = addr;
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
            case 0b10000000:  // ADD
                env->reg[reg1] = env->reg[reg2] + env->reg[reg3];
            break;
            case 0b10000001:  // SUB
                env->reg[reg1] = env->reg[reg2] - env->reg[reg3];
            break;
            case 0b10000010:  // MUL
                env->reg[reg1] = env->reg[reg2] * env->reg[reg3];
            break;
            case 0b10000011: // DIV
                env->reg[reg1] = env->reg[reg2] / env->reg[reg3];
            break;
            case 0b10000100: // MOD
                env->reg[reg1] = env->reg[reg2] % env->reg[reg3];
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
