#include "state.h"
#include "libzatar.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

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
  z_map_free(&scope.functions, free, (Z_Free_Fn)free_function_statement);
}

void initialize_state()
{
  action_push_scope();
  state.alias = (Z_Map){.compare_keys = (Z_Compare_Fn)strcmp};
}

bool action_mutate_variable(const char *name, const char *value)
{
  z_da_foreach_reversed(Scope *, scope, &state.scopes) {
    if (z_map_get(&scope->variables, name)) {
      z_map_put(&scope->variables, strdup(name), strdup(value), free, free);
      return true;
    }
  }

  return false;
}

void action_create_variable(const char *name, const char *value)
{
  z_map_put(&z_da_peek(&state.scopes).variables, strdup(name), strdup(value), free, free);
}

void action_create_fuction(const char *name, const Statement_Function *fn)
{
  z_map_put(&z_da_peek(&state.scopes).functions, strdup(name), dup_statement_function(fn), free, (Z_Free_Fn)free_function_statement);
}

void action_put_alias(const char *key, const char *value)
{
  z_map_put(&state.alias, strdup(key), strdup(value), free, free);
}

const char *select_variable(const char *name)
{
  z_da_foreach_reversed(Scope *, scope, &state.scopes) {
    void *value = z_map_get(&scope->variables, name);
    if (value) {
      return value;
    }
  }

  return "";
}

const Statement_Function *select_function(const char *name)
{
  z_da_foreach_reversed(Scope *, scope, &state.scopes) {
    void *fn = z_map_get(&scope->functions, name);
    if (fn) {
      return fn;
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
