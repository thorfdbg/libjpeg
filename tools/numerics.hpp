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
** A couple of numerical tools required here and there.
**
** $Id: numerics.hpp,v 1.11 2015/01/20 10:48:54 thor Exp $
**
*/

#ifndef TOOLS_NUMERICS_HPP
#define TOOLS_NUMERICS_HPP

/// Includes
#include "interface/types.hpp"
///

/// Defines for color transformations.
// Convert floating point to a fixpoint representation
#define TO_FIX(x) LONG((x) * (1L << FIX_BITS) + 0.5)
// Conversion from fixpoint to integer
#define FIX_TO_INT(x) (((x) + ((1L << FIX_BITS) >> 1)) >> FIX_BITS)
// Conversion from fixpoint to color preshifted bits
#define FIX_TO_COLOR(x) (((x) + ((1L << (FIX_BITS - COLOR_BITS)) >> 1)) >> (FIX_BITS - COLOR_BITS))
// With one additional fractional bit
#define FIXCOLOR_TO_COLOR(x) (((x) + ((1L << FIX_BITS) >> 1)) >> FIX_BITS)
// Conversion from fixpoint plus color preshifted bits to INT
#define FIX_COLOR_TO_INT(x) (((x) + ((1L << (FIX_BITS + COLOR_BITS)) >> 1)) >> (FIX_BITS + COLOR_BITS))
// Almost to int, one fractional bit remains.
#define FIX_COLOR_TO_INTCOLOR(x) (((x) + ((1L << FIX_BITS) >> 1)) >> FIX_BITS)
// Conversion from color fixpoint to integer
#define COLOR_TO_INT(x) (((x) + ((1L << COLOR_BITS) >> 1)) >> COLOR_BITS)
// Conversion from integer to color preshifted bits
#define INT_TO_COLOR(x) ((x) << (COLOR_BITS))
///

/// Endian-independent IEEE converters
// Convert an encoded big-endian IEEE number into a FLOAT.
FLOAT  IEEEDecode(ULONG bits);
DOUBLE IEEEDecode(UQUAD bits);
// And backwards. Encode an IEEE number into 32 or 64 bits.
ULONG  IEEEEncode(FLOAT number);
UQUAD  IEEEEncode(DOUBLE number);
///

#endif
