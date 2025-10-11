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

#define INIT_FILE_PATH "~/.config/flint/init.flint"

char *get_prompt()
{
  char *pwd = getcwd(NULL, 0);

  if (!pwd) {
    return str_format("couldn't retrive cwd > ");
  }

  char *home = str_compress_tilde(pwd);
  char *prompt = str_format("%s%s%s%s%s", Z_COLOR_MAGENTA, home, Z_COLOR_GREEN, "::dev:: ", Z_COLOR_RESET);
  free(home);
  free(pwd);

  return prompt;
}

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
  initialize_state();
  execute_file(INIT_FILE_PATH);

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
