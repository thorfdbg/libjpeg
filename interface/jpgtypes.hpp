/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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

