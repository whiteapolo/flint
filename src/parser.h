#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include <stdbool.h>

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
  Token_Vec argv;
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
  STATEMENT_JOB,
  STATEMENT_IF,
  STATEMENT_WHILE,
  STATEMENT_FOR,
  STATEMENT_FUNCTION,
} Statement_Type;

typedef struct {
  Statement_Type type;
} Statement;

typedef struct {
  Statement **ptr;
  int len;
  int cap;
} Statement_Vec;

typedef struct {
  Statement_Type type;
  Job *job;
} Statement_Job;

typedef struct {
  Statement_Type type;
  Job *condition;
  Statement_Vec ifBranch;
  Statement_Vec elseBranch;
} Statement_If;

typedef struct {
  Statement_Type type;
  Job *condition;
  Statement_Vec body;
} Statement_While;

typedef struct {
  Statement_Type type;
  Token var_name;
  Token string;
  Token delim;
  Statement_Vec body;
} Statement_For;

typedef struct {
  Statement_Type type;
  Token name;
  Statement_Vec body;
} Statement_Function;

Statement_Vec parse(const Token_Vec *t, const char *_source);
void parser_free(Statement_Vec *node);
void free_function_statement(Statement_Function *statement);

Statement_Vec clone_statements(Statement_Vec statements);
Job *clone_job(const Job *job);
Statement *clone_statement_function(const Statement_Function *fn);
Statement *clone_statement_if(const Statement_If *statement);
Statement *clone_statement_for(const Statement_For *statement);
Statement *clone_statement_job(const Statement_Job *statement);
Statement *clone_statement_while(const Statement_While *statement);


Token_Vec clone_argv(Token_Vec argv);
Job *clone_job(const Job *job);
Job *clone_job_binary(const Job_Binary *job);
Job *clone_job_unary(const Job_Unary *job);
Job *clone_job_command(const Job_Command *job);

#endif
