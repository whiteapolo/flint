#include <stdio.h>
#include "token.h"
#include "libzatar.h"

const char *token_type_to_string(Token_Type type)
{
    switch (type) {
        case TOKEN_PIPE:        return "PIPE";
        case TOKEN_AND_IF:      return "AND_IF";
        case TOKEN_AMPERSAND:   return "AMPERSAND";
        case TOKEN_STRING:      return "STRING";
        case TOKEN_ERROR:       return "ERROR";
        case TOKEN_EOD:         return "EOD";
        default:                     return "UNKNOWN";
    }
}

void print_token(Token token)
{
    const char *type_str = token_type_to_string(token.type);

    printf("Token(%s, \"%.*s\")\n",
            type_str, token.lexeme.len, token.lexeme.ptr);
}
