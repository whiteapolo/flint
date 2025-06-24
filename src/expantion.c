#include "expantion.h"
#include "eval.h"
#include "environment.h"
#include "interpreter.h"
#include "libzatar.h"
#include "scanner.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **ptr;
    int len;
    int capacity;
} String_Vec;

void resolve_var(Z_String_View var, Z_String *output)
{
    extern Environment environment;
    z_str_append_str(output, environment_get(&environment, var));
}

static void command_substitution(Scanner *scanner, Z_String *output);
static void scanner_advance_single_quoted_string(Scanner *scanner);
static void scanner_advance_double_quoted_string(Scanner *scanner);
static void scanner_advance_command_substitution(Scanner *scanner);

static void scanner_advance_command_substitution(Scanner *scanner)
{
    while (!scanner_is_at_end(scanner)) {
        switch (scanner_advance(scanner)) {
            case '(':
                scanner_advance_command_substitution(scanner);
                break;

            case ')':
                return;

            case '\'':
                scanner_advance_single_quoted_string(scanner);
                if (scanner_is_at_end(scanner)) return;
                scanner_advance(scanner);
                break;

            case '"': {
                scanner_advance_double_quoted_string(scanner);
                if (scanner_is_at_end(scanner)) return;
                scanner_advance(scanner);
                break;
            }
        }
    }
}

static void scanner_advance_double_quoted_string(Scanner *scanner)
{
    while (!scanner_is_at_end(scanner) && !scanner_check(scanner, '"')) {
        if (scanner_match_string(scanner, "$(")) {
            scanner_advance_command_substitution(scanner);
        } else {
            scanner_advance(scanner);
        }
    }
}

static void scanner_advance_single_quoted_string(Scanner *scanner)
{
    scanner_advance_untill(scanner, '\'');
}

static void command_substitution(Scanner *scanner, Z_String *output)
{
    const char *start = scanner->curr;
    scanner_advance_command_substitution(scanner);

    interpret_to(Z_SV(start, scanner->curr - start - 1), output);
}

void braced_variable(Scanner *scanner, Z_String *output)
{
    scanner->start = scanner->curr;

    while (!scanner_is_at_end(scanner) && scanner_peek(scanner) != '}') {
        scanner_advance(scanner);
    }

    if (scanner_is_at_end(scanner)) {
        z_str_append_str(output, Z_SV(scanner->start, scanner->curr - scanner->start));
        return;
    }

    Z_String_View variable_name = Z_SV(scanner->start, scanner->curr - scanner->start);

    extern Environment environment;
    z_str_append_str(output, environment_get(&environment, variable_name));
    scanner_advance(scanner); // eat the '}'
}

bool is_variable_char(char c)
{
    return isdigit(c) || isalpha(c) || strchr("_?", c);
}

void variable(Scanner *scanner, Z_String *output)
{
    scanner->start = scanner->curr;

    while (!scanner_is_at_end(scanner) && is_variable_char(scanner_peek(scanner))) {
        scanner_advance(scanner);
    }

    extern Environment environment;
    z_str_append_str(output, environment_get(&environment, Z_SV(scanner->start, scanner->curr - scanner->start)));
}

void expand_dqouted_string(Token token, String_Vec *output)
{
    Scanner scanner = scanner_new(token.lexeme);

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
            z_str_append_char(&exapnded, scanner_advance(&scanner));
        }
    }

    z_da_append(output, (char *)z_str_to_cstr(&exapnded));
}

void expand_word(Token token, String_Vec *output)
{
    String_Vec tmp = {0};
    expand_dqouted_string(token, &tmp);

    Z_String_View delim = Z_CSTR_TO_SV(" \n");
    Z_String_View word = z_str_tok_start(Z_CSTR_TO_SV(tmp.ptr[0]), delim);

    while (word.len > 0) {
        z_da_append(output, strndup(word.ptr, word.len));
        word = z_str_tok_next(Z_CSTR_TO_SV(tmp.ptr[0]), word, delim);
    }

    free(tmp.ptr[0]);
    free(tmp.ptr);
}

char **expand_argv(Argv argv)
{
    String_Vec expanded = {0};

    for (int i = 0; i < argv.len; i++) {
        Token arg = argv.ptr[i];

        switch (arg.type) {
            case TOKEN_WORD:
                expand_word(arg, &expanded);
                break;

            case TOKEN_DQUOTED_STRING:
                expand_dqouted_string(arg, &expanded);
                break;

            case TOKEN_SQUOTED_STRING:
            default:
                z_da_append(&expanded, strndup(arg.lexeme.ptr, arg.lexeme.len));
                break;
        }
    }

    z_da_null_terminate(&expanded);

    return expanded.ptr;
}
