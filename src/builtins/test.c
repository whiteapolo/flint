#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool is_number(const char *s)
{
    if (!*s) return false;

    char *endptr;
    strtod(s, &endptr);

    return s != endptr && *endptr == '\0';
}

static bool compare(int cmp, const char *operator)
{
    if (!strcmp(operator, "=="))  return cmp == 0;
    if (!strcmp(operator, "!="))  return cmp != 0;
    if (!strcmp(operator, "<" ))  return cmp < 0;
    if (!strcmp(operator, ">" ))  return cmp > 0;
    if (!strcmp(operator, "<="))  return cmp <= 0;
    if (!strcmp(operator, ">="))  return cmp >= 0;

    fprintf(stderr, "Unknown operator: '%s'\n", operator);

    return 0;
}

int builtin_test(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: test <string | number> <operator> <string | number>\n");
        return 1;
    }

    const char *a = argv[1];
    const char *operator = argv[2];
    const char *b = argv[3];

    bool is_a_number = is_number(a);
    bool is_b_number = is_number(b);

    if (is_a_number && is_b_number) {
        return !compare(strtod(a, NULL) - strtod(b, NULL), operator);
    } else if (is_a_number || is_b_number) {
        fprintf(stderr, "Both operands needs to be in the same type: string | number\n");
        return 1;
    } else {
        return !compare(strcmp(a, b), operator);
    }
}
