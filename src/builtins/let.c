#include "../environment.h"
#include <stdio.h>
#include <stdlib.h>

int builtin_let(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: let <variable_name> <value>\n");
        return 1;
    }

    extern Environment environment;

    const char *name = argv[1];
    const char *value = argv[2];

    environment_create_variable(&environment, name, value);

    return 0;
}
