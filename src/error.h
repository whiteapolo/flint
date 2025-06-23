#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include "libzatar.h"
#include "token.h"

void syntax_error(const char *fmt, ...);
void syntax_error_va(const char *fmt, va_list ap);
void syntax_error_at_token(Z_String_View source, Token token, const char *fmt, ...);
void syntax_error_at_token_va(Z_String_View source, Token token, const char *fmt, va_list ap);

#endif
