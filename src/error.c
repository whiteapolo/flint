#include "error.h"
#include "libzatar.h"
#include "token.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void syntax_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  syntax_error_va(fmt, ap);
  va_end(ap);
}

void syntax_error_va(const char *fmt, va_list ap) {
  fprintf(stderr, "" Z_COLOR_RED "YOU SUCK. here is why" Z_COLOR_RESET ": ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void syntax_error_at_token(Z_String_View source, Token token, const char *fmt,
                           ...) {
  va_list ap;
  va_start(ap, fmt);
  syntax_error_at_token_va(source, token, fmt, ap);
  va_end(ap);
}

Z_String_View get_token_line(Z_String_View source, Token token) {
  const char *start = token.lexeme.ptr - 1;
  const char *end = token.lexeme.ptr;

  while (*start != '\n' && start > source.ptr) {
    start--;
  }

  if (*start == '\n') {
    start++;
  }

  while (end < source.ptr + source.len && *end != '\n') {
    end++;
  }

  return Z_SV(start, end - start);
}

void print_str_without_tabs(Z_String_View s) {
  for (int i = 0; i < s.len; i++) {
    fprintf(stderr, "%c", s.ptr[i] == '\t' ? ' ' : s.ptr[i]);
  }
}

void syntax_error_at_token_va(Z_String_View source, Token token,
                              const char *fmt, va_list ap) {
  fprintf(stderr,
          "%d:%d: " Z_COLOR_RED "YOU SUCK. here is why" Z_COLOR_RESET ": ",
          token.line, token.column);

  if (token.type == TOKEN_EOD || token.type == TOKEN_STATEMENT_END) {
    fprintf(stderr, "at end: ");
  } else {
    fprintf(stderr, "at '%.*s': ", token.lexeme.len, token.lexeme.ptr);
  }

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  Z_String_View line = get_token_line(source, token);

  fprintf(stderr, "%5d | ", token.line);
  print_str_without_tabs(line);
  fprintf(stderr, "\n      | %*s" Z_COLOR_RED "^" Z_COLOR_RESET "\n",
          (int)(token.lexeme.ptr - line.ptr), "");
}

// const char *get_random_assult()
// {
//     srand(time(NULL));
//     const char *assults[] = {
//         "You suck",
//         "Stupid",
//         "Dumb you",
//         "You're like a cloud... when you disappear, it's a beutiful day."
//     }

// }
