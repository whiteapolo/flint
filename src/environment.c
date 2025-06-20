#include "environment.h"
#include "libzatar.h"
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

void environment_set(Environment *environment, Z_String_View name, Z_String_View value)
{
    char *_name = strndup(name.ptr, name.len);
    char *_value = strndup(value.ptr, value.len);

    var_map_put(&environment->values, _name, _value, free_string, free_string);
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
