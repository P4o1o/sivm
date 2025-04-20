#ifndef SIVM_INSTRUCTION_SET_H
#define SIVM_INSTRUCTION_SET_H

#define INSTRUCTION_SET \
	X()

/*
INSTRUCTION 		  0 0 0 0 0 0 0 0
					  | | |
					  | | math op bit
					  | float reg bit
					  vector/gpu
*/

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

#define I_SPAWN ((uint8_t) 26) //	SPAWN	00011010

#define I_CLC ((uint8_t) 29) //     CLC     00011101
#define I_CMP ((uint8_t) 30) //     CMP     00011110
#define I_TEST ((uint8_t) 31) //    TEST    00011111

#define I_ADD ((uint8_t) 33) //     ADD     00100001
#define I_SUB ((uint8_t) 34) //     SUB     00100010
#define I_MUL ((uint8_t) 35) //     MUL     00100011
#define I_DIV ((uint8_t) 36) //     DIV     00100100
#define I_MOD ((uint8_t) 37) //     MOD     00100101
#define I_INC ((uint8_t) 38) //     INC     00100110
#define I_DEC ((uint8_t) 39) //     DEC     00100111
#define I_AND ((uint8_t) 40) //     AND     00101000
#define I_OR ((uint8_t) 41) //      OR      00101001
#define I_XOR ((uint8_t) 42) //     XOR     00101010
#define I_NOT ((uint8_t) 43) //    NOT     00101011
#define I_SHL ((uint8_t) 44) //    SHL     00101100
#define I_SHR ((uint8_t) 45) //    SHR     00101101

#define I_CAST ((uint8_t) 63) //    CASTI   00111111


#define I_NOP ((uint8_t) 64) //     NOP     01000000

#define I_PUSHF ((uint8_t) 66) //  PUSHF   01000010
#define I_POPF ((uint8_t) 67) //   POPF    01000011

#define I_LOADF ((uint8_t) 68) //  LOADF   01000100
#define I_STOREF ((uint8_t) 69) // STOREF  01000101

#define I_MOVF ((uint8_t) 72) //   MOVF    01001000
#define I_MOVFZ ((uint8_t) 73) //  MOVFZ   01001001
#define I_MOVFNZ ((uint8_t) 74) // MOVFNZ  01001010
#define I_MOVFL ((uint8_t) 75) //  MOVFL   01001011
#define I_MOVFG ((uint8_t) 76) //  MOVFG   01001100
#define I_MOVFLE ((uint8_t) 77) // MOVFLE  01001101
#define I_MOVFGE ((uint8_t) 78) // MOVFGE  01001110
#define I_MOVFI ((uint8_t) 79) //  MOVFI   01001111

#define I_CMPF ((uint8_t) 91) //   CMPF    01011011
#define I_TESTF ((uint8_t) 92) //  TESTF   01011100

#define I_ADDF ((uint8_t) 97) //    ADDF    01100001
#define I_SUBF ((uint8_t) 98) //    SUBF    01100010
#define I_MULF ((uint8_t) 99) //    MULF    01100011
#define I_DIVF ((uint8_t) 100) //   DIVF    01100100
#define I_SQRT ((uint8_t) 101) //   SQRT    01100101
#define I_POW ((uint8_t) 102) //    POW     01100110
#define I_EXP ((uint8_t) 103) //    EXP     01100111
#define I_LN ((uint8_t) 104) //     LN      01101000
#define I_LOG ((uint8_t) 105) //   	LOG     01101001
#define I_LOG10 ((uint8_t) 106) //  LOG10   01101010

#define I_COS ((uint8_t) 112) //    COS     01110000
#define I_SIN ((uint8_t) 113) //    SEN     01110001
#define I_TAN ((uint8_t) 114) //    TAN     01110010
#define I_ACOS ((uint8_t) 115) //   ACOS    01110011
#define I_ASIN ((uint8_t) 116) //   ASEN    01110100
#define I_ATAN ((uint8_t) 117) //   ATAN    01110101
#define I_COSH ((uint8_t) 118) //   COSH    01110110
#define I_SINH ((uint8_t) 119) //   SENH    01110111
#define I_TANH ((uint8_t) 120) //   TANH    01111000
#define I_ACOSH ((uint8_t) 121) //  ACOSH   01111001
#define I_ASINH ((uint8_t) 122) //  ASENH   01111010
#define I_ATANH ((uint8_t) 123) //  ATANH   01111011

#define I_CASTF ((uint8_t) 127) //  CASTF   01111111

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