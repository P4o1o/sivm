#include "assmblr.h"
#include "instr_hash.h"

int parse_error;

inline char *parse_addr_hex(char *code, address *res){
    *res = 0;
    while (1) {
        if(*code >= '0' && *code <= '9')
            *res = (*res << 4) | (*code - '0');
        else if(toupper((unsigned char) *code) >= 'A' && toupper((unsigned char) *code) <= 'F'){
            *res = (*res << 4) | (*code - 'A' + 10);
        }else{
            return code;
        }
        code++;
    }
}

inline char *parse_uint_hex(char *code, uint64_t *res){
    *res = 0;
    while (1) {
        if(*code >= '0' && *code <= '9')
            *res = (*res << 4) | (*code - '0');
        else if(toupper((unsigned char) *code) >= 'A' && toupper((unsigned char) *code) <= 'F'){
            *res = (*res << 4) | (*code - 'A' + 10);
        }else{
            return code;
        }
        code++;
    }
}

inline char *parse_addr_bin(char *code, address *res){
    *res = 0;
    while (*code == '0' || *code == '1') {
        *res = (*res << 1) | (*code - '0');
        code++;
    }
    return code;
}

inline char *parse_uint_bin(char *code, uint64_t *res){
    *res = 0;
    while (*code == '0' || *code == '1') {
        *res = (*res << 1) | (*code - '0');
        code++;
    }
    return code;
}

char *assemble_load_values(struct Environment *env, char *code){
    while(*code != '\0'){
        while(*code == '\n' || IS_INDENT(*code)){
            code++;
        }
        if(*code == '@'){
            code++;
            address addr;
            while(IS_INDENT(*code)){
                code++;
            }
            if(toupper((unsigned char) *code) == 'X'){
                code++;
                code = parse_addr_hex(code, &addr);
            }else if(toupper((unsigned char) *code) == 'B'){
                code++;
                code = parse_addr_bin(code, &addr);
            }else if(*code >= '0' && *code <= '9'){
                addr = *code - '0';
                code++;
                while (*code >= '0' && *code <= '9') {
                    addr = addr * 10 + (*code - '0');
                    code++;
                }
            }else{
                printf("\nError unvalid address: %16s\n", code);
                parse_error = 2;
                return code;
            }
            while(IS_INDENT(*code)){
                code++;
            }
            if(*code != ':'){
                printf("\nError ':' is missing here: %16s\n", code);
                parse_error = 1;
                return code;
            }
            code++;
            while(IS_INDENT(*code)){
                code++;
            }
            uint64_t value;
            if(toupper((unsigned char) *code) == 'X'){
                code++;
                code = parse_uint_hex(code, &value);
            }else if(toupper((unsigned char) *code) == 'B'){
                code++;
                code = parse_uint_bin(code, &value);
            }else if(*code >= '0' && *code <= '9'){
                value = *code - '0';
                code++;
                while (*code >= '0' && *code <= '9') {
                    value = value * 10 + (*code - '0');
                    code++;
                }
            }else{
                printf("\nError unvalid value: %16s\n", code);
                parse_error = 3;
                return code;
            }
            env->vmem[addr].i64 = value;
        }else{
            break;
        }
    }
    return code;
}

int assemble_load_code(struct Environment *env, char *code){
    address current = 0;
    memset(env->core, 0, sizeof(struct Core) * CORE_NUM);
    while(*code != '\0'){
        while(*code == '\n' || IS_INDENT(*code)){
            code++;
        }
        parse_error = 0;
        code = assemble_line(code, &(env->program[current]));
        if(parse_error != 0){
            printf("\nError at line num %d; %16s\n", current, code);
            return parse_error;
        }
        current++;
    }
    return 0;
}

char *assemble_line(char *line, instr *res){
    char mnemonic[8];
    size_t i = 0;
    while(! IS_INDENT(*line) && *line != '\n' && *line != '\0') {
        mnemonic[i] = toupper((unsigned char) *line);
        line++;
        i++;
    }
    mnemonic[i] = '\0';
    const struct Inrstr *op = inrstr_lookup(mnemonic, i);
    if (!op) {
        printf("\nMnemonic sconosciuto: %s in %16s\n", mnemonic, line);
        parse_error = -1;
        return line;
    }
    *res = (instr) op->code;
    uint8_t regs[7];
    address addr;
    for(int i = 0; i < op->r_num; i++){
        while(IS_INDENT(*line)){
            line++;
        }
        if(*line == '\n' || *line == '\0'){
            printf("\nInstruction %s expects %d register arguments, got %d\n", mnemonic, op->r_num, i);
            parse_error = -2;
            return line;
        }
        if(toupper((unsigned char) *line) == 'R'){
            line++;
            regs[i] = 0;
            while (*line >= '0' && *line <= '9') {
                regs[i] = regs[i] * 10 + (*line - '0');
                line++;
            }
            if(*line != '\n' && *line != '\0' && ! IS_INDENT(*line)){
                printf("\nUnespected char after a register: %s\n", line);
                parse_error = -6;
                return line;
            }
        }else if(toupper((unsigned char) *line) == 'F'){
            if(i == 0) *res |= 64;
            line++;
            regs[i] = 0;
            while (*line >= '0' && *line <= '9') {
                regs[i] = regs[i] * 10 + (*line - '0');
                line++;
            }
            if(*line != '\n' && *line != '\0' && ! IS_INDENT(*line)){
                printf("\nUnespected char after a register: %s\n", line);
                parse_error = -6;
                return line;
            }
        }else{
            printf("\nInstruction %s expects a register arguments, got %s\n", mnemonic, line);
            parse_error = -4;
            return line;
        }
    }
    if(op->addr){
        while(IS_INDENT(*line)){
            line++;
        }
        if(*line == '\n' || *line == '\0'){
            printf("\nInstruction %s expects an address argument\n", mnemonic);
            parse_error = -3;
            return line;
        }
        if(toupper((unsigned char) *line) == 'X'){
            line++;
            line = parse_addr_hex(line, &addr);
            if(*line != '\n' && *line != '\0' && ! IS_INDENT(*line)){
                printf("\nUnespected char after an address: %s\n", line);
                parse_error = -5;
                return line;
            }
        }else if(toupper((unsigned char) *line) == 'B'){
            line++;
            line = parse_addr_bin(line, &addr);
            if(*line != '\n' && *line != '\0' && ! IS_INDENT(*line)){
                printf("\nUnespected char after an address: %s\n", line);
                parse_error = -5;
                return line;
            }
        }else if(*line >= '0' && *line <= '9'){
            addr = *line - '0';
            line++;
            while (*line >= '0' && *line <= '9') {
                addr = addr * 10 + (*line - '0');
                line++;
            }
            if(*line != '\n' && *line != '\0' && ! IS_INDENT(*line)){
                printf("\nUnespected char after an address: %s\n", line);
                parse_error = -5;
                return line;
            }
        }else{
            printf("\nInstruction %s expects an address arguments, got %s\n", mnemonic, line);
            parse_error = -7;
            return line;
        }
        *res |= ((instr) addr) << ((instr) 32);
    }
    switch(op->r_num){
        case 7:
            *res |= ((instr) regs[6] << (instr) 8);
        case 6:
            *res |= ((instr) regs[5] << (instr) 56);
        case 5:
            *res |= ((instr) regs[4] << (instr) 48);
        case 4:
            *res |= ((instr) regs[3] << (instr) 40);
        case 3:
            *res |= ((instr) regs[2] << (instr) 32);
        case 2:
            *res |= ((instr) regs[1] << (instr) 16);
        case 1:
            *res |= ((instr) regs[0] << (instr) 24);
        case 0:
        break;

        default:
            UNREACHABLE;
    }
    return line;
}