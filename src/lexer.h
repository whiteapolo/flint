#ifndef LEXEL_H
#define LEXEL_H

#include "libzatar.h"
#include "token.h"

Token_Array lexer_get_tokens(Z_String_View source);

#endif
