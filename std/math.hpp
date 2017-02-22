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
** $Id: math.hpp,v 1.8 2014/09/30 08:33:18 thor Exp $
*/

#ifndef MATH_HPP
#define MATH_HPP

#if HAVE_MATH_H
#include <math.h>
#ifdef DEPLOY_PICCLIB
# if defined(__cplusplus)
extern "C" {
# endif
# include "pegasus/picclib.h"
# if defined(__cplusplus)
}
# endif
#endif

// Check or add isnan and/or isinf
#ifndef isnan
# define isnan(x) ((x) != (x))
# ifdef _MSC_VER
#  define nan(x)   ((HUGE_VAL+HUGE_VAL)-(HUGE_VAL+HUGE_VAL))
# endif
#endif
#ifndef isinf
# define isinf(x) (!isnan(x) && (isnan((x) - (x))))
#endif

#else
#error "math.h header not available, won't compile without"
#endif
#endif

