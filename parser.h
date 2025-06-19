#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "token.h"
#include <stdbool.h>

typedef struct {
    Token *ptr;
    int len;
    int capacity;
} Argv;

typedef enum {
    JOB_COMMAND,
    JOB_BINARY,
    JOB_UNARY,
} Job_Type;

typedef struct {
    Job_Type type;
} Job;

typedef struct {
    Job_Type type;
    Argv argv;
} Job_Command;

typedef struct {
    Job_Type type;
    Token operator;
    Job *child;
} Job_Unary;

typedef struct {
    Job_Type type;
    Token operator;
    Job *left;
    Job *right;
} Job_Binary;

typedef enum {
    // STATEMENT_FOR,
    // STATEMENT_IF,
    STATEMENT_JOB,
} Statement_Type;

typedef struct {
    Statement_Type type;
} Statement;

typedef struct {
    Statement **ptr;
    int len;
    int capacity;
} Statement_Vec;

typedef struct {
    Statement_Type type;
    Job *job;
} Statement_Job;

// typedef struct {
//     Token token;
//     Statement **statements;
// } Statement_For;

// typedef struct {
//     Token token;
//     Statement **statements;
// } Statement_If;

Statement_Vec parse(const Token_Vec *t, Z_String_View s);
void parser_free(Statement_Vec *node);

#endif
