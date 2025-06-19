#include "cd.h"
#include "builtin.h"
#include "exit.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "alias.h"
#include "export.h"

int safe_fork();

typedef int (*function_t)(int argc, char **argv);

typedef struct {
    const char *name;
    function_t function;
} Builtin;

static Builtin builtins[] = {
    { .name = "cd", .function = builtin_cd },
    { .name = "exit", .function = builtin_exit },
    { .name = "alias", .function = builtin_alias },
    { .name = "export", .function = builtin_export },
};

function_t find_function(const char *name)
{
    for (int i = 0; i < (int)(sizeof(builtins) / sizeof(builtins[0])); i++) {
        if (strcmp(builtins[i].name, name) == 0) {
            return builtins[i].function;
        }
    }

    return NULL;
}

bool is_builtin(const char *name)
{
    return find_function(name) != NULL;
}

int count_argc(char **argv)
{
    int i = 0;

    while (argv[i] != NULL) {
        i++;
    }

    return i;
}

int execute_builtin(char **argv)
{
    return find_function(argv[0])(count_argc(argv), argv);
}
