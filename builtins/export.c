#include "export.h"
#include <stdio.h>
#include <stdlib.h>

int builtin_export(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: export <variable_name> <value>\n");
        return 1;
    }

    return setenv(argv[1], argv[2], 1);
}
