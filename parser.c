#include "parser.h"
#include "lexer.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static const Token_Vec *tokens;
static int curr;

static Token advance()
{
    return tokens->ptr[curr++];
}

static Token peek()
{
    return tokens->ptr[curr];
}

// static bool is_at_end()
// {
//     return peek().type == TOKEN_TYPE_EOD;
// }

static bool check(TOKEN_TYPE type)
{
    return peek().type == type;
}

// static bool match(TOKEN_TYPE type)
// {
//     if (is_at_end()) return false;
//     return peek().type == type;
// }

static bool check_node_operator(Parser_Node *node, TOKEN_TYPE expected)
{
    if (node->type == Parser_Node_Type_BINARY) {
        return ((Parser_Node_Binary *)node)->operator.type == expected;
    }

    if (node->type == Parser_Node_Type_UNARY) {
        return ((Parser_Node_Unary *)node)->operator.type == expected;
    }

    return false;
}

Parser_Node *create_binary_node(Parser_Node *left, Token operator, Parser_Node *right)
{
    Parser_Node_Binary *node = malloc(sizeof(Parser_Node_Binary));
    node->type = Parser_Node_Type_BINARY;
    node->left = left;
    node->operator = operator;
    node->right = right;

    return (Parser_Node *)node;
}

Parser_Node *create_unary_node(Token operator, Parser_Node *child)
{
    Parser_Node_Unary *node = malloc(sizeof(Parser_Node_Unary));
    node->type = Parser_Node_Type_UNARY;
    node->operator = operator;
    node->child = child;

    return (Parser_Node *)node;
}

Parser_Node *create_command_node(char **argv)
{
    Parser_Node_Command *node = malloc(sizeof(Parser_Node_Command));
    node->type = Parser_Node_Type_COMMAND;
    node->argv = argv;

    return (Parser_Node *)node;
}

void free_string_array(char **s)
{
    char **curr = s;

    while (*curr) {
        free(*(curr++));
    }

    free(s);
}

Parser_Node *parse_command()
{
    char **argv = NULL;
    int len = 0;

    while (check(TOKEN_TYPE_STRING)) {
        Token token = advance();
        argv = realloc(argv, sizeof(char *) * (++len + 1));
        argv[len - 1] = strndup(token.lexeme.ptr, token.lexeme.len);
        argv[len] = NULL;
    }

    if (len == 0) {
        return NULL;
    }

    return create_command_node(argv);
}

Parser_Node *parse_ampersand()
{
    Parser_Node *command = parse_command();

    if (command == NULL) {
        if (check(TOKEN_TYPE_AMPERSAND)) {
            fprintf(stderr, "Expected command before '&'\n");
        }
        return NULL;
    }

    if (check(TOKEN_TYPE_AMPERSAND)) {
        return create_unary_node(advance(), command);
    }

    return command;
}

Parser_Node *parse_pipe()
{
    Parser_Node *ast = parse_ampersand();

    if (ast == NULL) {
        if (check(TOKEN_TYPE_PIPE)) {
            fprintf(stderr, "Expected command before '|'\n");
        }
        return NULL;
    }

    if (check(TOKEN_TYPE_PIPE) && check_node_operator(ast, TOKEN_TYPE_AMPERSAND)) {
        fprintf(stderr, "Left side of '|' must be a regular command without '&'\n");
        return NULL;
    }

    while (check(TOKEN_TYPE_PIPE)) {
        Token pipe = advance();
        Parser_Node *right = parse_pipe();

        if (right == NULL) {
            fprintf(stderr, "Expected command after '|'\n");
            return NULL;
        }

        ast = create_binary_node(ast, pipe, right);
    }

    return ast;
}

Parser_Node *parse_and_if()
{
    Parser_Node *ast = parse_pipe();

    if (ast == NULL) {
        if (check(TOKEN_TYPE_AND_IF)) {
            fprintf(stderr, "Expected expression before '&&'\n");
        }
        return NULL;
    }

    if (check(TOKEN_TYPE_AND_IF) && check_node_operator(ast, TOKEN_TYPE_AMPERSAND)) {
        fprintf(stderr, "Left side of '&&' must be a regular command without '&'\n");
        return NULL;
    }

    while (check(TOKEN_TYPE_AND_IF)) {
        Token and_if = advance();
        Parser_Node *right = parse_and_if();

        if (right == NULL) {
            fprintf(stderr, "Expected expression after '&&'\n");
            return NULL;
        }

        ast = create_binary_node(ast, and_if, right);
    }

    return ast;
}

Parser_Node *parse_expression()
{
    return parse_and_if();
}

Parser_Node *parse(const Token_Vec *t)
{
    tokens = t;
    curr = 0;

    return parse_expression();
}

void parser_free(Parser_Node *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case Parser_Node_Type_COMMAND:
            free_string_array(((Parser_Node_Command *)node)->argv);
            break;

        case Parser_Node_Type_UNARY:
            parser_free(((Parser_Node_Unary *)node)->child);
            break;

        case Parser_Node_Type_BINARY:
            parser_free(((Parser_Node_Binary *)node)->right);
            parser_free(((Parser_Node_Binary *)node)->left);
            break;
    }

    free(node);
}
