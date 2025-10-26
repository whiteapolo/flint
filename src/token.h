#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"

#define TOKEN_TYPES                              \
  X(TOKEN_OR,             "or",             1)   \
  X(TOKEN_IF,             "if",             1)   \
  X(TOKEN_IN,             "in",             1)   \
  X(TOKEN_BY,             "by",             1)   \
  X(TOKEN_FOR,            "for",            1)   \
  X(TOKEN_FUN,            "fun",            1)   \
  X(TOKEN_END,            "end",            1)   \
  X(TOKEN_AND,            "and",            1)   \
  X(TOKEN_EOD,            "eod",            0)   \
  X(TOKEN_PIPE,           "pipe",           0)   \
  X(TOKEN_WORD,           "word",           0)   \
  X(TOKEN_ELSE,           "else",           1)   \
  X(TOKEN_WHILE,          "while",          1)   \
  X(TOKEN_ERROR,          "error",          0)   \
  X(TOKEN_AMPERSAND,      "ampersand",      0)   \
  X(TOKEN_STATEMENT_END,  "statement_end",  0)   \
  X(TOKEN_SQUOTED_STRING, "squoted_string", 0)   \
  X(TOKEN_DQUOTED_STRING, "dquoted_string", 0)

typedef enum {
#define X(type, lexeme, is_keyword) type,
  TOKEN_TYPES
#undef X
} Token_Type;

typedef struct {
  Token_Type type;
  char *lexeme;
  int line;
  int column;
} Token;

typedef struct {
  Token *ptr;
  int len;
  int cap;
} Token_Array;

void free_token(Token *token);
void free_tokens(Token_Array *tokens);
Token clone_token(Token token);
Token_Array clone_tokens(Token_Array tokens);
void print_token(Token token);
const char *token_type_to_string(Token_Type type);
Token_Type get_keyword_type(Z_String_View lexeme, Token_Type fallback);

#endif
