#include <stdio.h>

int builtin_print(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: print <string>\n");
        return 1;
    }

    printf("%s", argv[1]);

    return 0;
}
