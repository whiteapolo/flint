#include <stdio.h>
#include <string.h>

int builtin_len(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: len <value>\n");
        return 1;
    }

    printf("%zu\n", strlen(argv[1]));

    return 0;
}
