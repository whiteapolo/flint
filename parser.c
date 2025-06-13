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

Token eat()
{
    return tokens->ptr[curr++];
}

Token peek()
{
    return tokens->ptr[curr];
}

Parser_Node *parse_new_node(NODE_TYPE type, Parser_Node *left, Parser_Node *right, char **argv)
{
    Parser_Node *node = malloc(sizeof(Parser_Node));
    node->type = type;
    node->argv = argv;
    node->left = left;
    node->right = right;

    return node;
}

void free_argv(char **argv)
{
    for (char **curr = argv; *curr; curr++) {
        free(*curr);
    }

    free(argv);
}


Parser_Node *parse_command()
{
    char **argv = NULL;
    int len = 0;

    while (peek().type == TOKEN_TYPE_STRING) {
        Token token = eat();
        argv = realloc(argv, sizeof(char *) * (++len + 1));
        argv[len - 1] = strndup(token.lexeme.ptr, token.lexeme.len);
        argv[len] = NULL;
    }

    if (len == 0) {
        return NULL;
    }

    return parse_new_node(NODE_TYPE_COMMAND, NULL, NULL, argv);
}

Parser_Node *parse_ampersand()
{
    Parser_Node *command = parse_command();

    if (command == NULL) {
        if (peek().type == TOKEN_TYPE_AMPERSAND) {
            fprintf(stderr, "Expected command before '&'\n");
        }
        return NULL;
    }

    if (peek().type == TOKEN_TYPE_AMPERSAND) {
        eat();
        command->type = NODE_TYPE_AMPERSAND;
    }

    return command;
}

Parser_Node *parse_pipe()
{
    Parser_Node *ast = parse_ampersand();

    if (ast == NULL) {
        if (peek().type == TOKEN_TYPE_PIPE) {
            fprintf(stderr, "Expected command before '|'\n");
        }
        return NULL;
    }

    if (peek().type == TOKEN_TYPE_PIPE && ast->type == NODE_TYPE_AMPERSAND) {
        fprintf(stderr, "Left side of '|' must be a regular command without '&'\n");
        return NULL;
    }

    while (peek().type == TOKEN_TYPE_PIPE) {
        eat();
        Parser_Node *right = parse_pipe();

        if (right == NULL) {
            fprintf(stderr, "Expected command after '|'\n");
            return NULL;
        }

        ast = parse_new_node(NODE_TYPE_PIPE, ast, right, NULL);
    }

    return ast;
}


Parser_Node *parse_and_if()
{
    Parser_Node *ast = parse_pipe();

    if (ast == NULL) {
        if (peek().type == TOKEN_TYPE_AND_IF) {
            fprintf(stderr, "Expected expression before '&&'\n");
        }
        return NULL;
    }

    if (peek().type == TOKEN_TYPE_AND_IF && ast->type == NODE_TYPE_AMPERSAND) {
        fprintf(stderr, "Left side of '&&' must be a regular command without '&'\n");
        return NULL;
    }

    while (peek().type == TOKEN_TYPE_AND_IF) {
        eat();
        Parser_Node *right = parse_and_if();

        if (right == NULL) {
            fprintf(stderr, "Expected expression after '&&'\n");
            return NULL;
        }

        ast = parse_new_node(NODE_TYPE_AND_IF, ast, right, NULL);
    }

    return ast;
}

Parser_Node *parse(const Token_Vec *t)
{
    tokens = t;
    curr = 0;

    return parse_and_if();
}

void parser_free(Parser_Node *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case NODE_TYPE_COMMAND:
        case NODE_TYPE_AMPERSAND:
            free_argv(node->argv);
            break;

        case NODE_TYPE_PIPE:
        case NODE_TYPE_AND_IF:
            parser_free((Parser_Node *)node->left);
            parser_free((Parser_Node *)node->right);
            break;
    }

    free(node);
}
