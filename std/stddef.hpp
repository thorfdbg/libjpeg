
/*
** This is an Os abstraction of the stddef
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: stddef.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef STDDEF_HPP
#define STDDEF_HPP
#include "config.h"

#if defined(HAVE_STDDEF_H) && defined(HAS_PTRDIFF_T)
#include <stddef.h>
#else
typedef long ptrdiff_t;
#endif

#endif
