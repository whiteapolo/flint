#include "error.h"
#include "cstr.h"
#include "libzatar.h"
#include "token.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void syntax_error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  syntax_error_va(fmt, ap);
  va_end(ap);
}

void syntax_error_va(const char *fmt, va_list ap)
{
  fprintf(stderr, "%sYOU SUCK%s:", Z_COLOR_RED, Z_COLOR_RESET);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void syntax_error_at_token(const char * const *source, Token token, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  syntax_error_at_token_va(source, token, fmt, ap);
  va_end(ap);
}

void print_str_without_tabs(FILE *out, const char *s)
{
  char *no_tabs = str_replace(s, "\t", " ");
  fprintf(out, "%s", no_tabs);
  free(no_tabs);
}

void syntax_error_at_token_va(const char * const *source, Token token, const char *fmt, va_list ap)
{
  fprintf(stderr, "%d:%d: %sYOU SUCK%s: \n", token.line, token.column, Z_COLOR_RED, Z_COLOR_RESET);

  if (token.type == TOKEN_EOD || token.type == TOKEN_STATEMENT_END) {
    fprintf(stderr, "at end: ");
  } else {
    fprintf(stderr, "at '%s': ", token.lexeme);
  }

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  fprintf(stderr, "%5d | ", token.line);
  print_str_without_tabs(stderr, source[token.line - 1]);
  fprintf(stderr, "\n      | %*s%s^%s\n", token.column, "", Z_COLOR_RED, Z_COLOR_RESET);
}