
/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: stdlib.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef STDLIB_HPP
#define STDLIB_HPP
#include "config.h"

#ifdef DEPLOY_PICCLIB
# if defined(__cplusplus)
extern "C" {
# endif
# if defined(__IS_powerpc__)
#  include <stdlib.h>
# endif
# include "pegasus/picclib.h"
# if defined(__cplusplus)
}
# endif
#else
# if STDC_HEADERS
#  include <stdlib.h>
#  include <stddef.h>
# else
#  if HAVE_STDLIB_H
#   include <stdlib.h>
#  endif
# endif
#endif

#if !defined(HAVE_STRTOL)
#error "strtol not available, won't compile without"
#endif
#if !defined(HAVE_STRTOD)
#error "strtod not available, won't compile without"
#endif

#endif
