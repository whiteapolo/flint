#include "expantion.h"
#include "eval.h"
#include "environment.h"
#include "interpreter.h"
#include "libzatar.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **ptr;
    int len;
    int capacity;
} String_Vec;

typedef struct {
    const char *end;
    const char *start;
    const char *curr;
} Scanner;

static bool is_at_end(Scanner *scanner)
{
    return scanner->curr >= scanner->end;
}

static char advance(Scanner *scanner)
{
    return *(scanner->curr++);
}

static char peek(Scanner *scanner)
{
    return *scanner->curr;
}

static bool match(Scanner *scanner, char expected)
{
    if (is_at_end(scanner)) {
        return false;
    }

    if (peek(scanner) == expected) {
        advance(scanner);
        return true;
    }

    return false;
}

static Scanner scanner_new(Z_String_View s)
{
    Scanner scanner = {
        .curr = s.ptr,
        .start = s.ptr,
        .end = s.ptr + s.len,
    };

    return scanner;
}

bool is_alpha(char c)
{
    return ('0' <= c && c <= '9')      ||
                ('a' <= c && c <= 'z') ||
                ('A' <= c && c <= 'Z') ||
                (c == '_');
}

void resolve_var(Z_String_View var, Z_String *output)
{
    extern Environment environment;
    z_str_append_str(output, environment_get(&environment, var));
}

void command_substitution(Scanner *scanner, Z_String *output)
{
    const char *start = scanner->curr;
    int len = 0;

    while (!is_at_end(scanner) && advance(scanner) != ')') {
        len++;
    }

    interpret_to(Z_SV(start, len), output);
}

void braced_variable(Scanner *scanner, Z_String *output)
{
    const char *start = scanner->curr;
    int len = 0;

    while (!is_at_end(scanner) && advance(scanner) != '}') {
        len++;
    }

    extern Environment environment;
    z_str_append_str(output, environment_get(&environment, Z_SV(start, len)));
}

bool is_variable_char(char c)
{
    return is_alpha(c) || c == '?';
}

void variable(Scanner *scanner, Z_String *output)
{
    const char *start = scanner->curr;
    int len = 0;

    while (!is_at_end(scanner) && is_variable_char(peek(scanner))) {
        advance(scanner);
        len++;
    }

    extern Environment environment;
    z_str_append_str(output, environment_get(&environment, Z_SV(start, len)));
}

void expand_dqouted_string(Token token, String_Vec *output)
{
    Scanner scanner = scanner_new(token.lexeme);

    Z_String exapnded = {0};

    if (match(&scanner, '~')) {
        z_str_append_str(&exapnded, z_get_home_path());
    }

    while (!is_at_end(&scanner)) {
        if (match(&scanner, '$')) {
            if (match(&scanner, '(')) {
                command_substitution(&scanner, &exapnded);
            } else if (match(&scanner, '{')) {
                braced_variable(&scanner, &exapnded);
            } else {
                variable(&scanner, &exapnded);
            }
        } else {
            z_str_append_char(&exapnded, advance(&scanner));
        }
    }

    z_da_append(output, (char *)z_str_to_cstr(&exapnded));
}

void expand_word(Token token, String_Vec *output)
{
    String_Vec tmp = {0};
    expand_dqouted_string(token, &tmp);

    Z_String_View delim = Z_CSTR_TO_SV(" ");
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
