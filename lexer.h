#ifndef LEXEL_H
#define LEXEL_H

#include "token.h"

typedef struct {
    const char *source;
    const char *curr;
    const char *end;
} Lexer;

void lexer_init(Lexer *lexer, const char *source, int len);
Token lexer_next(Lexer *lexer);
Token lexer_peek(const Lexer *lexer);

#endif
