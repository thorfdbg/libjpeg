/*
 * Definitions and functions required for debugging.
 * This must be turned off for final code by means of a switch.
 *
 * $Id: debug.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
 *
 */

#ifndef DEBUG_HPP

#if DEBUG_LEVEL > 0 || CHECK_LEVEL > 0
#include "std/stdio.hpp"
#endif

#include "std/assert.hpp"

#if DEBUG_LEVEL > 2
#define KP(a) a
#else
#define KP(a)
#endif

#endif
