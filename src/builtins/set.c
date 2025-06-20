#include "export.h"
#include "../environment.h"
#include <stdio.h>
#include <stdlib.h>

int builtin_set(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: set <variable_name> <value>\n");
        return 1;
    }

    extern Environment environment;
    environment_set(&environment, Z_CSTR_TO_SV(argv[1]), Z_CSTR_TO_SV(argv[2]));

    return 0;
}
