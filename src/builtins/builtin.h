#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdbool.h>
#include "../libzatar.h"

typedef int (*BuiltinFn)(int argc, char **argv);

BuiltinFn get_builtin(const char *name);

int builtin_cd(int argc, char **argv);
int builtin_exit(int argc, char **argv);
int builtin_export(int argc, char **argv);
int builtin_let(int argc, char **argv);
int builtin_mut(int argc, char **argv);
int builtin_test(int argc, char **argv);
int builtin_len(int argc, char **argv);
int builtin_print(int argc, char **argv);
int builtin_println(int argc, char **argv);

const char *get_alias(Z_String_View key);
void add_alias(Z_String_View key, Z_String_View value);
int builtin_alias(int argc, char **argv);

#endif
