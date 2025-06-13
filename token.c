#include "token.h"
#include "libzatar.h"
#include <stdio.h>

const char *token_type_to_string(TOKEN_TYPE type)
{
    switch (type) {
        case TOKEN_TYPE_PIPE:        return "PIPE";
        case TOKEN_TYPE_AND_IF:      return "AND_IF";
        case TOKEN_TYPE_AMPERSAND:   return "AMPERSAND";
        case TOKEN_TYPE_STRING:      return "STRING";
        case TOKEN_TYPE_ERROR:       return "ERROR";
        case TOKEN_TYPE_EOD:         return "EOD";
        default:                     return "UNKNOWN";
    }
}

void print_token(Token token)
{
    const char *type_str = token_type_to_string(token.type);

    printf("Token(%s, \"%.*s\")\n",
            type_str, token.lexeme.len, token.lexeme.ptr);
}
