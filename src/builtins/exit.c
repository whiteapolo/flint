#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

bool is_number(const char *s)
{
    const char *curr = s;

    while (*curr) {
        if (!isdigit(*(curr++))) {
            return false;
        }
    }

    return true;
}

int builtin_exit(int argc, char **argv)
{
    if (argc == 1) {
        exit(0);
    } else if (argc == 2) {
        if (!is_number(argv[1])) {
            fprintf(stderr, "Argument must be a number\n");
            exit(255);
        }

        exit(atoi(argv[1]));
    } else {
        fprintf(stderr, "Too many arguments\n");
        exit(1);
    }
}
