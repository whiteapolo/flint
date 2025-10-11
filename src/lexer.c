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
static Scanner scanner;

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
  while (!scanner_is_at_end(&scanner) && is_space(scanner_peek(&scanner))) {
    scanner_advance(&scanner);
  }

  scanner.start = scanner.curr;
}

static Token create_token(Token_Type type)
{
  Token token = {
      .lexeme = strndup(scanner.start, scanner.curr - scanner.start),
      .type = type,
      .line = scanner.line,
      .column = scanner.column,
  };

  return token;
}

static bool is_argument(char c)
{
  return !strchr(" &|;()\n\"'", c);
}

static void advance_command_substitution();
static void advance_double_quoted_string();
static void advance_single_quoted_string();

static void advance_command_substitution()
{
  while (!scanner_is_at_end(&scanner)) {
    switch (scanner_advance(&scanner)) {
    case '(':
      advance_command_substitution();
      break;

    case ')':
      return;

    case '\'':
      advance_single_quoted_string();
      if (scanner_is_at_end(&scanner))
        return;
      scanner_advance(&scanner);
      break;

    case '"': {
      advance_double_quoted_string(&scanner);
      if (scanner_is_at_end(&scanner))
        return;
      scanner_advance(&scanner);
      break;
    }
    }
  }
}

static void advance_double_quoted_string()
{
  while (!scanner_is_at_end(&scanner) && !scanner_check(&scanner, '"')) {
    if (scanner_match_string(&scanner, "$(")) {
      advance_command_substitution();
    } else {
      scanner_advance(&scanner);
    }
  }
}

static void advance_single_quoted_string()
{
  scanner_advance_untill(&scanner, '\'');
}

Token multi_double_quoted_string()
{
  scanner_match(&scanner, '\n');
  scanner.start = scanner.curr;

  scanner_advance_untill_string(&scanner, "\"\"\"");

  if (scanner_is_at_end(&scanner)) {
    error("Unexpected end of file while looking for matching '\"\"\"'");
    return create_token(TOKEN_UNKOWN);
  }

  bool is_line_end = (scanner_previous(&scanner) == '\n');

  if (is_line_end) {
    scanner.curr--;
  }

  Token token = create_token(TOKEN_DQUOTED_STRING);
  scanner.curr += 3 + is_line_end;

  return token;
}

Token double_quoted_string()
{
  scanner.start = scanner.curr;

  advance_double_quoted_string();

  if (scanner_is_at_end(&scanner)) {
    error("Unexpected end of file while looking for matching \"");
    return create_token(TOKEN_UNKOWN);
  }

  Token token = create_token(TOKEN_DQUOTED_STRING);
  scanner_advance(&scanner);

  return token;
}

Token single_quoted_string()
{
  scanner.start = scanner.curr;

  advance_single_quoted_string(&scanner);

  if (scanner_is_at_end(&scanner)) {
    error("Unexpected end of file while looking for matching '");
    return create_token(TOKEN_UNKOWN);
  }

  Token token = create_token(TOKEN_SQUOTED_STRING);
  scanner_advance(&scanner);

  return token;
}

Token argument()
{
  while (!scanner_is_at_end(&scanner)) {
    if (scanner_previous(&scanner) == '$' && scanner_match(&scanner, '(')) {
      advance_command_substitution();
    } else if (is_argument(scanner_peek(&scanner))) {
      scanner_advance(&scanner);
    } else {
      break;
    }
  }

  Z_String_View arg = Z_SV(scanner.start, scanner.curr - scanner.start);

  Optional_Token_Type keyword_type = get_keyword_type(arg);

  return create_token(keyword_type.ok ? keyword_type.value : TOKEN_WORD);
}

void skip_comment()
{
  while (!scanner_is_at_end(&scanner) && !scanner_check(&scanner, '\n')) {
    scanner_advance(&scanner);
  }
}

Token lexer_next()
{
  skip_spaces(&scanner);

  if (scanner_is_at_end(&scanner)) {
    return create_token(TOKEN_EOD);
  }

  char c = scanner_advance(&scanner);

  switch (c) {
  case '|':
    return create_token(scanner_match(&scanner, '|') ? TOKEN_OR : TOKEN_PIPE);

  case '&':
    return create_token(scanner_match(&scanner, '&') ? TOKEN_AND : TOKEN_AMPERSAND);

  case '\'':
    return single_quoted_string(&scanner);

  case '"':
    return scanner_match_string(&scanner, "\"\"") ? multi_double_quoted_string() : double_quoted_string();

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
  scanner = scanner_new(source);
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
