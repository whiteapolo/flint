#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include "ast.h"
#include <stdbool.h>

Statement_Vec parse(const Token_Vec *t, const char *_source);
void parser_free(Statement_Vec *node);
void free_function_statement(Statement_Function *statement);

#endif
