#include "parser.h"
#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void free_job(Job *job);

static const Token_Vec *tokens;
static int curr;
static bool had_error;
static Z_String_View source;

static Token advance()
{
    return tokens->ptr[curr++];
}

static Token peek()
{
    return tokens->ptr[curr];
}

static bool is_at_end()
{
    return peek().type == TOKEN_EOD;
}

static bool check(Token_Type type)
{
    if (had_error) {
        return false;
    }

    return peek().type == type;
}

// static bool match(Token_Type type)
// {
//     if (had_error) {
//         return false;
//     }

//     if (check(type)) {
//         advance();
//         return true;
//     }

//     return false;
// }

Statement *create_statement_job(Job *job)
{
    Statement_Job *node = malloc(sizeof(Statement_Job));
    node->type = STATEMENT_JOB;
    node->job = job;

    return (Statement *)node;
}

Job *create_binary(Job *left, Token operator, Job *right)
{
    Job_Binary *node = malloc(sizeof(Job_Binary));
    node->type = JOB_BINARY;
    node->left = left;
    node->operator = operator;
    node->right = right;

    return (Job *)node;
}

Job *create_unary(Token operator, Job *child)
{
    Job_Unary *node = malloc(sizeof(Job_Unary));
    node->type = JOB_UNARY;
    node->operator = operator;
    node->child = child;

    return (Job *)node;
}

Job *create_command(char **argv)
{
    Job_Command *node = malloc(sizeof(Job_Command));
    node->type = JOB_COMMAND;
    node->argv = argv;

    return (Job *)node;
}

void free_string_array(char **s)
{
    char **curr = s;

    while (*curr) {
        free(*(curr++));
    }

    free(s);
}

Z_String_View get_token_line(Token token)
{
    const char *start = token.lexeme.ptr - 1;
    const char *end = token.lexeme.ptr;

    while (*start != '\n' && start > source.ptr) {
        start--;
    }

    if (*start == '\n') {
        start++;
    }

    while (end < source.ptr + source.len && *end != '\n') {
        end++;
    }

    return Z_SV(start, end - start);
}

void print_str_without_tabs(Z_String_View s)
{
    for (int i = 0; i < s.len; i++) {
        fprintf(stderr, "%c", s.ptr[i] == '\t' ? ' ' : s.ptr[i]);
    }
}

static void error(Token token, const char *msg)
{
    if (token.type == TOKEN_EOD || token.type == TOKEN_STATEMENT_END) {
        fprintf(stderr, "Error at end: %s\n", msg);
    } else {
        fprintf(stderr, "Error at '%.*s': %s\n", token.lexeme.len, token.lexeme.ptr, msg);
    }

    Z_String_View line = get_token_line(token);

    fprintf(stderr, "%5d | ", token.line);
    print_str_without_tabs(line);
    fprintf(stderr, "\n      | %*s^\n", (int)(token.lexeme.ptr - line.ptr), "");
    had_error = true;
}

Job *parse_simple_command()
{
    typedef struct {
        char **ptr;
        int len;
        int capacity;
    } Argv;

    Argv argv = {0};

    while (check(TOKEN_STRING)) {
        Token token = advance();
        z_da_append(&argv, strndup(token.lexeme.ptr, token.lexeme.len));
        z_da_null_terminate(&argv);
    }

    if (argv.len == 0) {
        error(peek(), "Expected command.");
        return NULL;
    }

    return create_command(argv.ptr);
}

Job *parse_pipeline()
{
    Job *job = parse_simple_command();

    while (check(TOKEN_PIPE)) {
        Token pipe = advance();
        Job *right = parse_simple_command();
        job = create_binary(job, pipe, right);
    }

    return job;
}

Job *parse_and_if()
{
    Job *job = parse_pipeline();

    while (check(TOKEN_AND_IF)) {
        Token and_if = advance();
        Job *right = parse_pipeline();
        job = create_binary(job, and_if, right);
    }

    return job;
}

Job *parse_background_job()
{
    Job *job = parse_and_if();

    if (check(TOKEN_AMPERSAND)) {
        Token ampersand = advance();
        return create_unary(ampersand, job);
    }

    return job;
}

Job *parse_job()
{
    return parse_background_job();
}

void syncronize()
{
    while (!is_at_end() && advance().type != TOKEN_STATEMENT_END) {}
}

Statement_Vec parse(const Token_Vec *t, Z_String_View s)
{
    tokens = t;
    curr = 0;
    had_error = false;
    source = s;

    Statement_Vec statements = {0};

    while (!is_at_end()) {

        if (peek().type == TOKEN_STATEMENT_END) {
            advance();
            continue;
        }

        Job *job = parse_job();

        if (had_error) {
            syncronize();
            free_job(job);
        } else {
            z_da_append(&statements, create_statement_job(job));
        }
    }

    if (had_error) {
        return (Statement_Vec){0};
    }

    return statements;
}


void free_job(Job *job)
{
    if (job == NULL) {
        return;
    }

    switch (job->type) {
        case JOB_COMMAND:
            free_string_array(((Job_Command *)job)->argv);
            break;

        case JOB_UNARY:
            free_job(((Job_Unary *)job)->child);
            break;

        case JOB_BINARY:
            free_job(((Job_Binary *)job)->left);
            free_job(((Job_Binary *)job)->right);
            break;
    }

    free(job);
}

void free_statement(Statement *statement)
{
    if (statement->type == STATEMENT_JOB) {
        Job *job = ((Statement_Job *)(statement))->job;
        free_job(job);
        free(statement);
    }
}

void parser_free(Statement_Vec *statements)
{
    for (int i = 0; i < statements->len; i++) {
        free_statement(statements->ptr[i]);
    }

    free(statements->ptr);
}
