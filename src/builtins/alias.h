#ifndef ALIAS_H
#define ALIAS_H

#include "../libzatar.h"

const char *get_alias(Z_String_View key);
void add_alias(Z_String_View key, Z_String_View value);
int builtin_alias(int argc, char **argv);

#endif
