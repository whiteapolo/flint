#include "environment.h"
#include "libzatar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Z_MAP_IMPLEMENT(Var_Map, char *, char *, var_map)

static void free_string(char *s)
{
    free(s);
}

Environment environment_new(Environment *parent)
{
    Environment environment = {
        .parent = parent,
        .values = { .root = NULL, .cmp_keys = strcmp },
    };

    return environment;
}

int environment_mut_variable(Environment *environment, const char *name, const char *value)
{
    if (environment == NULL) {
        return 1;
    }

    char *tmp;

    if (var_map_find(&environment->values, name, &tmp)) {
        var_map_put(&environment->values, strdup(name), strdup(value), free_string, free_string);
        return 0;
    }

    return environment_mut_variable(environment->parent, name, value);
}

void environment_create_variable(Environment *environment, const char *name, const char *value)
{
    var_map_put(&environment->values, strdup(name), strdup(value), free_string, free_string);
}

const char *environment_get_cstr(const Environment *environment, const char *name)
{
    if (environment == NULL) {
        char *value = getenv(name);
        return value ? value : "";
    }

    char *value;

    if (var_map_find(&environment->values, name, &value)) {
        return value;
    }

    return environment_get_cstr(environment->parent, name);
}

Z_String_View environment_get(const Environment *environment, Z_String_View name)
{
    char *_name = strndup(name.ptr, name.len);
    const char *value = environment_get_cstr(environment, _name);
    free(_name);

    return Z_CSTR_TO_SV(value);
}

void environment_free(Environment *environment)
{
    var_map_free(&environment->values, free_string, free_string);
}
