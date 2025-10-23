#ifndef AST_H
#define AST_H

#include "token.h"

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

Job *create_binary(Job *left, Token operator, Job *right);
Job *create_unary(Token operator, Job *child);
Job *create_command(Token_Vec argv);
Statement *create_statement_if(Job *condition, Statement_Vec ifBranch, Statement_Vec elseBranch);
Statement *create_statement_while(Job *condition, Statement_Vec body);
Statement *create_statement_function(Token name, Statement_Vec body);
Statement *create_statement_for(Token var_name, Token string, Token delim, Statement_Vec body);
Statement *create_statement_job(Job *job);

void free_if_statement(Statement_If *statement);
void free_while_statement(Statement_While *statement);
void free_for_statement(Statement_For *statement);
void free_function_statement(Statement_Function *statement);
void free_job_statement(Statement_Job *statement);
void free_statement(Statement *statement);
void free_statements(Statement_Vec *statements);
void free_job_command(Job_Command *cmd);
void free_job_unary(Job_Unary *un);
void free_job_binary(Job_Binary *bin);
void free_job(Job *job);

Job *clone_job(const Job *job);
Job *clone_job_binary(const Job_Binary *job);
Job *clone_job_unary(const Job_Unary *job);
Job *clone_job_command(const Job_Command *job);
Statement *clone_statement(Statement *statement);
Statement_Vec clone_statements(Statement_Vec statements);
Statement *clone_statement_function(const Statement_Function *fn);
Statement *clone_statement_if(const Statement_If *statement);
Statement *clone_statement_for(const Statement_For *statement);
Statement *clone_statement_job(const Statement_Job *statement);
Statement *clone_statement_while(const Statement_While *statement);

#endif
