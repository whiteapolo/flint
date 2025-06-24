#include "builtin.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int safe_fork();

typedef struct {
    const char *name;
    BuiltinFn function;
} Builtin;

static Builtin builtins[] = {
    { .name = "cd", .function = builtin_cd },
    { .name = "exit", .function = builtin_exit },
    { .name = "alias", .function = builtin_alias },
    { .name = "export", .function = builtin_export },
    { .name = "mut", .function = builtin_mut },
    { .name = "let", .function = builtin_let },
};

BuiltinFn get_builtin(const char *name)
{
    for (int i = 0; i < (int)(sizeof(builtins) / sizeof(builtins[0])); i++) {
        if (strcmp(builtins[i].name, name) == 0) {
            return builtins[i].function;
        }
    }

    return NULL;
}

