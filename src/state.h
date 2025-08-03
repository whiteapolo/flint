#ifndef STATE_H
#define STATE_H

#include "libzatar.h"
#include "parser.h"

typedef struct {
  Z_Map variables;
  Z_Map functions;
} Environment;

typedef struct {
  Environment *ptr;
  int len;
  int capacity;
} Environment_Vec;

typedef struct {
  Environment_Vec env;
  Z_String buf;
  Z_Map alias;
} State;

void initialize_state();

// actions that change the state
bool action_mutate_variable(const char *name, const char *value);
void action_create_variable(Z_String_View name, Z_String_View value);
void action_create_fuction(Z_String_View name, Statement_Function *fn);
void action_push_environment();
void action_pop_environment();
void action_put_alias(Z_String_View key, Z_String_View value);

// selectors that don't change the state
const char *select_variable(Z_String_View name);
Statement_Function *select_function(Z_String_View name);
const char *select_alias(Z_String_View name);

#endif
