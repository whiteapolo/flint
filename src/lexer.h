#ifndef LEXEL_H
#define LEXEL_H

#include "token.h"
#include "libzatar.h"

Token_Vec lexer_get_tokens(Z_String_View source);
void lexer_print_tokens(const Token_Vec *tokens);
void alias_expension(Token_Vec *tokens);

#endif
