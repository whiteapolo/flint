#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"
typedef enum {
    TOKEN_TYPE_PIPE,
    TOKEN_TYPE_AND_IF,
    TOKEN_TYPE_AMPERSAND,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_ERROR,
    TOKEN_TYPE_EOD,
} TOKEN_TYPE;

typedef struct {
    TOKEN_TYPE type;
    Z_String_View lexeme;
} Token;

typedef struct {
    Token *ptr;
    int len;
    int capacity;
} Token_Vec;

void print_token(Token token);
const char *token_type_to_string(TOKEN_TYPE type);

#endif
