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
