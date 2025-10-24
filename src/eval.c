#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins/builtin.h"
#include "eval.h"
#include "expantion.h"
#include "libzatar.h"
#include "parser.h"
#include "state.h"
#include "token.h"
#include "cstr.h"

int evaluate_job(Job *job);
void evaluate_block(Statement_Array statements);

int safe_fork()
{
  int pid = fork();

  if (pid < 0) {
    fprintf(stderr, "Fork failed: %s\n", strerror(errno));
    fprintf(stderr, "Exit...\n");
    exit(1);
  }

  return pid;
}

void safe_execvp(const char *file, char *const argv[])
{
  execvp(file, argv);
  fprintf(stderr, "'%s': %s\n", file, strerror(errno));
  exit(1);
}

void set_last_status_code(int status)
{
  char *s = str_format("%d", status);
  setenv("?", s, 1);
  free(s);
}

void initialize_function_arguments(char **argv)
{
  Z_String all = z_str_join(argv, " ");
  action_create_variable("@", z_str_to_cstr(&all));
  z_str_free(&all);

  Z_String name = {0};

  for (int i = 1; argv[i]; i++) {
    z_str_reset_format(&name, "%d", i);
    action_create_variable(z_str_to_cstr(&name), argv[i]);
  }

  z_str_free(&name);
}

void call_function(const Statement_Function *f, char **argv)
{
  action_push_scope();
  initialize_function_arguments(argv);
  evaluate_statements(f->body);
  action_pop_scope();
}

int exec_command(char **argv)
{
  if (argv[0] == NULL) {
    return 0;
  }

  const char *program = argv[0];

  if (select_function(program)) {
    call_function(select_function(program), argv);
    return 0;
  }

  if (get_builtin(program)) {
    return get_builtin(program)(str_array_len(argv), argv);
  }

  int status = 0;
  int pid = safe_fork();

  if (pid == 0) {
    safe_execvp(program, argv);
  } else {
    waitpid(pid, &status, 0);
  }

  set_last_status_code(status);

  return status;
}

int evaluate_command(Job_Command *job)
{
  char **argv = expand_argv(job->argv);
  int status = exec_command(argv);
  str_free_array(argv);

  return status;
}

void close_pipe(int fd[2])
{
  close(fd[0]);
  close(fd[1]);
}

int evaluate_pipe(Job_Binary *job)
{
  Job_Command *command = (Job_Command *)job->left;
  Job *right = job->right;

  int fd[2];
  pipe(fd);

  int pid1 = safe_fork();

  if (pid1 == 0) {
    dup2(fd[1], STDOUT_FILENO);
    close_pipe(fd);
    exit(evaluate_command(command));
  }

  int pid2 = safe_fork();

  if (pid2 == 0) {
    dup2(fd[0], STDIN_FILENO);
    close_pipe(fd);
    exit(evaluate_job(right));
  }

  close_pipe(fd);

  int status1;
  int status2;
  waitpid(pid1, &status1, 0);
  waitpid(pid2, &status2, 0);

  return status2;
}

int evaluate_and(Job_Binary *job)
{
  int status = evaluate_job(job->left);
  return status ? status : evaluate_job(job->right);
}

int evaluate_or(Job_Binary *job)
{
  int status = evaluate_job(job->left);
  return status ? evaluate_job(job->right) : status;
}

int evaluate_ampersand(Job_Unary *job)
{
  if (safe_fork() == 0) {
    int status = evaluate_job(job->child);

    if (status != 0) {
      fprintf(stderr, "exit with status %d\n", status);
    } else {
      printf("done\n");
    }

    exit(status);
  }

  return 0;
}

int evaluate_unary(Job_Unary *job)
{
  switch (job->operator.type) {
    case TOKEN_AMPERSAND: return evaluate_ampersand(job);
    default:
      assert(0 && "Unknown operator\n");
      return 0; // unreachable
  }
}

int evaluate_binary(Job_Binary *job)
{
  switch (job->operator.type) {
    case TOKEN_AND: return evaluate_and(job);
    case TOKEN_OR: return evaluate_or(job);
    case TOKEN_PIPE: return evaluate_pipe(job);
    default:
      assert(0 && "Unknown operator\n");
      return 0; // unreachable
  }
}

int evaluate_job(Job *job)
{
  if (job == NULL) {
    return 0;
  }

  switch (job->type) {
    case JOB_COMMAND: return evaluate_command((Job_Command *)job);
    case JOB_UNARY: return evaluate_unary((Job_Unary *)job);
    case JOB_BINARY: return evaluate_binary((Job_Binary *)job);
  }

  return 0;
}

void evaluate_block(Statement_Array statements)
{
  action_push_scope();
  evaluate_statements(statements);
  action_pop_scope();
}

void evaluate_if(Statement_If *statement)
{
  if (!evaluate_job(statement->condition)) {
    evaluate_block(statement->ifBranch);
  } else {
    evaluate_block(statement->elseBranch);
  }
}

void evaluate_while(Statement_While *statement)
{
  while (!evaluate_job(statement->condition)) {
    evaluate_block(statement->body);
  }
}

void evaluate_for(Statement_For *statement)
{
  action_push_scope();

  String_Array delim = {0};
  String_Array string = {0};
  expand_token(statement->delim, &delim);
  expand_token(statement->string, &string);

  Z_String name = z_str_new_from(Z_CSTR(statement->var_name.lexeme));
  Z_String value = {0};

  action_create_variable(z_str_to_cstr(&name), "");

  z_sv_split_cset_foreach(Z_CSTR(string.ptr[0]), Z_CSTR(delim.ptr[0]), tok) {
    value.len = 0;
    z_str_append_str(&value, tok);
    action_mutate_variable(z_str_to_cstr(&name), z_str_to_cstr(&value));
    evaluate_block(statement->body);
  }

  z_str_free(&value);
  z_str_free(&name);
  z_da_append(&delim, NULL);
  z_da_append(&string, NULL);
  str_free_array(delim.ptr);
  str_free_array(string.ptr);

  action_pop_scope();
}

void evaluate_function(Statement_Function *function)
{
  action_create_fuction(function->name.lexeme, function);
}

int evaluate_statement(Statement *statement)
{
  switch (statement->type) {
  case STATEMENT_JOB:
    return evaluate_job(((Statement_Job *)statement)->job);

  case STATEMENT_IF:
    evaluate_if(((Statement_If *)statement));
    return 0;

  case STATEMENT_WHILE:
    evaluate_while((Statement_While *)statement);
    return 0;

  case STATEMENT_FOR:
    evaluate_for((Statement_For *)statement);
    return 0;

  case STATEMENT_FUNCTION:
    evaluate_function((Statement_Function *)statement);
    return 0;

  default:
    return 0;
  }
}

void evaluate_statements(Statement_Array statements)
{
  for (int i = 0; i < statements.len; i++) {
    evaluate_statement(statements.ptr[i]);
  }
}
