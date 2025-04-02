/* C code produced by gperf version 3.1 */
/* Command-line: gperf -L C -t -N inrstr_lookup -K mnem src/instr.gperf  */
/* Computed positions: -k'1-4' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 7 "src/instr.gperf"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "instruction_set.h"
struct Inrstr {
    const char *mnem;
    uint8_t code;
    uint8_t addr;
    uint8_t r_num;
};
#line 19 "src/instr.gperf"
struct Inrstr;

#define TOTAL_KEYWORDS 40
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 5
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 159
/* maximum key range = 158, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register size_t len;
{
  static unsigned char asso_values[] =
    {
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160,  20,   5,  15,   0,  10,
      160,   5,  46,  55,   0, 160,   0,   0,  20,   0,
       30, 160,  25,   0,  45,  26,   0, 160,  45, 160,
       15, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160, 160, 160, 160, 160,
      160, 160, 160, 160, 160, 160
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct Inrstr *
inrstr_lookup (str, len)
     register const char *str;
     register size_t len;
{
  static struct Inrstr wordlist[] =
    {
      {""}, {""},
#line 28 "src/instr.gperf"
      {"JL",        I_JL,    1, 0},
#line 52 "src/instr.gperf"
      {"MOD",       I_MOD,   0, 3},
#line 39 "src/instr.gperf"
      {"MOVL",      I_MOVL,  1, 2},
#line 41 "src/instr.gperf"
      {"MOVLE",     I_MOVLE, 1, 2},
      {""},
#line 29 "src/instr.gperf"
      {"JG",        I_JG,    1, 0},
      {""},
#line 40 "src/instr.gperf"
      {"MOVG",      I_MOVG,  1, 2},
#line 42 "src/instr.gperf"
      {"MOVGE",     I_MOVGE, 1, 2},
      {""}, {""},
#line 30 "src/instr.gperf"
      {"JLE",       I_JLE,   1, 0},
#line 35 "src/instr.gperf"
      {"MOVE",      I_MOV,   0, 2},
      {""}, {""},
#line 26 "src/instr.gperf"
      {"JZ",        I_JZ,    1, 0},
#line 31 "src/instr.gperf"
      {"JGE",       I_JGE,   1, 0},
#line 37 "src/instr.gperf"
      {"MOVZ",      I_MOVZ,  1, 2},
      {""}, {""}, {""},
#line 48 "src/instr.gperf"
      {"ADD",       I_ADD,   0, 3},
#line 43 "src/instr.gperf"
      {"LOAD",      I_LOAD,  1, 1},
#line 38 "src/instr.gperf"
      {"MOVNZ",     I_MOVNZ, 1, 2},
      {""},
#line 56 "src/instr.gperf"
      {"OR",        I_OR,    0, 3},
#line 54 "src/instr.gperf"
      {"DEC",       I_DEC,   0, 1},
#line 50 "src/instr.gperf"
      {"MUL",       I_MUL,   0, 3},
      {""}, {""}, {""},
#line 22 "src/instr.gperf"
      {"CLC",       I_CLC,   0, 0},
#line 49 "src/instr.gperf"
      {"SUB",       I_SUB,   0, 3},
      {""}, {""}, {""},
#line 27 "src/instr.gperf"
      {"JNZ",       I_JNZ,   1, 0},
#line 33 "src/instr.gperf"
      {"CALL",      I_CALL,  1, 0},
      {""}, {""}, {""},
#line 55 "src/instr.gperf"
      {"AND",       I_AND,   0, 3},
#line 32 "src/instr.gperf"
      {"JREG",      I_JREG,  1, 1},
      {""}, {""}, {""},
#line 23 "src/instr.gperf"
      {"CMP",       I_CMP,   0, 2},
#line 59 "src/instr.gperf"
      {"SHL",       I_SHL,   0, 3},
      {""}, {""}, {""},
#line 47 "src/instr.gperf"
      {"NOP",       I_NOP,   0, 0},
      {""}, {""}, {""}, {""},
#line 51 "src/instr.gperf"
      {"DIV",       I_DIV,   0, 3},
#line 36 "src/instr.gperf"
      {"MOVI",      I_MOVI,  1, 1},
#line 25 "src/instr.gperf"
      {"JUMP",      I_JMP,   1, 0},
      {""}, {""},
#line 46 "src/instr.gperf"
      {"POP",       I_POP,   0, 1},
      {""}, {""}, {""}, {""},
#line 58 "src/instr.gperf"
      {"NOT",       I_NOT,   0, 2},
      {""}, {""}, {""}, {""},
#line 57 "src/instr.gperf"
      {"XOR",       I_XOR,   0, 3},
#line 60 "src/instr.gperf"
      {"SHR",       I_SHR,   0, 3},
#line 44 "src/instr.gperf"
      {"STORE",     I_STORE, 1, 1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 34 "src/instr.gperf"
      {"RET",       I_RET,   0, 0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 53 "src/instr.gperf"
      {"INC",       I_INC,   0, 1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 24 "src/instr.gperf"
      {"TEST",      I_TEST,  0, 1},
      {""},
#line 45 "src/instr.gperf"
      {"PUSH",      I_PUSH,  0, 1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 21 "src/instr.gperf"
      {"EXIT",      I_EXT,   0, 0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].mnem;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 61 "src/instr.gperf"

