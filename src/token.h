#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"

typedef enum {
    TOKEN_PIPE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_AMPERSAND,
    TOKEN_ERROR,
    TOKEN_STATEMENT_END,
    TOKEN_EOD,

    TOKEN_WORD,
    TOKEN_DQUOTED_STRING,
    TOKEN_SQUOTED_STRING,

    TOKEN_UNKOWN,

    // keywords
    TOKEN_IF,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_FUN,
    TOKEN_END,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_BY,
} Token_Type;

typedef struct {
    Token_Type type;
    Z_String_View lexeme;
    int line;
    int column;
} Token;

typedef struct {
    Token *ptr;
    int len;
    int capacity;
} Token_Vec;

void print_token(Token token);
const char *token_type_to_string(Token_Type type);

#endif
