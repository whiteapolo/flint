#include "state.h"
#include "libzatar.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

typedef void (*FreeFn)(void *);

State state = {0};

Environment environment_new()
{
  Environment environment = {
      .variables = {.compare_keys = (Z_Compare_Fn)strcmp},
      .functions = {.compare_keys = (Z_Compare_Fn)strcmp},
  };

  return environment;
}

void initialize_state()
{
  action_push_environment();
  state.alias = (Z_Map){.compare_keys = (Z_Compare_Fn)strcmp};
  state.buf = (Z_String){0};
}

bool action_mutate_variable(const char *name, const char *value)
{
  for (int i = state.env.len - 1; i >= 0; i++) {
    if (z_map_get(&state.env.ptr[i].variables, name)) {
      z_map_put(&state.env.ptr[i].variables, strdup(name), strdup(value), free,
                free);
      return true;
    }
  }

  return false;
}

void action_create_variable(Z_String_View name, Z_String_View value)
{
  char *_name = strndup(name.ptr, name.len);
  char *_value = strndup(value.ptr, value.len);
  z_map_put(&z_da_peek(&state.env).variables, _name, _value, free, free);
}

void action_create_fuction(Z_String_View name, Statement_Function *fn)
{
  z_map_put(&z_da_peek(&state.env).functions, strndup(name.ptr, name.len), fn, free, (FreeFn)free_function_statement);
}

void action_put_alias(Z_String_View key, Z_String_View value)
{
  char *_key = strndup(key.ptr, key.len);
  char *_value = strndup(value.ptr, value.len);

  z_map_put(&state.alias, _key, _value, free, free);
}

const char *select_variable(const char *name)
{
  for (int i = state.env.len - 1; i >= 0; i--) {
    void *tmp = z_map_get(&state.env.ptr[i].variables, name);
    if (tmp) {
      return tmp;
    }
  }

  return "";
}

Statement_Function *select_function(Z_String_View name)
{
  z_str_clear(&state.buf);
  z_str_append_str(&state.buf, name);

  for (int i = state.env.len - 1; i >= 0; i--) {
    void *fn = z_map_get(&state.env.ptr[i].functions, z_str_to_cstr(&state.buf));
    if (fn) {
      return (Statement_Function *)fn;
    }
  }

  return NULL;
}

const char *select_alias(Z_String_View name)
{
  z_str_clear(&state.buf);
  z_str_append_str(&state.buf, name);

  void *alias = z_map_get(&state.alias, z_str_to_cstr(&state.buf));
  if (alias) {
    return alias;
  }

  return NULL;
}

void action_push_environment()
{
  z_da_append(&state.env, environment_new());
}

void action_pop_environment()
{
  Environment env = z_da_pop(&state.env);
  z_map_free(&env.variables, free, free);
  z_map_free(&env.functions, free, (FreeFn)free_function_statement);
}
