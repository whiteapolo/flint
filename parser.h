#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "token.h"
#include <stdbool.h>

typedef enum {
    AST_NODE_COMMAND,
    AST_NODE_BINARY,
    AST_NODE_UNARY,
} Ast_Node_Type;

typedef struct Ast_Node {
    Ast_Node_Type type;
} Ast_Node;

typedef struct {
    Ast_Node_Type type;
    char **argv;
} Ast_Node_Command;

typedef struct {
    Ast_Node_Type type;
    Token operator;
    Ast_Node *child;
} Ast_Node_Unary;

typedef struct {
    Ast_Node_Type type;
    Token operator;
    Ast_Node *left;
    Ast_Node *right;
} Ast_Node_Binary;

Ast_Node *parse(const Token_Vec *t);
void parser_free(Ast_Node *node);
void render_ast(Ast_Node *ast, Z_String *output);
void print_ast(Ast_Node *ast);

#endif
