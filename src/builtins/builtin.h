#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdbool.h>

int execute_builtin(char **argv);
bool is_builtin(const char *name);

#endif
