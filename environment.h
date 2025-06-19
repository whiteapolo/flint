#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "libzatar.h"

Z_MAP_DECLARE(Var_Map, char *, char *, var_map)

typedef struct Environment {
    struct Environment *parent;
    Var_Map values;
} Environment;

Z_String_View environment_get(const Environment *environment, Z_String_View name);
void environment_set(Environment *environment, Z_String_View name, Z_String_View value);

#endif
