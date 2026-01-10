#ifndef TOKEN_H
#define TOKEN_H

#include "libzatar.h"

typedef enum {
  // keywords
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FOR,
  TOKEN_IN,
  TOKEN_BY,
  TOKEN_FUN,
  TOKEN_END,
  TOKEN_STATEMENT_END,

  // logical operators
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_PIPE,
  TOKEN_AMPERSAND,

  // strings
  TOKEN_WORD,
  TOKEN_SQUOTED_STRING,
  TOKEN_DQUOTED_STRING,

  TOKEN_ERROR,
  TOKEN_EOD,
  TOKEN_COUNT, // always last
} Token_Type;

const char *keywords_lexeme[TOKEN_COUNT] = {
  [TOKEN_OR]    = "or",
  [TOKEN_IF]    = "if",
  [TOKEN_IN]    = "in",
  [TOKEN_BY]    = "by",
  [TOKEN_FOR]   = "for",
  [TOKEN_FUN]   = "fn",
  [TOKEN_END]   = "end",
  [TOKEN_ELSE]  = "else",
  [TOKEN_WHILE] = "while",
};

const char *token_type_to_string[TOKEN_COUNT] = {
  [TOKEN_OR]             = "or",
  [TOKEN_IF]             = "if",
  [TOKEN_IN]             = "in",
  [TOKEN_BY]             = "by",
  [TOKEN_FOR]            = "for",
  [TOKEN_FUN]            = "fn",
  [TOKEN_END]            = "end",
  [TOKEN_AND]            = "and",
  [TOKEN_EOD]            = "eod",
  [TOKEN_PIPE]           = "pipe",
  [TOKEN_WORD]           = "word",
  [TOKEN_ELSE]           = "else",
  [TOKEN_WHILE]          = "while",
  [TOKEN_ERROR]          = "error",
  [TOKEN_AMPERSAND]      = "ampersand",
  [TOKEN_STATEMENT_END]  = "statement_end",
  [TOKEN_SQUOTED_STRING] = "squoted_string",
  [TOKEN_DQUOTED_STRING] = "dquoted_string",
};

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
Token_Type get_keyword_type(Z_String_View lexeme, Token_Type fallback);
void print_tokens(const Token_Array *tokens);

#endif
