#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"

typedef enum {
    TOKEN_PIPE,
    TOKEN_AND_IF,
    TOKEN_AMPERSAND,
    TOKEN_STRING,
    TOKEN_ERROR,
    TOKEN_STATEMENT_END,
    TOKEN_EOD,

    // keywords
    TOKEN_IF,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_FUN,
    TOKEN_END,
} Token_Type;

typedef struct {
    Token_Type type;
    Z_String_View lexeme;
    int line;
} Token;

typedef struct {
    Token *ptr;
    int len;
    int capacity;
} Token_Vec;

void print_token(Token token);
const char *token_type_to_string(Token_Type type);

#endif
