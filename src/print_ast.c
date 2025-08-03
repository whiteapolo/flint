#include "print_ast.h"
#include "libzatar.h"
#include "parser.h"
#include <stdio.h>

void render_job(Job *job, Z_String *output);

void render_job_binary(Job_Binary *job, Z_String *output) {
  z_str_append_format(output, "(");
  z_str_append_str(output, Z_STR(job->operator.lexeme));
  z_str_append_format(output, " ");
  render_job(job->left, output);
  z_str_append_format(output, " ");
  render_job(job->right, output);
  z_str_append_format(output, ")");
}

void render_job_unary(Job_Unary *job, Z_String *output) {
  z_str_append_format(output, "(");
  z_str_append_str(output, Z_STR(job->operator.lexeme));
  z_str_append_format(output, " ");
  render_job(job->child, output);
  z_str_append_format(output, ")");
}

void render_job_command(Job_Command *job, Z_String *output) {
  z_str_append_format(output, "(");

  z_str_append_str(output, Z_STR(job->argv.ptr[0].lexeme));

  for (int i = 1; i < job->argv.len; i++) {
    Token token = job->argv.ptr[i];
    z_str_append_format(output, " \"%.*s\"", token.lexeme.len,
                        token.lexeme.ptr);
  }

  z_str_append_format(output, ")");
}

void render_job(Job *job, Z_String *output) {
  if (job == NULL) {
    return;
  } else if (job->type == JOB_BINARY) {
    render_job_binary((Job_Binary *)job, output);
  } else if (job->type == JOB_UNARY) {
    render_job_unary((Job_Unary *)job, output);
  } else if (job->type == JOB_COMMAND) {
    render_job_command((Job_Command *)job, output);
  } else {
    z_str_append_format(output, "(Unknown)");
  }
}

void print_job(Job *job) {
  Z_String s = {0};
  render_job(job, &s);

  if (s.len > 0) {
    z_str_println(Z_STR(s));
  }

  free(s.ptr);
}

void print_statement_job(Statement_Job *statement) {
  print_job(statement->job);
}

void print_statement_if(Statement_If *statement) {
  printf("if (");
  print_job(statement->condition);
  printf(")");
  print_statements(statement->ifBranch);
}

void print_statement_while(Statement_While *statement) {
  printf("while (");
  print_job(statement->condition);
  printf(")\n");
  print_statements(statement->body);
}

void print_statement_for(Statement_For *statement) {
  printf("for (\"");
  z_str_print(Z_STR(statement->string.lexeme));
  printf("\" \"");
  z_str_print(Z_STR(statement->delim.lexeme));
  printf("\")\n");
  print_statements(statement->body);
}

void print_statement_function(Statement_Function *statement) {
  z_str_print(Z_STR(statement->name.lexeme));
  printf("() {\n");
  print_statements(statement->body);
  printf("}\n");
}

void print_statement(Statement *statement) {
  switch (statement->type) {
  case STATEMENT_JOB:
    print_statement_job((Statement_Job *)statement);
    break;

  case STATEMENT_IF:
    print_statement_if((Statement_If *)statement);
    break;

  case STATEMENT_WHILE:
    print_statement_while((Statement_While *)statement);
    break;

  case STATEMENT_FOR:
    print_statement_for((Statement_For *)statement);
    break;

  case STATEMENT_FUNCTION:
    print_statement_function((Statement_Function *)statement);
    break;
  }
}

void print_statements(Statement_Vec statements) {
  for (int i = 0; i < statements.len; i++) {
    print_statement(statements.ptr[i]);
  }
}
