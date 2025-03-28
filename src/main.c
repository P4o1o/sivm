#include <stdio.h>
#include <stdlib.h>
#include "env.h"
#include "assmblr.h"

int main(int argc, char *argv[]){
    struct Environment env;
    uint64_t values[5] = {11, 22, 33, 44, 55};
    load_value(&env, values, 32, 5);
    if(assemble_load_code(&env, "MOVI R0 32\nMOVI R1 34\nsub R3 R1 R0\nEXIT") != 0)
        exit(-1);
    run(&env);
    printf("%ld\n", env.reg[0]);
    printf("%ld\n", env.reg[1]);
    printf("%ld\n", env.reg[3]);
    return 0;
}
