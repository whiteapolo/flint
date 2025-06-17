#include "parser.h"
#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static const Token_Vec *tokens;
static int curr;
static bool had_error;

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
//     return peek().type == TOKEN_EOD;
// }

static bool check(Token_Type type)
{
    return peek().type == type;
}

// static bool match(Token_Type type)
// {
//     if (is_at_end()) return false;
//     return peek().type == type;
// }

static bool check_node_operator(Ast_Node *node, Token_Type expected)
{
    if (node->type == AST_NODE_BINARY) {
        return ((Ast_Node_Binary *)node)->operator.type == expected;
    }

    if (node->type == AST_NODE_UNARY) {
        return ((Ast_Node_Unary *)node)->operator.type == expected;
    }

    return false;
}

Ast_Node *create_binary_node(Ast_Node *left, Token operator, Ast_Node *right)
{
    Ast_Node_Binary *node = malloc(sizeof(Ast_Node_Binary));
    node->type = AST_NODE_BINARY;
    node->left = left;
    node->operator = operator;
    node->right = right;

    return (Ast_Node *)node;
}

Ast_Node *create_unary_node(Token operator, Ast_Node *child)
{
    Ast_Node_Unary *node = malloc(sizeof(Ast_Node_Unary));
    node->type = AST_NODE_UNARY;
    node->operator = operator;
    node->child = child;

    return (Ast_Node *)node;
}

Ast_Node *create_command_node(char **argv)
{
    Ast_Node_Command *node = malloc(sizeof(Ast_Node_Command));
    node->type = AST_NODE_COMMAND;
    node->argv = argv;

    return (Ast_Node *)node;
}

void free_string_array(char **s)
{
    char **curr = s;

    while (*curr) {
        free(*(curr++));
    }

    free(s);
}

Ast_Node *parse_command()
{
    char **argv = NULL;
    int len = 0;

    while (check(TOKEN_STRING)) {
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

Ast_Node *parse_ampersand()
{
    Ast_Node *command = parse_command();

    if (command == NULL) {
        if (check(TOKEN_AMPERSAND)) {
            fprintf(stderr, "Expected command before '&'\n");
        }
        return NULL;
    }

    if (check(TOKEN_AMPERSAND)) {
        return create_unary_node(advance(), command);
    }

    return command;
}

Ast_Node *parse_pipe()
{
    Ast_Node *ast = parse_ampersand();

    if (ast == NULL) {
        if (check(TOKEN_PIPE)) {
            fprintf(stderr, "Expected command before '|'\n");
        }
        return NULL;
    }

    if (check(TOKEN_PIPE) && check_node_operator(ast, TOKEN_AMPERSAND)) {
        fprintf(stderr, "Left side of '|' must be a regular command without '&'\n");
        return NULL;
    }

    if (check(TOKEN_PIPE)) {
        Token pipe = advance();
        Ast_Node *right = parse_pipe();

        if (right == NULL) {
            fprintf(stderr, "Expected command after '|'\n");
            return NULL;
        }

        ast = create_binary_node(ast, pipe, right);
    }

    return ast;
}

Ast_Node *parse_and_if()
{
    Ast_Node *ast = parse_pipe();

    if (ast == NULL) {
        if (check(TOKEN_AND_IF)) {
            fprintf(stderr, "Expected expression before '&&'\n");
        }
        return NULL;
    }

    if (check(TOKEN_AND_IF) && check_node_operator(ast, TOKEN_AMPERSAND)) {
        fprintf(stderr, "Left side of '&&' must be a regular command without '&'\n");
        return NULL;
    }

    if (check(TOKEN_AND_IF)) {
        Token and_if = advance();
        Ast_Node *right = parse_and_if();

        if (right == NULL) {
            fprintf(stderr, "Expected expression after '&&'\n");
            return NULL;
        }

        ast = create_binary_node(ast, and_if, right);
    }

    return ast;
}

Ast_Node *parse_expression()
{
    return parse_and_if();
}

Ast_Node *parse(const Token_Vec *t)
{
    tokens = t;
    curr = 0;
    had_error = false;

    Ast_Node *ast = parse_expression();

    if (had_error) {
        return NULL;
    }

    return ast;
}

void render_ast_binary(Ast_Node_Binary *ast, Z_String *output)
{
    z_str_append_format(output, "(");
    z_str_append_str(output, ast->operator.lexeme);
    z_str_append_format(output, " ");
    render_ast(ast->left, output);
    z_str_append_format(output, " ");
    render_ast(ast->right, output);
    z_str_append_format(output, ")");
}

void render_ast_unary(Ast_Node_Unary *ast, Z_String *output)
{
    z_str_append_format(output, "(");
    z_str_append_str(output, ast->operator.lexeme);
    z_str_append_format(output, " ");
    render_ast(ast->child, output);
    z_str_append_format(output, ")");
}

void render_ast_command(Ast_Node_Command *ast, Z_String *output)
{
    char **curr = ast->argv;
    z_str_append_format(output, "(%s", *(curr++));

    while (*curr) {
        z_str_append_format(output, " %s", *(curr++));
    }

    z_str_append_format(output, ")");
}

void render_ast(Ast_Node *ast, Z_String *output)
{
    if (ast->type == AST_NODE_BINARY) {
        render_ast_binary((Ast_Node_Binary *)ast, output);
    } else if (ast->type == AST_NODE_UNARY) {
        render_ast_unary((Ast_Node_Unary *)ast, output);
    } else if (ast->type == AST_NODE_COMMAND) {
        render_ast_command((Ast_Node_Command *)ast, output);
    } else {
        z_str_append_format(output, "(Unknown)");
    }
}

void print_ast(Ast_Node *ast)
{
    Z_String s = {0};
    render_ast(ast, &s);
    z_str_println(Z_STR_TO_SV(s));
    free(s.ptr);
}

void parser_free(Ast_Node *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case AST_NODE_COMMAND:
            free_string_array(((Ast_Node_Command *)node)->argv);
            break;

        case AST_NODE_UNARY:
            parser_free(((Ast_Node_Unary *)node)->child);
            break;

        case AST_NODE_BINARY:
            parser_free(((Ast_Node_Binary *)node)->right);
            parser_free(((Ast_Node_Binary *)node)->left);
            break;
    }

    free(node);
}
