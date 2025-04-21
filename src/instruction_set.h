#ifndef SIVM_INSTRUCTION_SET_H
#define SIVM_INSTRUCTION_SET_H

#include <stdint.h>

/*
INSTRUCTION 		  0 0 0 0 0 0 0 0
					  | | |
					  | | math op bit
					  | float reg bit
					  vector/gpu
*/
#define INSTR_SET \
	X(EXIT,   0)    /*     EXIT    00000000 */  \
	X(PUSH,   2)    /*     PUSH    00000010 */  \
	X(POP,    3)    /*      POP    00000011 */  \
	X(LOAD,   4)    /*     LOAD    00000100 */  \
	X(STORE,  5)    /*    STORE    00000101 */  \
	X(MOV,    8)    /*      MOV    00001000 */  \
	X(MOVZ,   9)    /*     MOVZ    00001001 */  \
	X(MOVNZ,  10)   /*    MOVNZ    00001010 */  \
	X(MOVL,   11)   /*     MOVL    00001011 */  \
	X(MOVG,   12)   /*     MOVG    00001100 */  \
	X(MOVLE,  13)   /*    MOVLE    00001101 */  \
	X(MOVGE,  14)   /*    MOVGE    00001110 */  \
	X(MOVI,   15)   /*     MOVI    00001111 */  \
	X(JMP,    16)   /*     JMP    00010000 */  \
	X(JZ,     17)   /*      JZ    00010001 */  \
	X(JNZ,    18)   /*     JNZ    00010010 */  \
	X(JL,     19)   /*      JL    00010011 */  \
	X(JG,     20)   /*      JG    00010100 */  \
	X(JLE,    21)   /*     JLE    00010101 */  \
	X(JGE,    22)   /*     JGE    00010110 */  \
	X(JREG,   23)   /*    JREG    00010111 */  \
	X(CALL,   24)   /*    CALL    00011000 */  \
	X(RET,    25)   /*     RET    00011001 */  \
	X(SPAWN,  26)   /*   SPAWN    00011010 */  \
	X(CLC,    29)   /*     CLC    00011101 */  \
	X(CMP,    30)   /*     CMP    00011110 */  \
	X(TEST,   31)   /*    TEST    00011111 */  \
	X(ADD,    33)   /*     ADD    00100001 */  \
	X(SUB,    34)   /*     SUB    00100010 */  \
	X(MUL,    35)   /*     MUL    00100011 */  \
	X(DIV,    36)   /*     DIV    00100100 */  \
	X(MOD,    37)   /*     MOD    00100101 */  \
	X(INC,    38)   /*     INC    00100110 */  \
	X(DEC,    39)   /*     DEC    00100111 */  \
	X(AND,    40)   /*     AND    00101000 */  \
	X(OR,     41)   /*      OR    00101001 */  \
	X(XOR,    42)   /*     XOR    00101010 */  \
	X(NOT,    43)   /*    NOT    00101011 */  \
	X(SHL,    44)   /*    SHL    00101100 */  \
	X(SHR,    45)   /*    SHR    00101101 */  \
	X(CAST,   63)   /*    CAST    00111111 */  \
	X(NOP,    64)   /*     NOP    01000000 */  \
	X(PUSHF,  66)   /*   PUSHF    01000010 */  \
	X(POPF,   67)   /*    POPF    01000011 */  \
	X(LOADF,  68)   /*   LOADF    01000100 */  \
	X(STOREF, 69)   /*  STOREF    01000101 */  \
	X(MOVF,   72)   /*    MOVF    01001000 */  \
	X(MOVFZ,  73)   /*   MOVFZ    01001001 */  \
	X(MOVFNZ, 74)   /*  MOVFNZ    01001010 */  \
	X(MOVFL,  75)   /*   MOVFL    01001011 */  \
	X(MOVFG,  76)   /*   MOVFG    01001100 */  \
	X(MOVFLE, 77)   /*  MOVFLE    01001101 */  \
	X(MOVFGE, 78)   /*  MOVFGE    01001110 */  \
	X(MOVFI,  79)   /*   MOVFI    01001111 */  \
	X(CMPF,   91)   /*   CMPF    01011011 */  \
	X(TESTF,  92)   /*  TESTF    01011100 */  \
	X(ADDF,   97)   /*    ADDF    01100001 */  \
	X(SUBF,   98)   /*    SUBF    01100010 */  \
	X(MULF,   99)   /*    MULF    01100011 */  \
	X(DIVF,   100)  /*   DIVF    01100100 */  \
	X(SQRT,   101)  /*   SQRT    01100101 */  \
	X(POW,    102)  /*    POW    01100110 */  \
	X(EXP,    103)  /*    EXP    01100111 */  \
	X(LN,     104)  /*     LN    01101000 */  \
	X(LOG,    105)  /*    LOG    01101001 */  \
	X(LOG10,  106)  /*  LOG10    01101010 */  \
	X(COS,    112)  /*    COS    01110000 */  \
	X(SIN,    113)  /*    SIN    01110001 */  \
	X(TAN,    114)  /*    TAN    01110010 */  \
	X(ACOS,   115)  /*   ACOS    01110011 */  \
	X(ASIN,   116)  /*   ASIN    01110100 */  \
	X(ATAN,   117)  /*   ATAN    01110101 */  \
	X(COSH,   118)  /*   COSH    01110110 */  \
	X(SINH,   119)  /*   SINH    01110111 */  \
	X(TANH,   120)  /*   TANH    01111000 */  \
	X(ACOSH,  121)  /*  ACOSH    01111001 */  \
	X(ASINH,  122)  /*  ASINH    01111010 */  \
	X(ATANH,  123)  /*  ATANH    01111011 */  \
	X(CASTF,  127)  /*  CASTF    01111111 */ \
	X(STOP, 128)	/*	STOP	 10000000 */ \
	X(CMALLOC, 136) /*	CMALLOC	 10010000 */ \
	X(CFREE, 137) /*	CFREE	 10010000 */ \
	X(CMOVH, 152) /*		CMOVH	 10011000 */ \
	X(CMOVD, 153) /*		CMOVD	 10011001 */


#define X(name, code) I_##name = (code),
enum InstrCode : uint8_t {
INSTR_SET
};
#undef X

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