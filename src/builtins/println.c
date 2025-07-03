#include <stdio.h>

int builtin_println(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: println <string>\n");
        return 1;
    }

    printf("%s\n", argv[1]);

    return 0;
}
