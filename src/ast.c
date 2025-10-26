#include "ast.h"
#include "token.h"
#include <stdlib.h>

Statement *create_statement_if(Job *condition, Statement_Array ifBranch, Statement_Array elseBranch)
{
  Statement_If *node = malloc(sizeof(Statement_If));
  node->type = STATEMENT_IF;
  node->condition = condition;
  node->ifBranch = ifBranch;
  node->elseBranch = elseBranch;

  return (Statement *)node;
}

Statement *create_statement_while(Job *condition, Statement_Array body)
{
  Statement_While *node = malloc(sizeof(Statement_While));
  node->type = STATEMENT_WHILE;
  node->condition = condition;
  node->body = body;

  return (Statement *)node;
}

Statement *create_statement_function(Token name, Statement_Array body)
{
  Statement_Function *node = malloc(sizeof(Statement_Function));
  node->type = STATEMENT_FUNCTION;
  node->body = body;
  node->name = name;

  return (Statement *)node;
}

Statement *create_statement_for(Token var_name, Token string, Token delim, Statement_Array body)
{
  Statement_For *node = malloc(sizeof(Statement_For));
  node->type = STATEMENT_FOR;
  node->body = body;
  node->var_name = var_name;
  node->string = string;
  node->delim = delim;

  return (Statement *)node;
}

Statement *create_statement_job(Job *job)
{
  Statement_Job *node = malloc(sizeof(Statement_Job));
  node->type = STATEMENT_JOB;
  node->job = job;

  return (Statement *)node;
}

Job *create_job_binary(Job *left, Token operator, Job * right)
{
  Job_Binary *node = malloc(sizeof(Job_Binary));
  node->type = JOB_BINARY;
  node->left = left;
  node->operator = operator;
  node->right = right;

  return (Job *)node;
}

Job *create_job_unary(Token operator, Job * child)
{
  Job_Unary *node = malloc(sizeof(Job_Unary));
  node->type = JOB_UNARY;
  node->operator = operator;
  node->child = child;

  return (Job *)node;
}

Job *create_job_command(Token_Array argv)
{
  Job_Command *node = malloc(sizeof(Job_Command));
  node->type = JOB_COMMAND;
  node->argv = argv;

  return (Job *)node;
}

void free_job_command(Job_Command *cmd)
{
  free_tokens(&cmd->argv);
  free(cmd);
}

void free_job_unary(Job_Unary *un)
{
  free_token(&un->operator);
  free_job(un->child);
  free(un);
}

void free_job_binary(Job_Binary *bin)
{
  free_token(&bin->operator);
  free_job(bin->left);
  free_job(bin->right);
  free(bin);
}

void free_job(Job *job)
{
  if (job == NULL) {
    return;
  }

  switch (job->type) {
    case JOB_COMMAND:
      free_job_command((Job_Command *)job);
      break;

    case JOB_UNARY:
      free_job_unary((Job_Unary *)job);
      break;

    case JOB_BINARY:
      free_job_binary((Job_Binary *)job);
      break;
  }
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
  free_token(&statement->name);
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

void free_statements(Statement_Array *statements)
{
  for (int i = 0; i < statements->len; i++) {
    free_statement(statements->ptr[i]);
  }

  free(statements->ptr);
}

Statement *clone_statement(Statement *statement)
{
  switch (statement->type) {
    case STATEMENT_IF:       return clone_statement_if((const Statement_If *)statement);
    case STATEMENT_JOB:      return clone_statement_job((const Statement_Job *)statement);
    case STATEMENT_FOR:      return clone_statement_for((const Statement_For *)statement);
    case STATEMENT_WHILE:    return clone_statement_while((const Statement_While *)statement);
    case STATEMENT_FUNCTION: return clone_statement_function((const Statement_Function *)statement);
    default: return NULL;
  }
}

Statement_Array clone_statements(Statement_Array statements)
{
  Statement_Array new_statements = {0};

  z_da_foreach(Statement **, statement, &statements) {
    z_da_append(&new_statements, clone_statement(*statement));
  }

  return new_statements;
}

Job *clone_job(const Job *job)
{
  switch (job->type) {
    case JOB_UNARY:   return clone_job_unary((const Job_Unary *)job);
    case JOB_BINARY:  return clone_job_binary((const Job_Binary *)job);
    case JOB_COMMAND: return clone_job_command((const Job_Command *)job);
    default: return NULL;
  }
}

Job *clone_job_binary(const Job_Binary *job)
{
  return create_job_binary(clone_job(job->left), clone_token(job->operator), clone_job(job->right));
}

Job *clone_job_unary(const Job_Unary *job)
{
  return create_job_unary(clone_token(job->operator), clone_job(job->child));
}

Job *clone_job_command(const Job_Command *job)
{
  return create_job_command(clone_tokens(job->argv));
}

Statement *clone_statement_function(const Statement_Function *fn)
{
  return create_statement_function(clone_token(fn->name), clone_statements(fn->body));
}

Statement *clone_statement_if(const Statement_If *statement)
{
  return create_statement_if(
      clone_job(statement->condition),
      clone_statements(statement->ifBranch),
      clone_statements(statement->elseBranch)
  );
}

Statement *clone_statement_for(const Statement_For *statement)
{
  return create_statement_for(
      clone_token(statement->var_name),
      clone_token(statement->string),
      clone_token(statement->delim),
      clone_statements(statement->body)
  );
}

Statement *clone_statement_job(const Statement_Job *statement)
{
  return create_statement_job(clone_job(statement->job));
}

Statement *clone_statement_while(const Statement_While *statement)
{
  return create_statement_while(clone_job(statement->condition), clone_statements(statement->body));
}
