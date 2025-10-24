#include "lexer.h"
#include "builtins/builtin.h"
#include "cstr.h"
#include "error.h"
#include "libzatar.h"
#include "token.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  bool had_error;
  Z_Scanner scanner;
} Lexer_State;

Lexer_State *lexer_state = NULL;

void create_new_lexer_state(Z_String_View source)
{
  lexer_state = malloc(sizeof(Lexer_State));
  lexer_state->had_error = false;
  lexer_state->scanner = z_scanner_new(source);
}

void free_lexer_state()
{
  free(lexer_state);
}

static void error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  syntax_error_va(fmt, ap);
  lexer_state->had_error = true;
  va_end(ap);
}

bool is_space(char c)
{
  return strchr(" \t\r", c);
}

void skip_spaces()
{
  while (!z_scanner_is_at_end(lexer_state->scanner) && is_space(z_scanner_peek(lexer_state->scanner))) {
    z_scanner_advance(&lexer_state->scanner);
  }

  z_scanner_reset_mark(&lexer_state->scanner);
}

static Token create_token(Token_Type type)
{
  Token token = {
      .lexeme = z_sv_to_cstr(z_scanner_capture(lexer_state->scanner)),
      .type = type,
      .line = lexer_state->scanner.line,
      .column = lexer_state->scanner.column,
  };

  return token;
}

static Token create_token_full(Z_String_View lexeme, Token_Type type, int line, int column)
{
  Token token = {
      .lexeme = z_sv_to_cstr(lexeme),
      .type = type,
      .line = line,
      .column = column,
  };

  return token;
}

static bool is_argument(char c)
{
  return !strchr(" &|;()\n\"'", c);
}

static void advance_command_substitution();
static void advance_double_quoted_string();
static void advance_untill(Z_Scanner *scanner, Z_String_View s);

static void advance_command_substitution()
{
  while (!z_scanner_is_at_end(lexer_state->scanner)) {
    switch (z_scanner_advance(&lexer_state->scanner)) {
      case '(':
        advance_command_substitution();
        break;

      case ')':
        return;

      case '\'':
        advance_untill(&lexer_state->scanner, Z_CSTR("'"));
        if (z_scanner_is_at_end(lexer_state->scanner))
          return;
        z_scanner_advance(&lexer_state->scanner);
        break;

      case '"': {
          advance_double_quoted_string(&lexer_state->scanner);
          if (z_scanner_is_at_end(lexer_state->scanner))
            return;
          z_scanner_advance(&lexer_state->scanner);
          break;
      }
    }
  }
}

static void advance_double_quoted_string()
{
  while (!z_scanner_is_at_end(lexer_state->scanner) && !z_scanner_check(lexer_state->scanner, '"')) {
    if (z_scanner_match_string(&lexer_state->scanner, Z_CSTR("$("))) {
      advance_command_substitution();
    } else {
      z_scanner_advance(&lexer_state->scanner);
    }
  }
}

static void advance_untill(Z_Scanner *scanner, Z_String_View s)
{
  while (!z_scanner_is_at_end(*scanner) && !z_scanner_check_string(*scanner, s)) {
    z_scanner_advance(scanner);
  }
}

Token multi_double_quoted_string()
{
  z_scanner_match(&lexer_state->scanner, '\n');
  z_scanner_reset_mark(&lexer_state->scanner);

  advance_untill(&lexer_state->scanner, Z_CSTR("\"\"\""));

  if (z_scanner_is_at_end(lexer_state->scanner)) {
    error("Unexpected end of file while looking for matching '\"\"\"'");
    return create_token(TOKEN_UNKOWN);
  }

  Z_String_View string = z_scanner_capture(lexer_state->scanner);

  if (z_sv_ends_with(string, Z_CSTR("\n"))) {
    string.len--;
  }

  Token token = create_token_full(string, TOKEN_DQUOTED_STRING, lexer_state->scanner.line, lexer_state->scanner.column);
  lexer_state->scanner.end += 3;

  return token;
}

Token double_quoted_string()
{
  z_scanner_reset_mark(&lexer_state->scanner);

  advance_double_quoted_string();

  if (z_scanner_is_at_end(lexer_state->scanner)) {
    error("Unexpected end of file while looking for matching \"");
    return create_token(TOKEN_UNKOWN);
  }

  Token token = create_token(TOKEN_DQUOTED_STRING);
  z_scanner_advance(&lexer_state->scanner);

  return token;
}

Token single_quoted_string()
{
  z_scanner_reset_mark(&lexer_state->scanner);
  advance_untill(&lexer_state->scanner, Z_CSTR("'"));

  if (z_scanner_is_at_end(lexer_state->scanner)) {
    error("Unexpected end of file while looking for matching '");
    return create_token(TOKEN_UNKOWN);
  }

  Token token = create_token(TOKEN_SQUOTED_STRING);
  z_scanner_advance(&lexer_state->scanner);

  return token;
}

Token argument()
{
  while (!z_scanner_is_at_end(lexer_state->scanner)) {
    if (z_scanner_previous(lexer_state->scanner) == '$' && z_scanner_match(&lexer_state->scanner, '(')) {
      advance_command_substitution();
    } else if (is_argument(z_scanner_peek(lexer_state->scanner))) {
      z_scanner_advance(&lexer_state->scanner);
    } else {
      break;
    }
  }

  Z_String_View arg = z_scanner_capture(lexer_state->scanner);

  return create_token(get_keyword_type(arg, TOKEN_WORD));
}

void skip_comment()
{
  while (!z_scanner_is_at_end(lexer_state->scanner) && !z_scanner_check(lexer_state->scanner, '\n')) {
    z_scanner_advance(&lexer_state->scanner);
  }
}

Token lexer_next()
{
  skip_spaces(&lexer_state->scanner);

  if (z_scanner_is_at_end(lexer_state->scanner)) {
    return create_token(TOKEN_EOD);
  }

  char c = z_scanner_advance(&lexer_state->scanner);

  switch (c) {
    case '|':
      return create_token(z_scanner_match(&lexer_state->scanner, '|') ? TOKEN_OR : TOKEN_PIPE);

    case '&':
      return create_token(z_scanner_match(&lexer_state->scanner, '&') ? TOKEN_AND : TOKEN_AMPERSAND);

    case '\'':
      return single_quoted_string(&lexer_state->scanner);

    case '"':
      return z_scanner_match_string(&lexer_state->scanner, Z_CSTR("\"\"")) ? multi_double_quoted_string() : double_quoted_string();

    case '#':
      skip_comment();
      return lexer_next();

    case ';':
      return create_token(TOKEN_STATEMENT_END);

    case '\n':
      return create_token(TOKEN_STATEMENT_END);

    default:
      return argument();
  }
}

Token_Array lexer_get_tokens(Z_String_View source)
{
  create_new_lexer_state(source);

  Token_Array tokens = {0};
  Token token = lexer_next();

  while (token.type != TOKEN_EOD) {
    z_da_append(&tokens, token);
    token = lexer_next();
  }

  z_da_append(&tokens, token);

  if (lexer_state->had_error) {
    tokens.len = 0;
    z_da_append(&tokens, create_token(TOKEN_EOD));
    free_lexer_state();
    return tokens;
  }

  free_lexer_state();
  return tokens;
}

void lexer_print_tokens(const Token_Array *tokens)
{
  for (int i = 0; i < tokens->len; i++) {
    print_token(tokens->ptr[i]);
  }
}
