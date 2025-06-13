#include "alias.h"
#include "../libzatar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>

Z_MAP_DECLARE(Map, char *, char *, map)
Z_MAP_IMPLEMENT(Map, char *, char *, map)

Map alias_table = { NULL, strcmp };

void free_string(char *s)
{
    free(s);
}

const char *get_alias(Z_String_View key)
{
    char *value;
    char *key_cstr = strndup(key.ptr, key.len);

    if (map_find(&alias_table, key_cstr, &value)) {
        free(key_cstr);
        return value;
    }

    free(key_cstr);
    return NULL;
}

void add_alias(Z_String_View key, Z_String_View value)
{
    map_put(&alias_table, strndup(key.ptr, key.len), strndup(value.ptr, value.len), free_string, free_string);
}

int builtin_alias(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "alias: need 2 arguments\n");
        return 1;
    }

    Z_String_View key = Z_CSTR_TO_SV(argv[1]);
    Z_String_View value = Z_CSTR_TO_SV(argv[2]);

    add_alias(key, value);

    return 0;
}
