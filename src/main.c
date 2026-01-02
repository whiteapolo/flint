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

#include "interpreter.h"
#include "state.h"
#include "cstr.h"
#include "config.h"
#include "prompt.h"

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
