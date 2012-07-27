/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/
/*
 * System independent type definitions.
 * $Id: jpgtypes.hpp,v 1.3 2012-06-02 10:27:14 thor Exp $
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

