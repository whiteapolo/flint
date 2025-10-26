#include "token.h"
#include "libzatar.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Token_Type type;
  const char *lexeme;
} Keyword;

const Keyword keywords[] = {
    { .type = TOKEN_FOR,   .lexeme = "for"   },
    { .type = TOKEN_IF,    .lexeme = "if"    },
    { .type = TOKEN_END,   .lexeme = "end"   },
    { .type = TOKEN_IN,    .lexeme = "in"    },
    { .type = TOKEN_FUN,   .lexeme = "fn"    },
    { .type = TOKEN_ELSE, .lexeme = "else"   },
    { .type = TOKEN_WHILE, .lexeme = "while" },
    { .type = TOKEN_BY,    .lexeme = "by"    },
};

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
  for (int i = 0; i < (int)Z_ARRAY_LEN(keywords); i++) {
    if (z_sv_equal(lexeme, Z_CSTR(keywords[i].lexeme))) {
      return keywords[i].type;
    }
  }

  return fallback;
}

const char *token_type_to_string(Token_Type type)
{
  switch (type) {
#define X(type, lexeme) case type: return lexeme;
    TOKEN_TYPES
#undef X
    default: return "UNKNOWN";
  }
}

void print_token(Token token)
{
  printf("Token(%s, \"%s\", line: %d, column: %d)\n", token_type_to_string(token.type), token.lexeme, token.line, token.column);
}
