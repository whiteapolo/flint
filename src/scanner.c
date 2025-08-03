#include "scanner.h"
#include "libzatar.h"
#include <ctype.h>
#include <string.h>

Scanner scanner_new(Z_String_View s) {
  Scanner scanner = {
      .source = s.ptr,
      .curr = s.ptr,
      .start = s.ptr,
      .end = s.ptr + s.len,
      .line = 0,
      .column = 0,
  };

  return scanner;
}

bool scanner_is_at_end(const Scanner *scanner) {
  return scanner->curr >= scanner->end;
}

char scanner_advance(Scanner *scanner) {
  scanner->column++;

  if (scanner_check(scanner, '\n')) {
    scanner->line++;
    scanner->column = 0;
  }

  return *(scanner->curr++);
}

char scanner_peek(const Scanner *scanner) { return *scanner->curr; }

char scanner_previous(const Scanner *scanner) { return scanner->curr[-1]; }

bool scanner_check(const Scanner *scanner, char c) {
  return scanner_peek(scanner) == c;
}

bool scanner_match(Scanner *scanner, char expected) {
  if (scanner_is_at_end(scanner)) {
    return false;
  }

  if (scanner_peek(scanner) == expected) {
    scanner_advance(scanner);
    return true;
  }

  return false;
}

bool scanner_check_string(const Scanner *scanner, const char *s) {
  int len = strlen(s);

  if (scanner->curr + len > scanner->end) {
    return false;
  }

  for (int i = 0; i < len; i++) {
    if (scanner->curr[i] != s[i]) {
      return false;
    }
  }

  return true;
}

void scanner_advance_untill(Scanner *scanner, char c) {
  while (!scanner_is_at_end(scanner) && !scanner_check(scanner, c)) {
    scanner_advance(scanner);
  }
}

void scanner_advance_untill_string(Scanner *scanner, const char *s) {
  while (!scanner_is_at_end(scanner) && !scanner_check_string(scanner, s)) {
    scanner_advance(scanner);
  }
}

bool scanner_match_string(Scanner *scanner, const char *s) {
  if (scanner_check_string(scanner, s)) {
    scanner->curr += strlen(s);
    return true;
  }

  return false;
}
