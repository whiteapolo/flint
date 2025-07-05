#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "libzatar.h"
#include "parser.h"

Z_MAP_DECLARE(Var_Map, char *, char *, var_map)
Z_MAP_DECLARE(Function_Map, Z_String_View, Statement_Function*, function_map)

typedef struct Environment {
    struct Environment *parent;
    Var_Map values;
    Function_Map functions;
} Environment;

Z_String_View environment_get(const Environment *environment, Z_String_View name);
int environment_mut_variable(Environment *environment, const char *name, const char *value);
void environment_create_variable(Environment *environment, const char *name, const char *value);
Environment environment_new(Environment *parent);
void environment_free(Environment *environment);
void environment_create_function(Environment *environment, Z_String_View name, Statement_Function *fuction);
Statement_Function *environment_get_function(const Environment *environment, Z_String_View name);

#endif
