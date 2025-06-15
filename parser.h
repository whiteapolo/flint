#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "token.h"
#include <stdbool.h>

typedef enum {
    Parser_Node_Type_COMMAND,
    Parser_Node_Type_BINARY,
    Parser_Node_Type_UNARY,
} Parser_Node_Type;

typedef struct Parser_Node {
    Parser_Node_Type type;
} Parser_Node;

typedef struct {
    Parser_Node_Type type;
    char **argv;
} Parser_Node_Command;

typedef struct {
    Parser_Node_Type type;
    Token operator;
    Parser_Node *child;
} Parser_Node_Unary;

typedef struct {
    Parser_Node_Type type;
    Token operator;
    Parser_Node *left;
    Parser_Node *right;
} Parser_Node_Binary;

Parser_Node *parse(const Token_Vec *t);
void parser_free(Parser_Node *node);

#endif
