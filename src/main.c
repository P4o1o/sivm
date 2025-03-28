#include <stdio.h>
#include <stdlib.h>
#include "env.h"

int main(int argc, char *argv[]){
    struct Environment env;
    uint64_t values[5] = {11, 22, 33, 44, 55};
    load_value(&env, values, 32, 5);
    uint64_t instr = ((uint64_t) 32) << ((uint64_t) 32);
    // instr |= (0 << 24);
    instr |= I_MOVI;
    printf("%ld\n", instr);
    uint64_t program[2] = {instr, 0};
    load_prog(&env, program, 2);
    run(&env);
    printf("%ld\n", env.reg[0]);
    return 0;
}
