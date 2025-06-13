#include "lexer.h"
#include "token.h"
#include "builtins/alias.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libzatar.h"

Lexer create_lexer(Z_String_View source)
{
    Lexer lexer = {
        .end = source.ptr + source.len,
        .curr = source.ptr,
        .start = source.ptr,
    };

    return lexer;
}

static Token create_error_token(const char *msg)
{
    Token token = {
        .lexeme = Z_CSTR_TO_SV(msg),
        .type = TOKEN_TYPE_ERROR,
    };

    return token;
}

static Token create_token(const Lexer *lexer, TOKEN_TYPE type)
{
    Token token = {
        .lexeme = Z_SV(lexer->start, lexer->curr - lexer->start),
        .type = type,
    };

    return token;
}

static bool is_at_end(const Lexer *lexer)
{
    return lexer->curr >= lexer->end;
}

static char advance(Lexer *lexer)
{
    return *(lexer->curr++);
}

static char peek(const Lexer *lexer)
{
    return *lexer->curr;
}

static bool match(Lexer *lexer, char expected)
{
    if (peek(lexer) == expected) {
        advance(lexer);
        return true;
    }

    return false;
}

static bool is_argument(char c)
{
    return !strchr(" &|;()", c);
}

Token eat_string(Lexer *lexer)
{
    // TODO: add support for " "

    while (!is_at_end(lexer) && peek(lexer) != '\'') {
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        return create_error_token("unexpected EOF while looking for matching '''");
    }

    advance(lexer);

    return create_token(lexer, TOKEN_TYPE_STRING);
}

Token eat_argument(Lexer *lexer)
{
    while (!is_at_end(lexer) && is_argument(peek(lexer))) {
        advance(lexer);
    }

    return create_token(lexer, TOKEN_TYPE_STRING);
}

void skip_spaces(Lexer *lexer)
{
    while (!is_at_end(lexer) && isspace(peek(lexer))) {
        advance(lexer);
    }

    lexer->start = lexer->curr;
}

Token lexer_next(Lexer *lexer)
{
    skip_spaces(lexer);

    if (is_at_end(lexer)) {
        return create_token(lexer, TOKEN_TYPE_EOD);
    }

    char c = advance(lexer);

    switch (c) {
        case '|':
            return create_token(lexer, TOKEN_TYPE_PIPE);

        case '&': {
            if (!is_at_end(lexer) && match(lexer, '&')) {
                return create_token(lexer, TOKEN_TYPE_AND_IF);
            }

            return create_token(lexer, TOKEN_TYPE_AMPERSAND);
        }

        case '\'':
            return eat_string(lexer);

        default:
            return eat_argument(lexer);
    }
}

void lexer_expand_alias(Token_Vec *tokens)
{
    (void)tokens;
#if 0
    Token_Vec tmp = {0};
    bool is_command_start = true;

    for (int i = 0; i < tokens->len; i++) {
        Token token = tokens->ptr[i];

        if (token.type != TOKEN_TYPE_STRING) {
            is_command_start = true;
            z_da_append(&tmp, token);
        } else {
            if (!is_command_start) {
                z_da_append(&tmp, token);
            } else {
                const char *alias = get_alias(token.lexeme);

                if (alias) {
                    lexer_get_tokens(alias, strlen(alias), &tmp);
                } else {
                    z_da_append(&tmp, token);
                }

                is_command_start = false;
            }
        }
    }

    tokens->len = 0;

    for (int i = 0; i < tmp.len; i++) {
        z_da_append(tokens, tmp.ptr[i]);
    }

    free(tmp.ptr);
#endif
}

Token_Vec lexer_get_tokens(Z_String_View source)
{
    Token_Vec tokens = {0};
    Lexer lexer = create_lexer(source);
    Token token = lexer_next(&lexer);

    while (token.type != TOKEN_TYPE_EOD) {
        z_da_append(&tokens, token);
        token = lexer_next(&lexer);
    }

    z_da_append(&tokens, token);
    // lexer_expand_alias(tokens);

    return tokens;
}

void lexer_print_tokens(Z_String_View source)
{
    Token_Vec tokens = lexer_get_tokens(source);

    for (int i = 0; i < tokens.len; i++) {
        print_token(tokens.ptr[i]);
    }
}
