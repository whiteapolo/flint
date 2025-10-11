#include <stdio.h>
#include <stdlib.h>
#include "../state.h"
#include "../libzatar.h"

int builtin_let(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: let <variable_name> <value>\n");
        return 1;
    }

    action_create_variable(argv[1], argv[2]);

    return 0;
}
