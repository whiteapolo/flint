#include "expantion.h"
#include "builtins/builtin.h"
#include "eval.h"
#include "interpreter.h"
#include "libzatar.h"
#include "state.h"
#include "token.h"
#include <ctype.h>
#include <endian.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void resolve_var(const char *var, Z_String *output)
{
  z_str_append_format(output, "%s", select_variable(var));
}

static void command_substitution(Z_Scanner *scanner, Z_String *output);
static void scanner_advance_single_quoted_string(Z_Scanner *scanner);
static void scanner_advance_double_quoted_string(Z_Scanner *scanner);
static void scanner_advance_command_substitution(Z_Scanner *scanner);

static void scanner_advance_command_substitution(Z_Scanner *scanner)
{
  while (!z_scanner_is_at_end(*scanner)) {
    switch (z_scanner_advance(scanner)) {
      case '(':
        scanner_advance_command_substitution(scanner);
        break;

      case ')': return;

      case '\'': {
         scanner_advance_single_quoted_string(scanner);
         if (z_scanner_is_at_end(*scanner)) return;
         z_scanner_advance(scanner);
         break;
      }

      case '"': {
        scanner_advance_double_quoted_string(scanner);
        if (z_scanner_is_at_end(*scanner)) return;
        z_scanner_advance(scanner);
        break;
      }
    }
  }
}

static void scanner_advance_double_quoted_string(Z_Scanner *scanner)
{
  while (!z_scanner_is_at_end(*scanner) && !z_scanner_check(*scanner, '"')) {
    if (z_scanner_match_string(scanner, Z_CSTR("$("))) {
      scanner_advance_command_substitution(scanner);
    } else {
      z_scanner_advance(scanner);
    }
  }
}

static void scanner_advance_single_quoted_string(Z_Scanner *scanner)
{
  while (!z_scanner_is_at_end(*scanner) && !z_scanner_check(*scanner, '\'')) {
    z_scanner_advance(scanner);
  }
}

static void command_substitution(Z_Scanner *scanner, Z_String *output)
{
  z_scanner_reset_mark(scanner);
  scanner_advance_command_substitution(scanner);
  Z_String_View command = z_scanner_capture(*scanner);
  command.len--;
  interpret_to(command, output);
}

void braced_variable(Z_Scanner *scanner, Z_String *output)
{
  z_scanner_reset_mark(scanner);

  while (!z_scanner_is_at_end(*scanner) && z_scanner_peek(*scanner) != '}') {
    z_scanner_advance(scanner);
  }

  if (z_scanner_is_at_end(*scanner)) {
    z_str_append_str(output, z_scanner_capture(*scanner));
    return;
  }

  char *var_name = z_sv_to_cstr(z_scanner_capture(*scanner));
  z_str_append_format(output, "%s", select_variable(var_name));
  free(var_name);

  z_scanner_advance(scanner); // eat the '}'
}

bool is_variable_char(char c)
{
  return isdigit(c) || isalpha(c) || strchr("_?@", c);
}

void variable(Z_Scanner *scanner, Z_String *output)
{
  z_scanner_reset_mark(scanner);

  while (!z_scanner_is_at_end(*scanner) && is_variable_char(z_scanner_peek(*scanner))) {
    z_scanner_advance(scanner);
  }

  char *var_name = z_sv_to_cstr(z_scanner_capture(*scanner));
  z_str_append_format(output, "%s", select_variable(var_name));
  free(var_name);
}

char escaped_char(char c)
{
  switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    default: return c;
  }
}

void escape_sequence(Z_Scanner *scanner, Z_String *output)
{
  char c = z_scanner_advance(scanner);

  if (c == '\\' && !z_scanner_is_at_end(*scanner)) {
    z_str_append_char(output, escaped_char(z_scanner_advance(scanner)));
  } else {
    z_str_append_char(output, c);
  }
}

void expand_dqouted_string(Token token, String_Vec *output)
{
  Z_Scanner scanner = z_scanner_new(Z_CSTR(token.lexeme));
  Z_String exapnded = {0};

  if (z_scanner_match(&scanner, '~')) {
    z_str_append_str(&exapnded, z_get_home_path());
  }

  while (!z_scanner_is_at_end(scanner)) {
    if (z_scanner_match(&scanner, '$')) {
      if (z_scanner_match(&scanner, '(')) {
        command_substitution(&scanner, &exapnded);
      } else if (z_scanner_match(&scanner, '{')) {
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

void expand_word(Token token, String_Vec *output)
{
  String_Vec tmp = {0};
  expand_dqouted_string(token, &tmp);

  z_sv_split_cset_foreach(Z_CSTR(tmp.ptr[0]), Z_CSTR(" \n"), word) {
    z_da_append(output, strndup(word.ptr, word.len));
  }

  free(tmp.ptr[0]);
  free(tmp.ptr);
}

void expand_sqouted_string(Token token, String_Vec *output)
{
  Z_Scanner scanner = z_scanner_new(Z_CSTR(token.lexeme));
  Z_String expanded = {0};

  while (!z_scanner_is_at_end(scanner)) {
    escape_sequence(&scanner, &expanded);
  }

  z_da_append(output, (char *)z_str_to_cstr(&expanded));
}

void expand_token(Token token, String_Vec *out)
{
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

char **expand_argv(Argv argv)
{
  String_Vec expanded = {0};

  for (int i = 0; i < argv.len; i++) {
    Token arg = argv.ptr[i];
    expand_token(arg, &expanded);
  }

  z_da_null_terminate(&expanded);

  return expanded.ptr;
}

void expand_alias(Token key, Token_Vec *output)
{
  const char *value = select_alias(key.lexeme);

  if (!value) {
    z_da_append(output, key);
  } else {
    Token_Vec tmp = lexer_get_tokens(Z_CSTR(value));
    z_da_append_da(output, &tmp);
    output->len--; // remove EOF token
    free(tmp.ptr);
  }
}

static bool is_string(Token_Type type)
{
  return type == TOKEN_WORD || type == TOKEN_DQUOTED_STRING || type == TOKEN_SQUOTED_STRING;
}

void expand_aliases(Token_Vec *tokens)
{
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
