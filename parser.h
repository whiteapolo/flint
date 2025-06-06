#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "token.h"
#include <stdbool.h>

typedef enum {
    NODE_TYPE_COMMAND,
    NODE_TYPE_PIPE,
    NODE_TYPE_AND_IF,
    NODE_TYPE_AMPERSAND,
} NODE_TYPE;

typedef struct Parser_Node {
    NODE_TYPE type;
    struct Parser_Node *left;
    struct Parser_Node *right;
    char **argv;
} Parser_Node;

Parser_Node *parse(Lexer *lexer);
void parser_free(Parser_Node *node);

#endif
