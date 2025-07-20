#include "../libzatar.h"
#include "../state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>

int builtin_alias(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: alias <key> <value>\n");
        return 1;
    }

    Z_String_View key = Z_CSTR_TO_SV(argv[1]);
    Z_String_View value = Z_CSTR_TO_SV(argv[2]);

    action_put_alias(key, value);

    return 0;
}
