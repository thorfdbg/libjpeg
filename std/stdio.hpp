
/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: stdio.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef STDIO_HPP
#define STDIO_HPP
#include "config.h"

#if defined(HAVE_STDIO_H)
#include <stdio.h>
#else
#error "stdio.h not available, won't compile without"
#endif

#ifndef HAVE_VSNPRINTF
#include "std/stdarg.hpp"
extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#ifndef HAVE_SNPRINTF
#include "std/stdarg.hpp"
extern int TYPE_CDECL snprintf(char *str,size_t size,const char *format,...);
#endif

// Replace fopen by fopen64 if this is available.
#ifdef HAVE_FOPEN64
# define fopen fopen64
#endif

#endif
