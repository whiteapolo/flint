#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"

#define TOKEN_TYPES                          \
  X(TOKEN_OR, "OR")                          \
  X(TOKEN_IF, "IF")                          \
  X(TOKEN_IN, "IN")                          \
  X(TOKEN_BY, "BY")                          \
  X(TOKEN_FOR, "FOR")                        \
  X(TOKEN_FUN, "FUN")                        \
  X(TOKEN_END, "END")                        \
  X(TOKEN_AND, "AND")                        \
  X(TOKEN_PIPE, "PIPE")                      \
  X(TOKEN_WORD, "WORD")                      \
  X(TOKEN_ELSE, "ELSE")                      \
  X(TOKEN_EOD, "EOD")                        \
  X(TOKEN_WHILE, "WHILE")                    \
  X(TOKEN_ERROR, "ERROR")                    \
  X(TOKEN_UNKOWN, "UNKOWN")                  \
  X(TOKEN_AMPERSAND, "AMPERSAND")            \
  X(TOKEN_STATEMENT_END, "STATEMENT_END")    \
  X(TOKEN_SQUOTED_STRING, "SQUOTED_STRING")  \
  X(TOKEN_DQUOTED_STRING, "DQUOTED_STRING")

typedef enum {
#define X(type, lexeme) type,
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
