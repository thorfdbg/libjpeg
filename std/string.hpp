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
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: string.hpp,v 1.2 2012-06-02 10:27:14 thor Exp $
*/

#ifndef STRING_HPP
#define STRING_HPP
#include "config.h"

/// Standard headers
#ifdef DEPLOY_PICCLIB
# if defined(__cplusplus)
extern "C" {
# endif
# if defined(__IS_powerpc__)
#  include <string.h>
# endif
# include "pegasus/picclib.h"
# if defined(__cplusplus)
}
# endif
#else
# if HAVE_STRING_H
#  if !STDC_HEADERS && HAVE_MEMORY_H
#   include <memory.h>
#  endif
#  include <string.h>
# endif
# if HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
///

/// strSize: XML compliant string size computation
// Compute the length of the string in bytes (without the NUL!) for
// XML encoded strings. This can be either a UTF-8 string, or a UTF-16
// string. In the latter case, the string is expected to start with a
// byte-order mark (0xfeff for big and 0xfffe for little endian).
inline static size_t strSize(const unsigned char *data)
{
  if (((data[0] == 0xff) && (data[1] == 0xfe)) ||
      ((data[0] == 0xfe) && (data[1] == 0xff))) {
    const unsigned char *p = data;
    // Repeat until a double 0 is found. Note that we cannot
    // cast to UWORD since the alignment isn't clear.
    do {
      p += 2; // The first one isn't NUL for sure!
    } while(p[0] | p[1]);
    //
    return p - data;
  } else {
    return strlen((const char *)data);
  }
}
///

/// Provide quicker replacements for memset/memcpy/memmove
#ifndef DEPLOY_PICCLIB
# ifdef HAVE_BUILTIN_MEMSET
#  undef memset
#  define memset __builtin_memset
# endif
# ifdef HAVE_BUILTIN_MEMCPY
#  undef memcpy
#  define memcpy __builtin_memcpy
# endif
# ifdef HAVE_BUILTIN_MEMMOVE
#  undef memmove
#  define memmove __builtin_memmove
# endif
#endif
///

/// Check for memchr function and provide a replacement if we don't have it.
#if !HAVE_MEMCHR
inline static void *memchr(const void *s, int c, size_t n) throw()
{
  const unsigned char *in = (const unsigned char *)s;

  if (n) {
    do {
      if (*in == c) return const_cast<void *>(s);
      in++;
    } while(--n);
  }
  return 0;
}
#endif
///

/// Check for the memmove function and provide a replacement if we don't have it.
#if !HAVE_MEMMOVE
// This is not overly efficient, but it should at least work...
inline static void *memmove(void *dest, const void *src, size_t n) throw()
{
  unsigned char       *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;

  if (n) {
    if (s > d) {
      // Source is behind dest, copy ascending.
      do {
	*d = *s;
	d++,s++;
      } while(--n);
    } else if (s < d) {
      // Source is in front of dest, copy descending.
      s += n, d += n;
      do {
	--d,--s;
	*d = *s;
      } while(--n);
    }
  }
  return dest;
}
#endif
///

/// Check for the memset function and provide a replacement if we don't
#if !HAVE_MEMSET
inline static void *memset(void *s, int c, size_t n) throw()
{
  unsigned char *d = (unsigned char *)s;
  
  if (n) {
    do {
      *d = c;
      d++;
    } while(--n);
  }

  return const_cast<void *>(s);
}
#endif
///

/// Check for availibility of the strchr function and provide a replacement if there is none.
#if !HAVE_STRCHR
inline static char *strchr(const char *s, int c) throw()
{
  while(*s) {
    if (*s == c) return const_cast<char *>(s);
    s++;
  }
  return 0;
}
#endif
///

/// Check for strerror function
#if !HAVE_STRERROR
// we cannot really do much about it as this is Os depdendent. Yuck.
#define strerror(c) "unknown error"
#endif
///

/// Check for strrchr function and provide a replacement.
#if !HAVE_STRRCHR
inline static char *strrchr(const char *s, int c) throw()
{
  const char *t = s + strlen(s);
  
  while(t > s) {
    t--;
    if (*t == c) return const_cast<char *>(t);
  }
  return 0;
}
#endif
///

///
#endif
