#include "state.h"
#include "libzatar.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

typedef void (*Free_Fn)(void *);

State state = {0};

Scope new_scope()
{
  Scope scope = {
      .variables = {.compare_keys = (Z_Compare_Fn)strcmp},
      .functions = {.compare_keys = (Z_Compare_Fn)strcmp},
  };

  return scope;
}

void free_scope(Scope scope)
{
  z_map_free(&scope.variables, free, free);
  z_map_free(&scope.functions, free, (Free_Fn)free_function_statement);
}

void initialize_state()
{
  action_push_scope();
  state.alias = (Z_Map){.compare_keys = (Z_Compare_Fn)strcmp};
}

bool action_mutate_variable(const char *name, const char *value)
{
  for (int i = state.scopes.len - 1; i >= 0; i++) {
    if (z_map_get(&state.scopes.ptr[i].variables, name)) {
      z_map_put(&state.scopes.ptr[i].variables, strdup(name), strdup(value), free, free);
      return true;
    }
  }

  return false;
}

void action_create_variable(Z_String_View name, Z_String_View value)
{
  z_map_put(&z_da_peek(&state.scopes).variables, strndup(name.ptr, name.len), strndup(value.ptr, value.len), free, free);
}

void action_create_fuction(Z_String_View name, Statement_Function *fn)
{
  z_map_put(&z_da_peek(&state.scopes).functions, strndup(name.ptr, name.len), fn, free, (Free_Fn)free_function_statement);
}

void action_put_alias(Z_String_View key, Z_String_View value)
{
  z_map_put(&state.alias, strndup(key.ptr, key.len), strndup(value.ptr, value.len), free, free);
}

const char *select_variable(const char *name)
{
  for (int i = state.scopes.len - 1; i >= 0; i--) {
    void *tmp = z_map_get(&state.scopes.ptr[i].variables, name);
    if (tmp) {
      return tmp;
    }
  }

  return "";
}

Statement_Function *select_function(const char *name)
{
  for (int i = state.scopes.len - 1; i >= 0; i--) {
    void *fn = z_map_get(&state.scopes.ptr[i].functions, name);
    if (fn) {
      return (Statement_Function *)fn;
    }
  }

  return NULL;
}

const char *select_alias(const char *name)
{
  return z_map_get(&state.alias, name);
}

void action_push_scope()
{
  z_da_append(&state.scopes, new_scope());
}

void action_pop_scope()
{
  Scope scope = z_da_pop(&state.scopes);
  free_scope(scope);
}
