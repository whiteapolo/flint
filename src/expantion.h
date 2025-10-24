#ifndef EXPANTION_H
#define EXPANTION_H

#include "eval.h"

typedef struct {
  char **ptr;
  int len;
  int cap;
} String_Array;

void expand_token(Token token, String_Array *out);
char **expand_argv(Token_Array argv);
void expand_aliases(Token_Array *tokens);

#endif
