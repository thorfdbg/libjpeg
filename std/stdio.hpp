/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** $Id: stdio.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef STDIO_HPP
#define STDIO_HPP
#include "config.h"

#if defined(HAVE_STDIO_H)
#include <stdio.h>
#else
#error "stdio.h not available, won't compile without"
#endif

#ifndef HAVE_VSNPRINTF
#include "std/stdarg.hpp"
extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#ifndef HAVE_SNPRINTF
#include "std/stdarg.hpp"
extern int TYPE_CDECL snprintf(char *str,size_t size,const char *format,...);
#endif

// Replace fopen by fopen64 if this is available.
#ifdef HAVE_FOPEN64
# define fopen fopen64
#endif

#endif
