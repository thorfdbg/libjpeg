
/*
** This is an Os abstraction of the ctype
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: ctype.hpp,v 1.7 2014/09/30 08:33:17 thor Exp $
*/

#ifndef CTYPE_HPP
#define CTYPE_HPP
#include "config.h"

#if defined(HAVE_CTYPE_H) && defined(HAVE_ISSPACE)
#include <ctype.h>
#else
extern int isspace(int c);
#endif

#endif
