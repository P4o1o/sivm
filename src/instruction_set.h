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
#define I_JREG ((uint8_t) 23) //    JREG    00010111

#define I_CALL ((uint8_t) 24) //    CALL    00011000
#define I_RET ((uint8_t) 25) //     RET     00011001

#define I_CLC ((uint8_t) 56) //     CLC     00111000
#define I_CMP ((uint8_t) 32) //     CMP     00100000
#define I_TEST ((uint8_t) 48) //    TEST    00110000

#define I_NOP ((uint8_t) 128) //    NOP     10000000

#define I_ADD ((uint8_t) 65) //     ADD     01000001
#define I_SUB ((uint8_t) 66) //     SUB     01000010
#define I_MUL ((uint8_t) 67) //     MUL     01000011
#define I_DIV ((uint8_t) 68) //     DIV     01000100
#define I_MOD ((uint8_t) 69) //     MOD     01000101
#define I_INC ((uint8_t) 70) //     INC     01000110
#define I_DEC ((uint8_t) 71) //     DEC     01000111

#define I_AND ((uint8_t) 97) //     AND     01100000
#define I_OR ((uint8_t) 98) //      OR      01100001
#define I_XOR ((uint8_t) 99) //     XOR     01100010
#define I_NOT ((uint8_t) 100) //    NOT     01100011
#define I_SHL ((uint8_t) 101) //    SHL     01100100
#define I_SHR ((uint8_t) 102) //    SHR     01100101

#define I_CAST ((uint8_t) 112) //  CASTI   01110000

#define I_PUSHF ((uint8_t) 130) //  PUSH    10000010
#define I_POPF ((uint8_t) 131) //   POP     10000011

#define I_LOADF ((uint8_t) 132) //  LOAD    10000100
#define I_STOREF ((uint8_t) 133) // STORE   10000101

#define I_MOVF ((uint8_t) 136) //   MOVF    10001000
#define I_MOVFZ ((uint8_t) 137) //  MOVFZ   10001001
#define I_MOVFNZ ((uint8_t) 138) // MOVFNZ  10001010
#define I_MOVFL ((uint8_t) 139) //  MOVFL   10001011
#define I_MOVFG ((uint8_t) 140) //  MOVFG   10001100
#define I_MOVFLE ((uint8_t) 141) // MOVFLE  10001101
#define I_MOVFGE ((uint8_t) 142) // MOVFGE  10001110
#define I_MOVFI ((uint8_t) 143) //  MOVFI   10001111

#define I_CMPF ((uint8_t) 160) //   CMPF    10100000
#define I_TESTF ((uint8_t) 176) //  TESTF   10110000

#define I_ADDF ((uint8_t) 193) //   ADDF    11000001
#define I_SUBF ((uint8_t) 194) //   SUBF    11000010
#define I_MULF ((uint8_t) 195) //   MULF    11000011
#define I_DIVF ((uint8_t) 196) //   DIVF    11000100

#define I_CASTF ((uint8_t) 240) //  CASTF   11110000

#ifdef __GNUC__
	#define UNREACHABLE __builtin_unreachable()
#else
#ifdef _MSC_VER
	#define UNREACHABLE __assume(0);
#else
	[[noreturn]] inline void unreachable(){}
	#define UNREACHABLE unreachable()
#endif
#endif

#endif