#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

static bool is_number(const char *s)
{
  if (strlen(s) == 0) {
    return false;
  }

  for (int i = 0; s[i]; i++) {
    if (!isdigit(s[i])) {
      return false;
    }
  }

  return true;
}

int builtin_exit(int argc, char **argv)
{
  if (argc == 1) {
    exit(0);
  }

  if (argc == 2) {
    if (!is_number(argv[1])) {
      fprintf(stderr, "Argument must be a number\n");
      exit(255);
    }

    exit(atoi(argv[1]));
  }

  fprintf(stderr, "Too many arguments\n");
  exit(1);
}
