#ifndef LEXEL_H
#define LEXEL_H

#include "token.h"
#include "libzatar.h"

typedef struct {
    const char *end;
    const char *start;
    const char *curr;
} Lexer;

Token_Vec lexer_get_tokens(Z_String_View source);
void lexer_print_tokens(const Token_Vec *tokens);


#endif
