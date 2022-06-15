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
** Type definition: Some system independent type definitions
** (thor's peculiarities)
** $Id: types.hpp,v 1.9 2022/06/14 06:18:30 thor Exp $
**
** The following header defines basic types to be used in the JPG interface
** routines. Especially, this file must be adapted if your compiler has
** different ideas what a "unsigned long" is as we *MUST* fix the width
** of elementary data types. Especially, do not use types not defined here for
** interface glue routines.
**
** This is the "internal" header file defining internal types, importing the
** types from the external "j2ktypes" header.
*/


#ifndef TYPES_HPP
#define TYPES_HPP

#include "config.h"
#include "jpgtypes.hpp"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/// Elementary types
#if defined(HAS_INT8_T) && defined(HAS_UINT8_T)
typedef int8_t              BYTE;
typedef uint8_t             UBYTE;
#else
# if SIZEOF_CHAR != 1
#  error "No 8 bit type available"
# endif
typedef signed char         BYTE;    /* an 8 bit signed integer */
typedef unsigned char       UBYTE;   /* an 8 bit unsigned integer */
#endif

#if defined(HAS_INT16_T) && defined(HAS_UINT16_T)
typedef int16_t             WORD;
typedef uint16_t            UWORD;
#else
# if SIZEOF_SHORT == 2
typedef signed short int    WORD;    /* an 16 bit signed integer */
typedef unsigned short int  UWORD;   /* an 16 bit unsigned integer */
# elif SIZEOF_INT == 2
typedef signed int          WORD;    /* an 16 bit signed integer */
typedef unsigned int        UWORD;   /* an 16 bit unsigned integer */
# else
#  error "No 16 bit type available"
# endif
#endif

// The following are external types.
typedef JPG_LONG            LONG;    /* an 32 bit signed long */
typedef JPG_ULONG           ULONG;   /* an 32 bit unsigned long */

typedef LONG                FIXED;   /* a fixed point number in format 27.5       */
typedef LONG                QIXED;   /* a fixed point number in format 14.18      */
typedef WORD                SIXED;   /* a short fixed point number in format 11.5 */

// Floating point types, available by ANSI.
// Precision doesn't matter too much
typedef JPG_FLOAT           FLOAT;
typedef double              DOUBLE;
///

/// Quads

// The following is not available on every compiler.
// They might be called differently on your machine, hence you might
// have to change these...
#if defined(HAS_INT64_T) && defined(HAS_UINT64_T)
typedef int64_t             QUAD;
typedef uint64_t            UQUAD;
# define HAVE_QUAD
# if SIZEOF_LONG == 8
#  ifndef IS_64BIT_CODE
#   define IS_64BIT_CODE
#  endif
# endif
#else
# if SIZEOF_LONG == 8
typedef signed long long    QUAD;    /* an 64 bit signed long */
typedef unsigned long long  UQUAD;   /* an 64 bit unsigned long */
#  define HAVE_QUAD
#  ifndef IS_64BIT_CODE
#   define IS_64BIT_CODE
#  endif
# else
#  if defined(SIZEOF___INT64)
#   if SIZEOF___INT64 == 8
typedef __int64             QUAD;
typedef unsigned __int64    UQUAD;
#    define HAVE_QUAD
#   endif
#  endif
#  ifndef HAVE_QUAD
#   if defined(SIZEOF_LONG_LONG)
#    if SIZEOF_LONG_LONG == 8
typedef signed long long    QUAD;    /* an 64 bit signed long */
typedef unsigned long long  UQUAD;   /* an 64 bit unsigned long */
#     define HAVE_QUAD
#    endif
#   endif
#  endif
# endif
#endif
#ifndef HAVE_QUAD
# error "No 64 bit integer available"
#endif
///

/// Boolean values
/* boolean values for convenience */
#ifndef TRUE
#define TRUE JPG_TRUE
#endif

#ifndef FALSE
#define FALSE JPG_FALSE
#endif
///

/// Limits
// Limits of the types defined above. It is rather important that
// the specific implementation meets these limits.
//

#define MIN_BYTE -0x80
#define MAX_BYTE 0x7f

#define MIN_UBYTE 0x00
#define MAX_UBYTE 0xff

#define MIN_WORD -0x8000
#define MAX_WORD 0x7fff

#define MIN_UWORD 0x0000
#define MAX_UWORD 0xffff

// Define the following only if not yet defined:
#ifndef MIN_LONG
# define MIN_LONG JPG_MIN_LONG
#endif
#ifndef MAX_LONG
# define MAX_LONG JPG_MAX_LONG
#endif

#ifndef MIN_ULONG
# define MIN_ULONG JPG_MIN_ULONG
#endif
#ifndef MAX_ULONG
# define MAX_ULONG JPG_MAX_ULONG
#endif

// The next two are really adventurous...
// Yuck!
#define MIN_UQUAD 0x0
#define MAX_UQUAD ((UQUAD)((QUAD)(-1L)))

#define MAX_QUAD  ((QUAD)((MAX_UQUAD)>>1))
#define MIN_QUAD  (-MAX_QUAD-1)

// HUGE_VAL is ANSI C and should be defined in
// std/math.hpp, or what is included by it.
#define MAX_DOUBLE HUGE_VAL
///

/// APTR
// The next one defines a "generic" pointer ("A PoiNTeR")
#ifndef JPG_NOVOIDPTR
typedef JPG_APTR APTR;
typedef JPG_CPTR CPTR;
#else
# define APTR JPG_APTR
# define CPTR JPG_CPTR
#endif

#ifndef NULL
# ifdef HAS__NULL_TYPE
#  define NULL (__null)
# else
#  define NULL (0)
# endif
#endif
///

/// Aliasing types
// The following types are copies of the elementary data types that allow
// aliasing for the "hacky" conversion routines
#ifdef HAS_MAY_ALIAS
typedef UBYTE  __attribute((may_alias)) UBYTE_ALIASED;
typedef BYTE   __attribute((may_alias)) BYTE_ALIASED;
typedef UWORD  __attribute((may_alias)) UWORD_ALIASED;
typedef WORD   __attribute((may_alias)) WORD_ALIASED;
typedef ULONG  __attribute((may_alias)) ULONG_ALIASED;
typedef LONG   __attribute((may_alias)) LONG_ALIASED;
typedef UQUAD  __attribute((may_alias)) UQUAD_ALIASED;
typedef QUAD   __attribute((may_alias)) QUAD_ALIASED;
typedef FLOAT  __attribute((may_alias)) FLOAT_ALIASED;
typedef DOUBLE __attribute((may_alias)) DOUBLE_ALIASED;
#else
typedef UBYTE  UBYTE_ALIASED;
typedef BYTE   BYTE_ALIASED;
typedef UWORD  UWORD_ALIASED;
typedef WORD   WORD_ALIASED;
typedef ULONG  ULONG_ALIASED;
typedef LONG   LONG_ALIASED;
typedef UQUAD  UQUAD_ALIASED;
typedef QUAD   QUAD_ALIASED;
typedef FLOAT  FLOAT_ALIASED;
typedef DOUBLE DOUBLE_ALIASED;
#endif
///

///
#endif
