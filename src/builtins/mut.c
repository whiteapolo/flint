#include <stdio.h>
#include <stdlib.h>
#include "../state.h"

int builtin_mut(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: mut <variable_name> <value>\n");
        return 1;
    }

    const char *name = argv[1];
    const char *value = argv[2];

    if (!action_mutate_variable(name, value)) {
        fprintf(stderr, "Flint: mut: variable '%s' doesn't exists\n", name);
        fprintf(stderr, "Flint: declare like this: let %s \"%s\"\n", name, value);
        return 1;
    }

    return 0;
}
