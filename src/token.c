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

Token dup_token(Token token)
{
  Token new_token = {
      .column = token.column,
      .line = token.line,
      .type = token.type,
      .lexeme = strdup(token.lexeme),
  };

  return new_token;
}

void free_token(Token *token)
{
  free(token->lexeme);
}

void free_tokens(Token_Vec *tokens)
{
  z_da_foreach(Token *, token, tokens) {
    free_token(token);
  }
  z_da_free(tokens);
}

const int keywords_len = Z_ARRAY_LEN(keywords);

Optional_Token_Type get_keyword_type(Z_String_View lexeme)
{
  for (int i = 0; i < keywords_len; i++) {
    if (!z_sv_compare(lexeme, Z_CSTR(keywords[i].lexeme))) {
      return (Optional_Token_Type)Z_Optional_Ok(keywords[i].type);
    }
  }

  return (Optional_Token_Type)Z_Optional_None();
}

const char *token_type_to_string(Token_Type type)
{
  switch (type) {
    case TOKEN_PIPE: return "PIPE";
    case TOKEN_AND: return "AND";
    case TOKEN_OR: return "OR";
    case TOKEN_AMPERSAND: return "AMPERSAND";
    case TOKEN_ERROR: return "ERROR";
    case TOKEN_EOD: return "EOD";
    case TOKEN_STATEMENT_END: return "STATEMENT_END";
    case TOKEN_WORD: return "TOKEN_WORD";
    case TOKEN_DQUOTED_STRING: return "TOKEN_DQUOTED_STRING";
    case TOKEN_SQUOTED_STRING: return "TOKEN_SQUOTED_STRING";
    case TOKEN_FOR: return "for";
    case TOKEN_IF: return "if";
    case TOKEN_IN: return "in";
    case TOKEN_FUN: return "fn";
    case TOKEN_END: return "end";
    case TOKEN_ELSE: return "else";
    case TOKEN_WHILE: return "while";
    case TOKEN_BY: return "by";
    default: return "UNKNOWN";
  }
}

void print_token(Token token)
{
  const char *type_str = token_type_to_string(token.type);
  printf("Token(%s, \"%s\")\n", type_str, token.lexeme);
}
