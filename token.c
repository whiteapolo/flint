#include <stdio.h>
#include "token.h"
#include "libzatar.h"

const char *token_type_to_string(Token_Type type)
{
    switch (type) {
        case TOKEN_PIPE:            return "PIPE";
        case TOKEN_AND_IF:          return "AND_IF";
        case TOKEN_AMPERSAND:       return "AMPERSAND";
        case TOKEN_ERROR:           return "ERROR";
        case TOKEN_EOD:             return "EOD";
        case TOKEN_STATEMENT_END:   return "STATEMENT_END";
        case TOKEN_FOR:             return "FOR";
        case TOKEN_IF:              return "IF";
        case TOKEN_IN:              return "IN";
        case TOKEN_FUN:             return "FUN";
        case TOKEN_END:             return "END";

        case TOKEN_WORD:            return "TOKEN_WORD";
        case TOKEN_DQUOTED_STRING:  return "TOKEN_DQUOTED_STRING";
        case TOKEN_SQUOTED_STRING:  return "TOKEN_SQUOTED_STRING";

        default:                    return "UNKNOWN";
    }
}

void print_token(Token token)
{
    const char *type_str = token_type_to_string(token.type);

    printf("Token(%s, \"%.*s\")\n",
            type_str, token.lexeme.len, token.lexeme.ptr);
}
