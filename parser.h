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

// typedef struct {
//     NODE_TYPE type;
//     char **argv;
// } Parser_Node_Command;

// typedef struct {
//     NODE_TYPE type;
//     union Parser_Node *right;
//     union Parser_Node *left;
// } Parser_Node_Binary;

// typedef struct {
//     NODE_TYPE type;
//     union Parser_Node_Command *child;
// } Parser_Node_Unary;

// typedef union {
//     NODE_TYPE type;
//     Parser_Node_Unary unary;
//     Parser_Node_Binary binary;
//     Parser_Node_Command command;
// } Parser_Node;

typedef struct Parser_Node {
    NODE_TYPE type;
    struct Parser_Node *child;
    struct Parser_Node *left;
    struct Parser_Node *right;
    char **argv;
} Parser_Node;

Parser_Node *parse(Lexer *lexer);
void parser_free(Parser_Node *node);

#endif
