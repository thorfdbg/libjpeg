
/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: setjmp.hpp,v 1.8 2014/09/30 08:33:18 thor Exp $
*/

#ifndef SETJMP_HPP
#define SETJMP_HPP
#include "config.h"

#ifdef DEPLOY_PICCLIB
# include "pegasus/setjmp.h"
#else
# if defined(HAVE_SETJMP_H) && defined(HAVE_LONGJMP)
#  include <setjmp.h>
# else
#  error "setjmp and longjmp are not available, won't compile without"
# endif
#endif

#endif
