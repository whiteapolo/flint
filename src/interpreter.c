#include "interpreter.h"
#include "config.h"
#include "eval.h"
#include "expantion.h"
#include "libzatar.h"
#include "parser.h"
#include "print_ast.h"
#include "token.h"
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void interpret(const char *source)
{
  const Flint_Config *config = get_config();

  Token_Array tokens = lexer_get_tokens(Z_CSTR(source));
  expand_aliases(&tokens);
  if (config->log_tokens) print_tokens(&tokens);
  Statement_Array statements = parse(&tokens, source);
  if (config->log_statements) print_statements(statements);
  free_tokens(&tokens);
  evaluate_statements(statements);
  free_statements(&statements);
}

void interpret_to(Z_String_View source, Z_String *output)
{
  int fd[2];
  pipe(fd);

  int saved_stdout = dup(STDOUT_FILENO);
  dup2(fd[1], STDOUT_FILENO);
  close(fd[1]);

  char *_source = strndup(source.ptr, source.len);
  interpret(_source);
  free(_source);

  fflush(stdout);

  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);

  FILE *fp = fdopen(fd[0], "r");
  char buf[BUFSIZ];

  while (fgets(buf, BUFSIZ, fp)) {
    z_str_append_format(output, "%s", buf);
  }

  if (output->len > 0 && z_sv_top_char(Z_STR(*output)) == '\n') {
    z_str_pop_char(output);
  }

  fclose(fp);
}
