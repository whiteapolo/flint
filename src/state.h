#ifndef STATE_H
#define STATE_H

#include "libzatar.h"
#include "parser.h"

typedef struct {
  Z_Map variables;
  Z_Map functions;
} Scope;

typedef struct {
  Scope *ptr;
  int len;
  int cap;
} Scope_Vec;

typedef struct {
  Scope_Vec scopes;
  Z_Map alias;
} State;

void initialize_state();

// actions that change the state
bool action_mutate_variable(const char *name, const char *value);
void action_create_variable(const char *name, const char *value);
void action_create_fuction(const char *name, const Statement_Function *fn);
void action_push_scope();
void action_pop_scope();
void action_put_alias(const char *key, const char *value);

// selectors that don't change the state
const char *select_variable(const char *name);
const Statement_Function *select_function(const char *name);
const char *select_alias(const char *name);

#endif
