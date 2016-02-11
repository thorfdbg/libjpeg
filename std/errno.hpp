
/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: errno.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef ERRNO_HPP
#define ERRNO_HPP

#include "config.h"
#if defined(HAVE_ERRNO_H)
#include <errno.h>
#else
// Must provide some dummy values
#define ENOENT -1024
#define EACCES -1025
extern int errno;
#endif

#endif
