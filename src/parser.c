#include "parser.h"
#include "error.h"
#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>

/*
 * GRAMMER
 * -----------------------------------------------
 * program: statement*
 *
 * statement: for_statement | if_statement | job
 *
 * for_statement: "for" WORD "in" WORD*; statement*; end
 * if_statement: "if" job; statement*; end
 *
 * job: background_job
 * background_job: and_if "&"
 * and: pipeline { "&&" pipeline }*
 * or: and { "||" and }*
 * pipeline: simple_command { "|" simple_command }*
 * simple_command: WORD+
 */

void free_job(Job *job);
void free_if_statement(Statement_If *statement);
void free_job_statement(Statement_Job *statement);
void free_statement(Statement *statement);
void free_statements(Statement_Vec *statements);
Statement *parse_statement();

static const Token_Vec *tokens;
static int curr;
static bool had_error;
static bool panic_mode;
static Z_String_View source;

static Token advance()
{
    assert(curr < tokens->len);
    return tokens->ptr[curr++];
}

static Token peek()
{
    assert(curr < tokens->len);
    return tokens->ptr[curr];
}

static bool is_at_end()
{
    return peek().type == TOKEN_EOD;
}

static bool check(Token_Type type)
{
    if (panic_mode) {
        return false;
    }

    return peek().type == type;
}

static bool check_array(Token_Type types[], int len)
{
    for (int i = 0; i < len; i++) {
        if (check(types[i])) {
            return true;
        }
    }

    return false;
}

static bool check_keyword()
{
    Token_Type keywords[] = {
        TOKEN_FOR, TOKEN_IN, TOKEN_FUN,
        TOKEN_END, TOKEN_ELSE, TOKEN_WHILE,
    };

    return check_array(keywords, Z_ARRAY_LEN(keywords));
}

static bool check_string()
{
    Token_Type string_types[] = {
        TOKEN_WORD, TOKEN_DQUOTED_STRING, TOKEN_SQUOTED_STRING,
    };

    return check_array(string_types, Z_ARRAY_LEN(string_types));
}

static bool check_argument()
{
    // argument override keywords
    // for example:
    // echo if
    return check_string() || check_keyword();
}

static bool match(Token_Type type)
{
    if (had_error) {
        return false;
    }

    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

static void error(Token token, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (!panic_mode) {
        syntax_error_at_token_va(source, token, fmt, ap);
    }

    had_error = true;
    panic_mode = true;
    va_end(ap);
}

Token consume(Token_Type type, const char *msg)
{
    if (check(type)) {
        return advance();
    }

    error(peek(), msg);

    return peek();
}

Token consume_string(const char *msg)
{
    if (check_string()) {
        return advance();
    }

    error(peek(), msg);

    return peek();
}

Token consume_argument(const char *msg)
{
    if (check_argument()) {
        return advance();
    }

    error(peek(), msg);

    return peek();
}

void synchronize()
{
    match(TOKEN_STATEMENT_END);
    while (!is_at_end() && advance().type != TOKEN_STATEMENT_END) {}
    panic_mode = false;
}

void skip_empty_statements()
{
    while (!is_at_end() && match(TOKEN_STATEMENT_END)) { }
}

Statement *create_statement_if(Job *condition, Statement_Vec ifBranch, Statement_Vec elseBranch)
{
    Statement_If *node = malloc(sizeof(Statement_If));
    node->type = STATEMENT_IF;
    node->condition = condition;
    node->ifBranch = ifBranch;
    node->elseBranch = elseBranch;

    return (Statement *)node;
}

Statement *create_statement_while(Job *condition, Statement_Vec body)
{
    Statement_While *node = malloc(sizeof(Statement_While));
    node->type = STATEMENT_WHILE;
    node->condition = condition;
    node->body = body;

    return (Statement *)node;
}

Statement *create_statement_function(Token name, Statement_Vec body)
{
    Statement_Function *node = malloc(sizeof(Statement_Function));
    node->type = STATEMENT_FUNCTION;
    node->name = name;
    node->body = body;

    return (Statement *)node;
}


Statement *create_statement_for(Token var_name, Token string, Token delim, Statement_Vec body)
{
    Statement_For *node = malloc(sizeof(Statement_For));
    node->type = STATEMENT_FOR;
    node->var_name = var_name;
    node->string = string;
    node->delim = delim;
    node->body = body;

    return (Statement *)node;
}

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

Job *create_command(Argv argv)
{
    Job_Command *node = malloc(sizeof(Job_Command));
    node->type = JOB_COMMAND;
    node->argv = argv;

    return (Job *)node;
}

Job *parse_simple_command()
{
    Argv argv = {0};

    while (check_argument()) {
        Token token = advance();
        z_da_append(&argv, token);
    }

    if (argv.len == 0) {
        error(peek(), "Expected command.");
        return NULL;
    }

    return create_command(argv);
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

Job *parse_and()
{
    Job *job = parse_pipeline();

    while (check(TOKEN_AND)) {
        Token and_if = advance();
        Job *right = parse_pipeline();
        job = create_binary(job, and_if, right);
    }

    return job;
}

Job *parse_or()
{
    Job *job = parse_and();

    while (check(TOKEN_OR)) {
        Token or = advance();
        Job *right = parse_and();
        job = create_binary(job, or, right);
    }

    return job;
}

Job *parse_background_job()
{
    Job *job = parse_or();

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

Statement *parse_job_statement()
{
    Job *job = parse_job();
    return job ? create_statement_job(job) : NULL;
}

Statement_Vec parse_block_utill(Token_Type types[], int len)
{
    Statement_Vec statements = {0};
    skip_empty_statements();

    while (!is_at_end() && !check_array(types, len)) {

        Statement *statement = parse_statement();

        if (statement == NULL) {
            synchronize();
        } else {
            z_da_append(&statements, statement);
        }

        skip_empty_statements();
    }

    return statements;
}

Statement_Vec parse_block_utill_end()
{
    Token_Type end[] = { TOKEN_END };
    return parse_block_utill(end, Z_ARRAY_LEN(end));
}

Statement *parse_if_statement()
{
    Token_Type if_body_end[] = { TOKEN_ELSE, TOKEN_END };

    Job *condition = parse_job();

    if (condition == NULL) {
        return NULL;
    }

    skip_empty_statements();

    Statement_Vec ifBranch = parse_block_utill(if_body_end, Z_ARRAY_LEN(if_body_end));
    Statement_Vec elseBranch = {0};

    skip_empty_statements();

    if (match(TOKEN_ELSE)) {
        elseBranch = parse_block_utill_end();
    }

    skip_empty_statements();

    consume(TOKEN_END, "Expected 'end' after if statement");

    return create_statement_if(condition, ifBranch, elseBranch);
}

Statement *parse_while_statement()
{
    Job *condition = parse_job();

    if (condition == NULL) {
        return NULL;
    }

    skip_empty_statements();

    Statement_Vec body = parse_block_utill_end();

    consume(TOKEN_END, "Expected 'end' after if statement");

    return create_statement_while(condition, body);
}

Statement *parse_for_statement()
{
    if (!check_string()) {
        error(peek(), "Expected idenifier after for.");
        return NULL;
    }

    Token var_name = advance();
    consume(TOKEN_IN, "Expected 'in' after idenifier.");

    if (!check_string()) {
        error(peek(), "Expected string after in.");
        return NULL;
    }

    Token string = advance();

    consume(TOKEN_BY, "Expected 'by' after idenifier.");

    if (!check_string()) {
        error(peek(), "Expected delimeter string after by.");
        return NULL;
    }

    Token delim = advance();

    skip_empty_statements();

    Statement_Vec body = parse_block_utill_end();
    consume(TOKEN_END, "Expected 'end' after if statement");

    return create_statement_for(var_name, string, delim, body);
}

Statement *parse_function_statement()
{
    Token name = consume(TOKEN_WORD, "Expected function name after 'fn'");

    skip_empty_statements();

    Statement_Vec body = parse_block_utill_end();

    consume(TOKEN_END, "Expected 'end' after function body.");

    return create_statement_function(name, body);
}

Statement *parse_statement()
{
    if (match(TOKEN_IF))    return parse_if_statement();
    if (match(TOKEN_WHILE)) return parse_while_statement();
    if (match(TOKEN_FOR))   return parse_for_statement();
    if (match(TOKEN_FUN))   return parse_function_statement();

    return parse_job_statement();
}

Statement_Vec parse(const Token_Vec *t, Z_String_View s)
{
    tokens = t;
    curr = 0;
    had_error = false;
    panic_mode = false;
    source = s;

    Statement_Vec statements = {0};
    skip_empty_statements();

    while (!is_at_end()) {
        Statement *statement = parse_statement();

        if (statement == NULL) {
            synchronize();
        } else {
            z_da_append(&statements, statement);
        }

        skip_empty_statements();
    }

    if (had_error) {
        parser_free(&statements);
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
            free(((Job_Command *)job)->argv.ptr);
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

void free_if_statement(Statement_If *statement)
{
    free_job(statement->condition);
    free_statements(&statement->ifBranch);
    free(statement);
}

void free_while_statement(Statement_While *statement)
{
    free_job(statement->condition);
    free_statements(&statement->body);
    free(statement);
}

void free_for_statement(Statement_For *statement)
{
    free_statements(&statement->body);
    free(statement);
}

void free_function_statement(Statement_Function *statement)
{
    free_statements(&statement->body);
    free(statement);
}

void free_job_statement(Statement_Job *statement)
{
    Job *job = statement->job;
    free_job(job);
    free(statement);
}

void free_statement(Statement *statement)
{
    switch (statement->type) {
        case STATEMENT_JOB:
            free_job_statement((Statement_Job *)statement);
            break;

        case STATEMENT_IF:
            free_if_statement((Statement_If *)statement);
            break;

        case STATEMENT_WHILE:
            free_while_statement((Statement_While *)statement);
            break;

        case STATEMENT_FOR:
            free_for_statement((Statement_For *)statement);
            break;

        case STATEMENT_FUNCTION:
            free_function_statement((Statement_Function *)statement);
            break;
    }
}

void free_statements(Statement_Vec *statements)
{
    for (int i = 0; i < statements->len; i++) {
        free_statement(statements->ptr[i]);
    }

    free(statements->ptr);
}

void parser_free(Statement_Vec *statements)
{
    free_statements(statements);
}
