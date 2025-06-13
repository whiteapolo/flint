#ifndef LEXEL_H
#define LEXEL_H

#include "token.h"
#include "libzatar.h"

#define VARIBALE_EXPENTION  (1 << 0) // only expand in the end
#define ALIAS_EXPENSION     (1 << 1) // expand after lexing
#define TILDE_EXPENSION     (1 << 2) // only expand in the end
#define GLOB_EXPENSTION     (1 << 3) // only expand in the end

typedef struct {
    const char *end;
    const char *start;
    const char *curr;
} Lexer;

Token_Vec lexer_get_tokens(Z_String_View source);

#endif
