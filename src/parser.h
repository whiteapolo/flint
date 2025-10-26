#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "libzatar.h"
#include "token.h"
#include "ast.h"
#include <stdbool.h>

Statement_Array parse(const Token_Array *t, const char *_source);

#endif
