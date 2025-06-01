#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_TYPE_PIPE,
    TOKEN_TYPE_AND_IF,
    TOKEN_TYPE_AMPERSAND,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_ERROR,
    TOKEN_TYPE_EOD,
} TOKEN_TYPE;

typedef struct {
    TOKEN_TYPE type;
    const char *lexeme;
    int len;
} Token;

Token new_token(TOKEN_TYPE type, const char *lexeme, int len);
void print_token(Token token);
const char *token_type_to_string(TOKEN_TYPE type);

#endif
