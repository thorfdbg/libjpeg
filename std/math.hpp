
/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: math.hpp,v 1.8 2014/09/30 08:33:18 thor Exp $
*/

#ifndef MATH_HPP
#define MATH_HPP

#if HAVE_MATH_H
#include <math.h>
#ifdef DEPLOY_PICCLIB
# if defined(__cplusplus)
extern "C" {
# endif
# include "pegasus/picclib.h"
# if defined(__cplusplus)
}
# endif
#endif

// Check or add isnan and/or isinf
#ifndef isnan
# define isnan(x) ((x) != (x))
# ifdef _MSC_VER
#  define nan(x)   ((HUGE_VAL+HUGE_VAL)-(HUGE_VAL+HUGE_VAL))
# endif
#endif
#ifndef isinf
# define isinf(x) (!isnan(x) && (isnan((x) - (x))))
#endif

#else
#error "math.h header not available, won't compile without"
#endif
#endif

