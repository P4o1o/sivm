#ifndef SIVM_INSTRUCTION_SET_H
#define SIVM_INSTRUCTION_SET_H

#define I_EXT ((uint8_t) 0) //      EXIT    00000000

#define I_PUSH ((uint8_t) 2) //     PUSH    00000010
#define I_POP ((uint8_t) 3) //      POP     00000011

#define I_LOAD ((uint8_t) 4) //     LOAD    00000100
#define I_STORE ((uint8_t) 5) //    STORE   00000101

#define I_MOV ((uint8_t) 8) //      MOV     00001000
#define I_MOVZ ((uint8_t) 9) //     MOVZ    00001001
#define I_MOVNZ ((uint8_t) 10) //   MOVNZ   00001010
#define I_MOVL ((uint8_t) 11) //    MOVL    00001011
#define I_MOVG ((uint8_t) 12) //    MOVG    00001100
#define I_MOVLE ((uint8_t) 13) //   MOVLE   00001101
#define I_MOVGE ((uint8_t) 14) //   MOVGE   00001110
#define I_MOVI ((uint8_t) 15) //    MOVI    00001111

#define I_JMP ((uint8_t) 16) //     JMP     00010000
#define I_JZ ((uint8_t) 17) //      JZ      00010001
#define I_JNZ ((uint8_t) 18) //     JNZ     00010010
#define I_JL ((uint8_t) 19) //      JL      00010011
#define I_JG ((uint8_t) 20) //      JG      00010100
#define I_JLE ((uint8_t) 21) //     JLE     00010101
#define I_JGE ((uint8_t) 22) //     JGE     00010110

#define I_CALL ((uint8_t) 24) //    CALL    00011000
#define I_RET ((uint8_t) 25) //     RET     00011001

#define I_CLC ((uint8_t) 112) //    CLC     01110000
#define I_CMP ((uint8_t) 32) //     CMP     00100000
#define I_TEST ((uint8_t) 96) //    TEST    01100000

#define I_NOP ((uint8_t) 128) //    NOP     10000000

#define I_ADD ((uint8_t) 129) //    ADD     10000001
#define I_SUB ((uint8_t) 130) //    SUB     10000010
#define I_MUL ((uint8_t) 131) //    MUL     10000011
#define I_DIV ((uint8_t) 132) //    DIV     10000100
#define I_MOD ((uint8_t) 133) //    MOD     10000101
#define I_INC ((uint8_t) 134) //    INC     10000110
#define I_DEC ((uint8_t) 135) //    DEC     10000111

#define I_AND ((uint8_t) 192) //    AND     11000000
#define I_OR ((uint8_t) 193) //     OR      11000001
#define I_XOR ((uint8_t) 194) //    XOR     11000010
#define I_NOT ((uint8_t) 195) //    NOT     11000011
#define I_SHL ((uint8_t) 196) //    SHL     11000100
#define I_SHR ((uint8_t) 197) //    SHR     11000101


#endif