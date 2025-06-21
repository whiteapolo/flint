#include "export.h"
#include "../environment.h"
#include <stdio.h>
#include <stdlib.h>

int builtin_mut(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: mut <variable_name> <value>\n");
        return 1;
    }

    extern Environment environment;

    const char *name = argv[1];
    const char *value = argv[2];

    int ret = environment_mut_variable(&environment, name, value);

    if (ret) {
        fprintf(stderr, "Flint: mut: variable '%s' doesn't exists\n", name);
        fprintf(stderr, "Flint: declare like this: let %s \"%s\"\n", name, value);
        return 1;
    }

    return 0;
}
