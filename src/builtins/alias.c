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

    action_put_alias(argv[1], argv[2]);

    return 0;
}
