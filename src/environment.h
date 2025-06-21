#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "libzatar.h"

Z_MAP_DECLARE(Var_Map, char *, char *, var_map)

typedef struct Environment {
    struct Environment *parent;
    Var_Map values;
} Environment;

Z_String_View environment_get(const Environment *environment, Z_String_View name);
int environment_mut_variable(Environment *environment, const char *name, const char *value);
void environment_create_variable(Environment *environment, const char *name, const char *value);
Environment environment_new(Environment *parent);

#endif
