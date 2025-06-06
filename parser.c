#include "parser.h"
#include "lexer.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

Parser_Node *parser_node_new_command(char **argv)
{
    Parser_Node *node = malloc(sizeof(Parser_Node));
    node->type = NODE_TYPE_COMMAND;
    node->argv = argv;

    return node;
}

Parser_Node *parser_node_new_binary(NODE_TYPE type, Parser_Node *left, Parser_Node *right)
{
    Parser_Node *node = malloc(sizeof(Parser_Node));
    node->type = type;
    node->left = left;
    node->right = right;

    return node;
}

Parser_Node *parser_node_new_unary(NODE_TYPE type, Parser_Node *child)
{
    Parser_Node *node = malloc(sizeof(Parser_Node));
    node->type = type;
    node->child = child;

    return node;
}

void free_argv(char **argv)
{
    char **curr = argv;

    while (*curr) {
        free(*(curr++));
    }
}


Parser_Node *parse_command(Lexer *lexer)
{
    char **argv = NULL;
    int len = 0;

    while (lexer_peek(lexer).type == TOKEN_TYPE_STRING) {
        Token token = lexer_next(lexer);
        argv = realloc(argv, sizeof(char *) * (++len + 1));
        argv[len - 1] = strndup(token.lexeme, token.len);
        argv[len] = NULL;
    }

    if (len == 0) {
        return NULL;
    }

    return parser_node_new_command(argv);
}

Parser_Node *parse_ampersand(Lexer *lexer)
{
    Parser_Node *command = parse_command(lexer);

    if (command == NULL) {
        if (lexer_peek(lexer).type == TOKEN_TYPE_AMPERSAND) {
            fprintf(stderr, "Expected command before '&'\n");
        }
        return NULL;
    }

    if (lexer_peek(lexer).type == TOKEN_TYPE_AMPERSAND) {
        lexer_next(lexer);
        return parser_node_new_unary(NODE_TYPE_AMPERSAND, command);
    }

    return command;
}

void reverse_ast(Parser_Node *ast)
{
    if (ast->type == NODE_TYPE_PIPE || ast->type == NODE_TYPE_AND_IF) {
        Parser_Node *tmp = (Parser_Node *)ast->right;
        ast->right = ast->left;
        ast->left = tmp;
    } else if (ast->type == NODE_TYPE_AMPERSAND) {
        reverse_ast(ast->child);
    }
}

Parser_Node *parse_pipe(Lexer *lexer)
{
    Parser_Node *ast = parse_ampersand(lexer);

    if (ast == NULL) {
        if (lexer_peek(lexer).type == TOKEN_TYPE_PIPE) {
            fprintf(stderr, "Expected command before '|'\n");
        }
        return NULL;
    }

    if (lexer_peek(lexer).type == TOKEN_TYPE_PIPE && ast->type == NODE_TYPE_AMPERSAND) {
        fprintf(stderr, "Left side of '|' must be a regular command without '&'\n");
        return NULL;
    }

    while (lexer_peek(lexer).type == TOKEN_TYPE_PIPE) {
        lexer_next(lexer);
        Parser_Node *right = parse_pipe(lexer);

        if (right == NULL) {
            fprintf(stderr, "Expected command after '|'\n");
            return NULL;
        }

        ast = parser_node_new_binary(NODE_TYPE_PIPE, ast, right);
    }

    // reverse_ast(ast);

    return ast;
}


Parser_Node *parse_and_if(Lexer *lexer)
{
    Parser_Node *ast = parse_pipe(lexer);

    if (ast == NULL) {
        if (lexer_peek(lexer).type == TOKEN_TYPE_AND_IF) {
            fprintf(stderr, "Expected expression before '&&'\n");
        }
        return NULL;
    }

    if (lexer_peek(lexer).type == TOKEN_TYPE_AND_IF && ast->type == NODE_TYPE_AMPERSAND) {
        fprintf(stderr, "Left side of '&&' must be a regular command without '&'\n");
        return NULL;
    }

    while (lexer_peek(lexer).type == TOKEN_TYPE_AND_IF) {
        lexer_next(lexer);
        Parser_Node *right = parse_and_if(lexer);

        if (right == NULL) {
            fprintf(stderr, "Expected expression after '&&'\n");
            return NULL;
        }

        ast = parser_node_new_binary(NODE_TYPE_AND_IF, ast, right);
    }

    return ast;
}

Parser_Node *parse(Lexer *lexer)
{
    if (lexer_peek(lexer).type == TOKEN_TYPE_EOD) {
        return NULL;
    }

    return parse_and_if(lexer);
}

void parser_free(Parser_Node *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case NODE_TYPE_COMMAND:
            free_argv(node->argv);
            break;

        case NODE_TYPE_PIPE:
        case NODE_TYPE_AND_IF:
            parser_free((Parser_Node *)node->left);
            parser_free((Parser_Node *)node->right);
            break;

        case NODE_TYPE_AMPERSAND:
            parser_free((Parser_Node *)node->child);
            break;
    }

    free(node);
}
