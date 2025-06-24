#include "lexer.h"
#include "error.h"
#include "token.h"
#include "builtins/builtin.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libzatar.h"

static const char *end;
static const char *start;
static const char *curr;
static bool had_error;
static int line;

typedef struct {
    Token_Type type;
    const char *lexeme;
} Keyword;

static Keyword keywords[] = {
    { .type = TOKEN_FOR, .lexeme = "for" },
    { .type = TOKEN_IF, .lexeme = "if" },
    { .type = TOKEN_END, .lexeme = "end" },
    { .type = TOKEN_IN, .lexeme = "in" },
    { .type = TOKEN_FUN, .lexeme = "fun" },
    { .type = TOKEN_ELSE, .lexeme = "else" },
    { .type = TOKEN_WHILE, .lexeme = "while" },
};

static void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    syntax_error_va(fmt, ap);
    had_error = true;
    va_end(ap);
}

static Token create_token(Token_Type type)
{
    Token token = {
        .lexeme = Z_SV(start, curr - start),
        .type = type,
        .line = line,
    };

    return token;
}

static bool is_at_end()
{
    return curr >= end;
}

static char peek()
{
    return *curr;
}

static bool check(char c)
{
    return peek() == c;
}

static char advance()
{
    if (check('\n')) {
        line++;
    }

    return *(curr++);
}

static char previous()
{
    return curr[-1];
}

static bool match(char expected)
{
    if (is_at_end()) {
        return false;
    }

    if (check(expected)) {
        advance();
        return true;
    }

    return false;
}

static bool check_string(const char *s)
{
    int len = strlen(s);

    if (curr + len > end) {
        return false;
    }

    for (int i = 0; i < len; i++) {
        if (curr[i] != s[i]) {
            return false;
        }
    }

    return true;
}

static bool match_string(const char *s)
{
    if (check_string(s)) {
        curr += strlen(s);
        return true;
    }

    return false;
}

static bool is_argument(char c)
{
    return !strchr(" &|;()\n\"'", c);
}

void advance_utill(char c)
{
    while (!is_at_end() && !check(c)) {
        advance();
    }
}

void advance_utill_string(const char *s)
{
    while (!is_at_end() && !check_string(s)) {
        advance();
    }
}

void advance_command_substitution()
{
    int nesting = 1;

    while (!is_at_end() && nesting) {
        char c = advance();

        switch (c) {
            case '(':
                nesting++;
                break;

            case ')':
                nesting--;
                break;

            case '\'':
                if (!is_at_end()) advance();
                advance_utill('\'');
                break;

            case '"':
                advance_utill('"');
                if (!is_at_end()) advance();
                break;
        }
    }
}

Token multi_double_quoted_string()
{
    match('\n');
    start = curr;

    advance_utill_string("\"\"\"");

    if (is_at_end()) {
        error("Unexpected end of file while looking for matching '\"\"\"'");
        return create_token(TOKEN_UNKOWN);
    }

    bool is_line_end = (previous() == '\n');

    if (is_line_end) {
        curr--;
    }

    Token token = create_token(TOKEN_DQUOTED_STRING);
    curr += 3 + is_line_end;

    return token;
}

Token double_quoted_string()
{
    start = curr;

    while (!is_at_end() && !check('"')) {
        if (match_string("$(")) {
            advance_command_substitution();
        } else {
            advance();
        }
    }

    if (is_at_end()) {
        error("Unexpected end of file while looking for matching \"");
        return create_token(TOKEN_UNKOWN);
    }

    Token token = create_token(TOKEN_DQUOTED_STRING);
    advance();

    return token;
}

Token single_quoted_string()
{
    start = curr;

    advance_utill('\'');

    if (is_at_end()) {
        error("Unexpected end of file while looking for matching '");
        return create_token(TOKEN_UNKOWN);
    }

    Token token = create_token(TOKEN_SQUOTED_STRING);
    advance();

    return token;
}

Token argument()
{
    while (!is_at_end()) {
        if (previous() == '$' && match('(')) {
            advance_command_substitution();
        } else if (is_argument(peek())) {
            advance();
        } else {
            break;
        }
    }

    Z_String_View arg = Z_SV(start, curr - start);

    for (int i = 0; i < (int)(sizeof(keywords) / sizeof(keywords[0])); i++) {
        if (!z_str_compare(arg, Z_CSTR_TO_SV(keywords[i].lexeme))) {
            return create_token(keywords[i].type);
        }
    }

    return create_token(TOKEN_WORD);
}

bool is_space(char c)
{
    return strchr(" \t\r", c);
}

void skip_spaces()
{
    while (!is_at_end() && is_space(peek())) {
        advance();
    }

    start = curr;
}

void skip_comment()
{
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
}

Token lexer_next()
{
    skip_spaces();

    if (is_at_end()) {
        return create_token(TOKEN_EOD);
    }

    char c = advance();

    switch (c) {
        case '|':
            return create_token(match('|') ? TOKEN_OR : TOKEN_PIPE);

        case '&':
            return create_token(match('&') ? TOKEN_AND : TOKEN_AMPERSAND);

        case '\'':
            return single_quoted_string();

        case '"':
            return match_string("\"\"") ? multi_double_quoted_string() : double_quoted_string();

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

void expand_alias(Token key, Token_Vec *output)
{
    const char *value = get_alias(key.lexeme);

    if (!value) {
        z_da_append(output, key);
    } else {
        Token_Vec tmp = lexer_get_tokens(Z_CSTR_TO_SV(value));
        z_da_append_da(output, &tmp);
        output->len--; // remove EOF token
        free(tmp.ptr);
    }
}

void alias_expension(Token_Vec *tokens)
{
    Token_Vec tmp = {0};
    bool is_command_start = true;

    for (int i = 0; i < tokens->len; i++) {
        Token token = tokens->ptr[i];

        if (token.type != TOKEN_WORD) {
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

Token_Vec lexer_get_tokens(Z_String_View source)
{
    end = source.ptr + source.len;
    start = source.ptr;
    curr = source.ptr;
    had_error = false;
    line = 0;

    Token_Vec tokens = {0};
    Token token = lexer_next();

    while (token.type != TOKEN_EOD) {
        z_da_append(&tokens, token);
        token = lexer_next();
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
