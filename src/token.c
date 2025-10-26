#include "token.h"
#include "libzatar.h"
#include <stdio.h>
#include <stdlib.h>

Token clone_token(Token token)
{
  Token new_token = {
      .column = token.column,
      .line = token.line,
      .type = token.type,
      .lexeme = strdup(token.lexeme),
  };

  return new_token;
}

Token_Array clone_tokens(Token_Array tokens)
{
  Token_Array new_tokens = {0};

  z_da_foreach(Token *, token, &tokens) {
    z_da_append(&new_tokens, clone_token(*token));
  }

  return new_tokens;
}

void free_token(Token *token)
{
  free(token->lexeme);
}

void free_tokens(Token_Array *tokens)
{
  z_da_foreach(Token *, token, tokens) {
    free_token(token);
  }

  z_da_free(tokens);
}

Token_Type get_keyword_type(Z_String_View lexeme, Token_Type fallback)
{
#define X(type, token_lexeme, is_keyword)                       \
  if (is_keyword && z_sv_equal(lexeme, Z_CSTR(token_lexeme))) { \
    return type;                                                \
  }
  TOKEN_TYPES
#undef X

  return fallback;
}

const char *token_type_to_string(Token_Type type)
{
  switch (type) {
#define X(type, lexeme, is_keyword) case type: return lexeme;
    TOKEN_TYPES
#undef X
    default: return NULL;
  }
}

void print_token(Token token)
{
  printf("Token(%s, \"%s\", line: %d, column: %d)\n", token_type_to_string(token.type), token.lexeme, token.line, token.column);
}

void print_tokens(const Token_Array *tokens)
{
  for (int i = 0; i < tokens->len; i++) {
    print_token(tokens->ptr[i]);
  }
}