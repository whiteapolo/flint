#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"

typedef enum {
    TOKEN_PIPE,
    TOKEN_AND_IF,
    TOKEN_AMPERSAND,
    TOKEN_STRING,
    TOKEN_ERROR,
    TOKEN_EOD,
} Token_Type;

typedef struct {
    Token_Type type;
    Z_String_View lexeme;
} Token;

typedef struct {
    Token *ptr;
    int len;
    int capacity;
} Token_Vec;

void print_token(Token token);
const char *token_type_to_string(Token_Type type);

#endif
