#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "libzatar.h"

void interpret(const char *source);
void interpret_to(Z_String_View source, Z_String *output);

#endif
