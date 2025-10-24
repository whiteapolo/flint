#ifndef EVAL_H
#define EVAL_H

#include "parser.h"

void evaluate_statements(Statement_Array statements);
int exec_command(char **argv);

#endif
