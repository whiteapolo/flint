#include <ctype.h>
#include <linux/limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <unistd.h>

#include "ast.c"
#include "config.c"
#include "error.c"
#include "eval.c"
#include "expantion.c"
#include "interpreter.c"
#include "lexer.c"
#include "parser.c"
#include "print_ast.c"
#include "prompt.c"
#include "state.c"
#include "token.c"
#include "builtins/alias.c"
#include "builtins/builtin.c"
#include "builtins/cd.c"
#include "builtins/command.c"
#include "builtins/exit.c"
#include "builtins/export.c"
#include "builtins/len.c"
#include "builtins/let.c"
#include "builtins/mut.c"
#include "builtins/print.c"
#include "builtins/println.c"
#include "builtins/test.c"
#include "builtins/time.c"

void repl()
{
  char *prompt = get_prompt();

  for (char *line = readline(prompt); line; line = readline(prompt)) {
    add_history(line);
    interpret(line);
    free(line);
    free(prompt);
    prompt = get_prompt();
  }

  free(prompt);
}

void execute_file(const char *pathname)
{
  char *expanded_path = str_expand_tilde(pathname);
  char *content = str_read_file(expanded_path);
  free(expanded_path);

  if (!content) {
    z_print_warning("Flint: No such file or directory: '%s'", pathname);
    return;
  }

  interpret(content);
  free(content);
}

int main(int argc, char **argv)
{
  initialize_config(argc, argv);
  initialize_state();
  execute_file(get_config()->init_file_path);

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    execute_file(argv[1]);
  } else {
    z_die_format("Flint: Usage: Flint <path>\n");
  }
}

#define LIBZATAR_IMPLEMENTATION
#include "libzatar.h"

#define CSTR_IMPLEMENTATION
#include "cstr.h"