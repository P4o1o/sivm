#include <stdio.h>
#include <stdlib.h>
#include "env.h"
#include "assmblr.h"

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <nome_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    struct Environment env;
    FILE *source;
    source = fopen(argv[1], "r");
    fseek(source, 0, SEEK_END);
    long file_size = ftell(source);
    rewind(source);
    char *code = malloc(file_size + 1);
    if (!code) {
        perror("Errore nell'allocazione della memoria");
        fclose(source);
        return EXIT_FAILURE;
    } size_t read_size = fread(code, 1, file_size, source);
    if (read_size != file_size) {
        perror("Errore nella lettura del file");
        free(code);
        fclose(source);
        return EXIT_FAILURE;
    }
    code[file_size] = '\0';
    fclose(source);
    char *restofcode = assemble_load_values(&env, code);
    printf("Values Loaded\n\n");
    printf("%s\n\n", restofcode);
    assemble_load_code(&env, restofcode);
    run(&env);
    printf("--------------------- Registers --------------------\n\n");
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 8; col++) {
            int idx = row * 8 + col;
            printf("| r%-2d:%8ld |", idx, env.reg[idx]);
        }
        printf("\n\n");
    }
    printf("-----------------------------------------------------\n\n");
    printf("------------------ Float Registers ------------------\n\n");
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 8; col++) {
            int idx = row * 8 + col;
            printf("| r%-2d:%8lf |", idx, env.freg[idx]);
        }
        printf("\n\n");
    }
    printf("-----------------------------------------------------\n");

    free(code);
    return EXIT_SUCCESS;
}
