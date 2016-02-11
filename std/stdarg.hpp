
/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: stdarg.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef STDARG_HPP
#define STDARG_HPP
#include "config.h"

#if defined(HAVE_STDARG_H)
#include <stdarg.h>
#else
#error "No stdarg.h header available, won't compile without"
#endif

#endif
