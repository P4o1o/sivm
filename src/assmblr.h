#ifndef SIVM_ASSMBLR_H
#define SIVM_ASSMBLR_H
#include "env.h"
#include <stdio.h>
#include <ctype.h>

#define IS_INDENT(c) ((c == '\t') || (c == '\r') || (c == '\a') || (c == '\v') || (c == '\b') || (c == ' '))


int assemble_load_code(struct Environment *env, char *code);
char *assemble_line(char *line, uint64_t *res);

#endif