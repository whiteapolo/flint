#include "prompt.h"
#include "cstr.h"
#include "libzatar.h"

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
