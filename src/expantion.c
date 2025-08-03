#include "expantion.h"
#include "builtins/builtin.h"
#include "environment.h"
#include "eval.h"
#include "interpreter.h"
#include "libzatar.h"
#include "scanner.h"
#include "state.h"
#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void resolve_var(Z_String_View var, Z_String *output) {
  // z_str_append_str(output, select_variable(var));
  z_str_append_format(output, "%s", select_variable(var));
}

static void command_substitution(Scanner *scanner, Z_String *output);
static void scanner_advance_single_quoted_string(Scanner *scanner);
static void scanner_advance_double_quoted_string(Scanner *scanner);
static void scanner_advance_command_substitution(Scanner *scanner);

static void scanner_advance_command_substitution(Scanner *scanner) {
  while (!scanner_is_at_end(scanner)) {
    switch (scanner_advance(scanner)) {
    case '(':
      scanner_advance_command_substitution(scanner);
      break;

    case ')':
      return;

    case '\'':
      scanner_advance_single_quoted_string(scanner);
      if (scanner_is_at_end(scanner))
        return;
      scanner_advance(scanner);
      break;

    case '"': {
      scanner_advance_double_quoted_string(scanner);
      if (scanner_is_at_end(scanner))
        return;
      scanner_advance(scanner);
      break;
    }
    }
  }
}

static void scanner_advance_double_quoted_string(Scanner *scanner) {
  while (!scanner_is_at_end(scanner) && !scanner_check(scanner, '"')) {
    if (scanner_match_string(scanner, "$(")) {
      scanner_advance_command_substitution(scanner);
    } else {
      scanner_advance(scanner);
    }
  }
}

static void scanner_advance_single_quoted_string(Scanner *scanner) {
  scanner_advance_untill(scanner, '\'');
}

static void command_substitution(Scanner *scanner, Z_String *output) {
  const char *start = scanner->curr;
  scanner_advance_command_substitution(scanner);

  interpret_to(Z_SV(start, scanner->curr - start - 1), output);
}

void braced_variable(Scanner *scanner, Z_String *output) {
  scanner->start = scanner->curr;

  while (!scanner_is_at_end(scanner) && scanner_peek(scanner) != '}') {
    scanner_advance(scanner);
  }

  if (scanner_is_at_end(scanner)) {
    z_str_append_str(output,
                     Z_SV(scanner->start, scanner->curr - scanner->start));
    return;
  }

  Z_String_View variable_name =
      Z_SV(scanner->start, scanner->curr - scanner->start);

  z_str_append_format(output, "%s", select_variable(variable_name));
  scanner_advance(scanner); // eat the '}'
}

bool is_variable_char(char c) {
  return isdigit(c) || isalpha(c) || strchr("_?@", c);
}

void variable(Scanner *scanner, Z_String *output) {
  scanner->start = scanner->curr;

  while (!scanner_is_at_end(scanner) &&
         is_variable_char(scanner_peek(scanner))) {
    scanner_advance(scanner);
  }

  z_str_append_format(
      output, "%s",
      select_variable(Z_SV(scanner->start, scanner->curr - scanner->start)));
}

char escaped_char(char c) {
  switch (c) {
  case 'n':
    return '\n';
  case 't':
    return '\t';
  case 'r':
    return '\r';
  default:
    return c;
  }
}

void escape_sequence(Scanner *scanner, Z_String *output) {
  char c = scanner_advance(scanner);

  if (c == '\\' && !scanner_is_at_end(scanner)) {
    z_str_append_char(output, escaped_char(scanner_advance(scanner)));
  } else {
    z_str_append_char(output, c);
  }
}

void expand_dqouted_string(Token token, String_Vec *output) {
  Scanner scanner = scanner_new(Z_STR_TO_SV(token.lexeme));

  Z_String exapnded = {0};

  if (scanner_match(&scanner, '~')) {
    z_str_append_str(&exapnded, z_get_home_path());
  }

  while (!scanner_is_at_end(&scanner)) {
    if (scanner_match(&scanner, '$')) {
      if (scanner_match(&scanner, '(')) {
        command_substitution(&scanner, &exapnded);
      } else if (scanner_match(&scanner, '{')) {
        braced_variable(&scanner, &exapnded);
      } else {
        variable(&scanner, &exapnded);
      }
    } else {
      escape_sequence(&scanner, &exapnded);
    }
  }

  z_da_append(output, (char *)z_str_to_cstr(&exapnded));
}

void expand_word(Token token, String_Vec *output) {
  String_Vec tmp = {0};
  expand_dqouted_string(token, &tmp);

  z_str_tok_foreach(Z_CSTR_TO_SV(tmp.ptr[0]), Z_CSTR_TO_SV(" \n"), word) {
    z_da_append(output, strndup(word.ptr, word.len));
  }

  free(tmp.ptr[0]);
  free(tmp.ptr);
}

void expand_sqouted_string(Token token, String_Vec *output) {
  Scanner scanner = scanner_new(Z_STR_TO_SV(token.lexeme));
  Z_String expanded = {0};

  while (!scanner_is_at_end(&scanner)) {
    escape_sequence(&scanner, &expanded);
  }

  z_da_append(output, (char *)z_str_to_cstr(&expanded));
}

void expand_token(Token token, String_Vec *out) {
  switch (token.type) {
  case TOKEN_WORD:
    expand_word(token, out);
    break;

  case TOKEN_DQUOTED_STRING:
    expand_dqouted_string(token, out);
    break;

  case TOKEN_SQUOTED_STRING:
  default:
    expand_sqouted_string(token, out);
    break;
  }
}

char **expand_argv(Argv argv) {
  String_Vec expanded = {0};

  for (int i = 0; i < argv.len; i++) {
    Token arg = argv.ptr[i];
    expand_token(arg, &expanded);
  }

  z_da_null_terminate(&expanded);

  return expanded.ptr;
}

void expand_alias(Token key, Token_Vec *output) {
  const char *value = select_alias(Z_STR_TO_SV(key.lexeme));

  if (!value) {
    z_da_append(output, key);
  } else {
    Token_Vec tmp = lexer_get_tokens(Z_CSTR_TO_SV(value));
    z_da_append_da(output, &tmp);
    output->len--; // remove EOF token
    free(tmp.ptr);
  }
}

static bool is_string(Token_Type type) {
  return type == TOKEN_WORD || type == TOKEN_DQUOTED_STRING ||
         type == TOKEN_SQUOTED_STRING;
}

void expand_aliases(Token_Vec *tokens) {
  Token_Vec tmp = {0};
  bool is_command_start = true;

  for (int i = 0; i < tokens->len; i++) {
    Token token = tokens->ptr[i];

    if (!is_string(token.type) && token.type != TOKEN_FUN) {
      is_command_start = true;
      z_da_append(&tmp, token);
    } else if (!is_command_start) {
      z_da_append(&tmp, token);
    } else {
      expand_alias(token, &tmp);
      is_command_start = false;
    }
  }

  tokens->len = 0;
  z_da_append_da(tokens, &tmp);
  free(tmp.ptr);
}
