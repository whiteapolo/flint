// #include "environment.h"
// #include "libzatar.h"
// #include "parser.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// Z_MAP_IMPLEMENT(Var_Map, char *, char *, var_map)
// Z_MAP_IMPLEMENT(Function_Map, Z_String_View, Statement_Function*, function_map)

// static void free_string(char *s)
// {
//     free(s);
// }

// Environment environment_new(Environment *parent)
// {
//     Environment environment = {
//         .parent = parent,
//         .values = { .root = NULL, .cmp_keys = strcmp },
//         .functions = { .root = NULL, .cmp_keys = z_str_compare },
//     };

//     return environment;
// }

// int environment_mut_variable(Environment *environment, const char *name, const char *value)
// {
//     if (environment == NULL) {
//         return 1;
//     }

//     char *tmp;

//     if (var_map_find(&environment->values, name, &tmp)) {
//         var_map_put(&environment->values, strdup(name), strdup(value), free_string, free_string);
//         return 0;
//     }

//     return environment_mut_variable(environment->parent, name, value);
// }

// void environment_create_variable(Environment *environment, const char *name, const char *value)
// {
//     var_map_put(&environment->values, strdup(name), strdup(value), free_string, free_string);
// }

// void environment_create_function(Environment *environment, Z_String_View name, Statement_Function *fuction)
// {
//     function_map_put(&environment->functions, name, fuction, NULL, free_function_statement);
// }

// const char *environment_get_cstr(const Environment *environment, const char *name)
// {
//     if (environment == NULL) {
//         char *value = getenv(name);
//         return value ? value : "";
//     }

//     char *value;

//     if (var_map_find(&environment->values, name, &value)) {
//         return value;
//     }

//     return environment_get_cstr(environment->parent, name);
// }

// Z_String_View environment_get(const Environment *environment, Z_String_View name)
// {
//     char *_name = strndup(name.ptr, name.len);
//     const char *value = environment_get_cstr(environment, _name);
//     free(_name);

//     return Z_CSTR(value);
// }

// Statement_Function *environment_get_function(const Environment *environment, Z_String_View name)
// {
//     if (!environment) {
//         return NULL;
//     }

//     Statement_Function *ret;
//     if (function_map_find(&environment->functions, name, &ret)) {
//         return ret;
//     }

//     return environment_get_function(environment->parent, name);
// }

// void environment_free(Environment *environment)
// {
//     var_map_free(&environment->values, free_string, free_string);
//     function_map_free(&environment->functions, NULL, free_function_statement);
// }
