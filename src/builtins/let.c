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

    const char *name = argv[1];
    const char *value = argv[2];

    action_create_variable(Z_CSTR_TO_SV(name), Z_CSTR_TO_SV(value));

    return 0;
}
