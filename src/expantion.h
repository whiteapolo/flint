#ifndef EXPANTION_H
#define EXPANTION_H

#include "eval.h"

typedef struct {
  char **ptr;
  int len;
  int cap;
} String_Vec;

void expand_token(Token token, String_Vec *out);
char **expand_argv(Argv argv);
void expand_aliases(Token_Vec *tokens);

#endif
