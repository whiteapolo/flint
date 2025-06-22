#include "lexer.h"
#include "token.h"
#include "builtins/alias.h"
#include <ctype.h>
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
};

static Token create_error_token(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    had_error = true;

    Token token = {
        .lexeme = Z_CSTR_TO_SV(msg),
        .type = TOKEN_ERROR,
    };

    return token;
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

static char advance()
{
    if (peek() == '\n') {
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

    if (peek() == expected) {
        advance();
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
    while (!is_at_end() && peek() != c) {

        if (peek() == '\n') {
            line++;
        }

        advance();
    }
}

Token double_quoted_string()
{
    start = curr;

    advance_utill('"');

    if (is_at_end()) {
        return create_error_token("unexpected EOF while looking for matching '\"'");
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
        return create_error_token("unexpected EOF while looking for matching '''");
    }

    Token token = create_token(TOKEN_SQUOTED_STRING);
    advance();

    return token;
}

void advance_command_substitution()
{
    while (!is_at_end() && advance() != ')') { }
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
    while (!is_at_end() && advance() != '\n') { }
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
            return create_token(TOKEN_PIPE);

        case '&':
            return create_token(match('&') ? TOKEN_AND_IF : TOKEN_AMPERSAND);

        case '\'':
            return single_quoted_string();

        case '"':
            return double_quoted_string();

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

    while (!is_at_end()) {
        z_da_append(&tokens, token);
        token = lexer_next();
    }

    z_da_append(&tokens, token);
    z_da_append(&tokens, lexer_next());

    return tokens;
}

void lexer_print_tokens(const Token_Vec *tokens)
{
    for (int i = 0; i < tokens->len; i++) {
        print_token(tokens->ptr[i]);
    }
}
