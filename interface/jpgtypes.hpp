/*
 * System independent type definitions.
 * $Id: jpgtypes.hpp,v 1.9 2014/09/30 08:33:17 thor Exp $
 *
 * The following header defines basic types to be used in the JPG interface
 * routines.
 *
 * These are external definitions, visible to the outher world and part of
 * the jpeg interface.
 */


#ifndef JPGTYPES_HPP
#define JPGTYPES_HPP

#include "config.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/// Elementary types
#if defined(HAS_INT32_T) && defined(HAS_UINT32_T)
typedef int32_t           JPG_LONG;
typedef uint32_t          JPG_ULONG;
#else
# if SIZEOF_LONG == 4
typedef signed long int   JPG_LONG;    /* an 32 bit signed integer */
typedef unsigned long int JPG_ULONG;   /* an 32 bit unsigned integer */
# elif SIZEOF_INT == 4
typedef signed int        JPG_LONG;    /* an 32 bit signed integer */
typedef unsigned int      JPG_ULONG;   /* an 32 bit unsigned integer */
# else
#  error "No 32 bit integer type available"
# endif
#endif
///

// Floating point types, available by ANSI.
// Precision doesn't matter too much
typedef float             JPG_FLOAT;
///

/// Convenience boolean types
#define JPG_TRUE  (1)
#define JPG_FALSE (0)
///

/// Limits
/* Limits of the types defined above. It is rather important that
 * the specific implementation meets these limits.
 */

// Define the following only if not yet defined:
#define JPG_MIN_LONG ((JPG_LONG)(-0x80000000L))
#define JPG_MAX_LONG ((JPG_LONG)(0x7fffffffL))

#define JPG_MIN_ULONG ((JPG_ULONG)(0x00000000UL))
#define JPG_MAX_ULONG ((JPG_ULONG)(0xffffffffUL))
///

/// Pointers
// The next one defines a "generic" pointer ("A PoiNTeR").
#ifndef JPG_NOVOIDPTR
typedef void *JPG_APTR;
typedef const void *JPG_CPTR;
#else
#define JPG_APTR void *
#define JPG_CPTR const void *
#endif
///

///
#endif

