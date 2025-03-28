/* C code produced by gperf version 3.1 */
/* Command-line: gperf -L C -t -N inrstr_lookup -K mnem instr.gperf  */
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

#line 7 "instr.gperf"

#include <stdint.h>
struct Inrstr {
    const char *mnem;
    uint8_t code;
    uint8_t addr;
    uint8_t r_num;
};
#line 16 "instr.gperf"
struct Inrstr;

#define TOTAL_KEYWORDS 39
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 5
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 164
/* maximum key range = 163, duplicates = 0 */

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
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165,  20,  15,  15,   0,  10,
      165,   5,  31,  55,   0, 165,   0,   0,  20,   0,
       30, 165,  20,   0,  45,  26,   0, 165,  50, 165,
       15, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165, 165, 165, 165, 165,
      165, 165, 165, 165, 165, 165
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
#line 25 "instr.gperf"
      {"JL",        0b00010011,   1, 0},
#line 48 "instr.gperf"
      {"MOD",       0b10000101,   0, 3},
#line 35 "instr.gperf"
      {"MOVL",      0b00001011,   1, 2},
#line 37 "instr.gperf"
      {"MOVLE",     0b00001101,   1, 2},
      {""},
#line 26 "instr.gperf"
      {"JG",        0b00010100,   1, 0},
      {""},
#line 36 "instr.gperf"
      {"MOVG",      0b00001100,   1, 2},
#line 38 "instr.gperf"
      {"MOVGE",     0b00001110,   1, 2},
      {""}, {""},
#line 27 "instr.gperf"
      {"JLE",       0b00010101,   1, 0},
#line 31 "instr.gperf"
      {"MOVE",      0b00001000,   0, 2},
      {""}, {""},
#line 23 "instr.gperf"
      {"JZ",        0b00010001,   1, 0},
#line 28 "instr.gperf"
      {"JGE",       0b00010110,   1, 0},
#line 33 "instr.gperf"
      {"MOVZ",      0b00001001,   1, 2},
      {""}, {""},
#line 52 "instr.gperf"
      {"OR",        0b11000001,   0, 3},
#line 44 "instr.gperf"
      {"ADD",       0b10000001,   0, 3},
#line 39 "instr.gperf"
      {"LOAD",      0b00000100,   1, 1},
#line 34 "instr.gperf"
      {"MOVNZ",     0b00001010,   1, 2},
      {""}, {""},
#line 50 "instr.gperf"
      {"DEC",       0b10000111,   0, 1},
#line 46 "instr.gperf"
      {"MUL",       0b10000011,   0, 3},
      {""}, {""}, {""},
#line 19 "instr.gperf"
      {"CLC",       0b00111000,   0, 0},
#line 55 "instr.gperf"
      {"SHL",       0b11000100,   0, 3},
      {""}, {""}, {""},
#line 24 "instr.gperf"
      {"JNZ",       0b00010010,   1, 0},
#line 29 "instr.gperf"
      {"CALL",      0b00011000,   1, 0},
      {""}, {""}, {""},
#line 51 "instr.gperf"
      {"AND",       0b11000000,   0, 3},
#line 45 "instr.gperf"
      {"SUB",       0b10000010,   0, 3},
      {""}, {""}, {""},
#line 20 "instr.gperf"
      {"CMP",       0b00100000,   0, 2},
      {""}, {""}, {""}, {""},
#line 43 "instr.gperf"
      {"NOP",       0b10000000,   0, 0},
#line 56 "instr.gperf"
      {"SHR",       0b11000101,   0, 3},
      {""}, {""}, {""},
#line 47 "instr.gperf"
      {"DIV",       0b10000100,   0, 3},
#line 32 "instr.gperf"
      {"MOVI",      0b00001111,   1, 1},
#line 22 "instr.gperf"
      {"JUMP",      0b00010000,   1, 0},
      {""}, {""},
#line 42 "instr.gperf"
      {"POP",       0b00000011,   0, 1},
      {""}, {""}, {""}, {""},
#line 54 "instr.gperf"
      {"NOT",       0b11000011,   0, 2},
      {""},
#line 40 "instr.gperf"
      {"STORE",     0b00000101,   1, 1},
      {""}, {""},
#line 53 "instr.gperf"
      {"XOR",       0b11000010,   0, 3},
      {""}, {""}, {""}, {""},
#line 30 "instr.gperf"
      {"RET",       0b00011001,   1, 0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 41 "instr.gperf"
      {"PUSH",      0b00000010,   0, 1},
      {""},
#line 49 "instr.gperf"
      {"INC",       0b10000110,   0, 1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 21 "instr.gperf"
      {"TEST",      0b00110000,   0, 1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 18 "instr.gperf"
      {"EXIT",      0,            0, 0}
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
#line 57 "instr.gperf"

