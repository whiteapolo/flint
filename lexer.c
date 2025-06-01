#include "lexer.h"
#include "token.h"
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

void lexer_init(Lexer *lexer, const char *source, int len)
{
    lexer->source = source;
    lexer->curr = source;
    lexer->end = source + len;
}

Token eat_string(Lexer *lexer)
{
    // TODO: add support for " "
    lexer->curr++; // eat '

    const char *lexeme = lexer->curr;

    while (lexer->curr < lexer->end && *lexer->curr != '\'') {
        lexer->curr++;
    }

    if (lexer->curr >= lexer->end) {
        const char *err_msg = "unexpected EOF while looking for matching '''";
        return new_token(TOKEN_TYPE_ERROR, err_msg, strlen(err_msg));
    }

    lexer->curr++; // eat '

    return new_token(TOKEN_TYPE_STRING, lexeme, lexer->curr - lexeme - 1);
}

bool is_argument(char c)
{
    return isalnum(c) || !strchr(" &|;()", c);
}

Token eat_argument(Lexer *lexer)
{
    const char *lexeme = lexer->curr;

    while (lexer->curr < lexer->end && is_argument(*lexer->curr)) {
        lexer->curr++;
    }

    return new_token(TOKEN_TYPE_STRING, lexeme, lexer->curr - lexeme);
}

Token lexer_peek(const Lexer *lexer)
{
    Lexer cpy = *lexer;

    return lexer_next(&cpy);
}

Token lexer_next(Lexer *lexer)
{
    while (lexer->curr < lexer->end && isspace(*lexer->curr)) {
        lexer->curr++;
    }

    if (lexer->curr >= lexer->end) {
        return new_token(TOKEN_TYPE_EOD, NULL, 0);
    }

    switch (*lexer->curr) {
        case '|':
            lexer->curr++;
            return new_token(TOKEN_TYPE_PIPE, "|", 1);

        case '&': {
            if (lexer->curr + 1 < lexer->end && lexer->curr[1] == '&') {
                lexer->curr += 2;
                return new_token(TOKEN_TYPE_AND_IF, "&&", 2);
            }

            lexer->curr++;
            return new_token(TOKEN_TYPE_AMPERSAND, "&", 1);
        }

        case '\'':
            return eat_string(lexer);

        default:
            return eat_argument(lexer);
    }
}
