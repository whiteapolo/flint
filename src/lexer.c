#include "lexer.h"
#include "builtins/builtin.h"
#include "cstr.h"
#include "error.h"
#include "libzatar.h"
#include "scanner.h"
#include "token.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool had_error;
static Z_Scanner scanner;

static void error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  syntax_error_va(fmt, ap);
  had_error = true;
  va_end(ap);
}

bool is_space(char c)
{
  return strchr(" \t\r", c);
}

void skip_spaces()
{
  while (!z_scanner_is_at_end(scanner) && is_space(z_scanner_peek(scanner))) {
    z_scanner_advance(&scanner);
  }

  z_scanner_reset_mark(&scanner);
}

static Token create_token(Token_Type type)
{
  Token token = {
      .lexeme = z_sv_to_cstr(z_scanner_capture(scanner)),
      .type = type,
      .line = scanner.line,
      .column = scanner.column,
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
  while (!z_scanner_is_at_end(scanner)) {
    switch (z_scanner_advance(&scanner)) {
      case '(':
        advance_command_substitution();
        break;

      case ')':
        return;

      case '\'':
        advance_untill(&scanner, Z_CSTR("'"));
        if (z_scanner_is_at_end(scanner))
          return;
        z_scanner_advance(&scanner);
        break;

      case '"': {
          advance_double_quoted_string(&scanner);
          if (z_scanner_is_at_end(scanner))
            return;
          z_scanner_advance(&scanner);
          break;
      }
    }
  }
}

static void advance_double_quoted_string()
{
  while (!z_scanner_is_at_end(scanner) && !z_scanner_check(scanner, '"')) {
    if (z_scanner_match_string(&scanner, Z_CSTR("$("))) {
      advance_command_substitution();
    } else {
      z_scanner_advance(&scanner);
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
  z_scanner_match(&scanner, '\n');
  z_scanner_reset_mark(&scanner);

  advance_untill(&scanner, Z_CSTR("\"\"\""));

  if (z_scanner_is_at_end(scanner)) {
    error("Unexpected end of file while looking for matching '\"\"\"'");
    return create_token(TOKEN_UNKOWN);
  }

  Z_String_View string = z_scanner_capture(scanner);

  if (z_sv_ends_with(string, Z_CSTR("\n"))) {
    string.len--;
  }

  Token token = create_token_full(string, TOKEN_DQUOTED_STRING, scanner.line, scanner.column);
  scanner.end += 3;

  return token;
}

Token double_quoted_string()
{
  z_scanner_reset_mark(&scanner);

  advance_double_quoted_string();

  if (z_scanner_is_at_end(scanner)) {
    error("Unexpected end of file while looking for matching \"");
    return create_token(TOKEN_UNKOWN);
  }

  Token token = create_token(TOKEN_DQUOTED_STRING);
  z_scanner_advance(&scanner);

  return token;
}

Token single_quoted_string()
{
  z_scanner_reset_mark(&scanner);
  advance_untill(&scanner, Z_CSTR("'"));

  if (z_scanner_is_at_end(scanner)) {
    error("Unexpected end of file while looking for matching '");
    return create_token(TOKEN_UNKOWN);
  }

  Token token = create_token(TOKEN_SQUOTED_STRING);
  z_scanner_advance(&scanner);

  return token;
}

Token argument()
{
  while (!z_scanner_is_at_end(scanner)) {
    if (z_scanner_previous(scanner) == '$' && z_scanner_match(&scanner, '(')) {
      advance_command_substitution();
    } else if (is_argument(z_scanner_peek(scanner))) {
      z_scanner_advance(&scanner);
    } else {
      break;
    }
  }

  Z_String_View arg = z_scanner_capture(scanner);

  Optional_Token_Type keyword_type = get_keyword_type(arg);

  return create_token(keyword_type.ok ? keyword_type.value : TOKEN_WORD);
}

void skip_comment()
{
  while (!z_scanner_is_at_end(scanner) && !z_scanner_check(scanner, '\n')) {
    z_scanner_advance(&scanner);
  }
}

Token lexer_next()
{
  skip_spaces(&scanner);

  if (z_scanner_is_at_end(scanner)) {
    return create_token(TOKEN_EOD);
  }

  char c = z_scanner_advance(&scanner);

  switch (c) {
  case '|':
    return create_token(z_scanner_match(&scanner, '|') ? TOKEN_OR : TOKEN_PIPE);

  case '&':
    return create_token(z_scanner_match(&scanner, '&') ? TOKEN_AND : TOKEN_AMPERSAND);

  case '\'':
    return single_quoted_string(&scanner);

  case '"':
    return z_scanner_match_string(&scanner, Z_CSTR("\"\"")) ? multi_double_quoted_string() : double_quoted_string();

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

Token_Vec lexer_get_tokens(Z_String_View source)
{
  scanner = z_scanner_new(source);
  had_error = false;

  Token_Vec tokens = {0};
  Token token = lexer_next(&scanner);

  while (token.type != TOKEN_EOD) {
    z_da_append(&tokens, token);
    token = lexer_next(&scanner);
  }

  z_da_append(&tokens, token);

  if (had_error) {
    tokens.len = 0;
    z_da_append(&tokens, create_token(TOKEN_EOD));
    return tokens;
  }

  return tokens;
}

void lexer_print_tokens(const Token_Vec *tokens)
{
  for (int i = 0; i < tokens->len; i++) {
    print_token(tokens->ptr[i]);
  }
}
