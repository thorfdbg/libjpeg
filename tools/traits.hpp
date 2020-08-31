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
**
** This file contains type traits for all the integer types and provides
** information on the number of bits, sign bit location, signedness, etc.
** of various types.
**
** $Id: traits.hpp,v 1.9 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef TOOLS_TRAITS_HPP
#define TOOLS_TRAITS_HPP

/// Includes
#include "config.h"
#include "std/math.hpp"
#include "interface/types.hpp"
///

/// Coefficient data type
// note that the coefficient type is equal to the size of the
// object OR'd with a sign bit.
// These must coincide with the JPG_Type macros
#define CTYP_SIGNED_BIT     6
#define CTYP_SIGNED_MASK    (1<<6)
#define CTYP_FLOAT_BIT      5
#define CTYP_FLOAT_MASK     (1<<5)
#define CTYP_FIX_BIT        4
#define CTYP_FIX_MASK       (1<<4)
// The next one is special: It means that the data is organized
// different from the native CPU organization: i.e. little
// endian on big-endian machines and vice versa
#define CTYP_SWAP_BIT       3
#define CTYP_SWAP_MASK      (1<<3)

#define CTYP_SIZE_MASK      (0x07)

#define CTYP_UBYTE   (sizeof(UBYTE))
#define CTYP_BYTE    (sizeof(BYTE) | CTYP_SIGNED_MASK)
#define CTYP_UWORD   (sizeof(UWORD))
#define CTYP_WORD    (sizeof(WORD) | CTYP_SIGNED_MASK)
#define CTYP_ULONG   (sizeof(ULONG))
#define CTYP_LONG    (sizeof(LONG) | CTYP_SIGNED_MASK)

#define CTYP_FLOAT   (sizeof(FLOAT) | CTYP_SIGNED_MASK | CTYP_FLOAT_MASK)
#define CTYP_FIX     (sizeof(LONG)  | CTYP_SIGNED_MASK | CTYP_FIX_MASK)
#define CTYP_SIX     (sizeof(WORD)  | CTYP_SIGNED_MASK | CTYP_FIX_MASK)
//
//
#define CTYP_SW_UWORD (CTYP_UWORD   | CTYP_SWAP_MASK)
#define CTYP_SW_WORD  (CTYP_WORD    | CTYP_SWAP_MASK)
///

/// Useful macros for coefficient data types
#define CTYP_SIZE_OF(x)     (x & CTYP_SIZE_MASK)
#define CTYP_BITS_OF(x)     (CTYP_SIZE_OF(x)<<3)
#define CTYP_SIGNBIT_OF(x)  (CTYP_BITS_OF(x)-1)
#define CTYP_IS_FLOAT(x)    (x & CTYP_FLOAT_MASK)
#define CTYP_IS_FIX(x)      (x & CTYP_FIX_MASK)
///

/// TypeTrait
// Abstract base type, specialized versions supply all the data required.
template<typename type>
struct TypeTrait {
};
// This structure performs the reverse lookup: From ID to type.
template<int ID>
struct IDTrait {
};
///

/// TypeTrait<UBYTE>
template<>
struct TypeTrait<UBYTE> {
  //
  enum {
    isSigned  = false
  };
  enum {
    isFloat   = false
  };
  enum {
    TypeID    = CTYP_UBYTE
  };
  enum {
    ByteSize  = 1
  };
  enum {
    BitSize   = 8
  };
  enum {
    MSBMask   = 0x80UL
  };
  enum {
    Min       = MIN_UBYTE
  };
  enum {
    Max       = MAX_UBYTE
  };
  //
  // Related types
  typedef BYTE  Signed;
  typedef UBYTE Unsigned;
};
template<>
struct IDTrait<CTYP_UBYTE> {
  typedef UBYTE Type;
};
///

/// TypeTrait<BYTE>
template<>
struct TypeTrait<BYTE> {
  //
  enum {
    isSigned  = true
  };
  enum {
    isFloat   = false
  };
  enum {
    TypeID    = CTYP_BYTE
  };
  enum {
    ByteSize  = 1
  };
  enum {
    BitSize   = 8
  }; 
  enum {
    Min       = MIN_BYTE
  };
  enum {
    Max       = MAX_BYTE
  };
  enum {
    SignBit   = 7
  };
  enum {
    SignMask  = 0x80
  }; 
  //
  // Related types
  typedef BYTE  Signed;
  typedef UBYTE Unsigned;
};
template<>
struct IDTrait<CTYP_BYTE> {
  typedef BYTE Type;
};
///

/// TypeTrait<UWORD>
template<>
struct TypeTrait<UWORD> {
  //
  enum {
    isSigned  = false
  };
  enum {
    isFloat   = false
  };
  enum {
    TypeID    = CTYP_UWORD
  };
  enum {
    ByteSize  = 2
  };
  enum {
    BitSize   = 16
  };  
  enum {
    MSBMask   = 0x8000UL
  };
  enum {
    Min       = MIN_UWORD
  };
  enum {
    Max       = MAX_UWORD
  }; 
  //
  // Related types
  typedef WORD  Signed;
  typedef UWORD Unsigned;
};
template<>
struct IDTrait<CTYP_UWORD> {
  typedef UWORD Type;
};
///

/// TypeTrait<WORD>
template<>
struct TypeTrait<WORD> {
  //
  enum {
    isSigned  = true
  };
  enum {
    isFloat   = false
  };
  enum {
    TypeID    = CTYP_WORD
  };
  enum {
    ByteSize  = 2
  };
  enum {
    BitSize   = 16
  }; 
  enum {
    Min       = MIN_WORD
  };
  enum {
    Max       = MAX_WORD
  };
  enum {
    SignBit   = 15
  };
  enum {
    SignMask  = 0x8000
  };
  //
  // Related types
  typedef WORD  Signed;
  typedef UWORD Unsigned; 
};
template<>
struct IDTrait<CTYP_WORD> {
  typedef WORD Type;
};
///

/// TypeTrait<ULONG>
template<>
struct TypeTrait<ULONG> {
  //
  enum {
    isSigned  = false
  };
  enum {
    isFloat   = false
  };
  enum {
    TypeID    = CTYP_ULONG
  };
  enum {
    ByteSize  = 4
  };
  enum {
    BitSize   = 32
  }; 
  enum {
    MSBMask   = 0x80000000UL
  };
  enum {
    Min       = MIN_ULONG
  };
  enum {
    Max       = MAX_ULONG
  };
  //
  // Related types
  typedef LONG  Signed;
  typedef ULONG Unsigned; 
};
template<>
struct IDTrait<CTYP_ULONG> {
  typedef ULONG Type;
};
///

/// TypeTrait<LONG>
template<>
struct TypeTrait<LONG> {
  //
  enum {
    isSigned  = true
  };
  enum {
    isFloat   = false
  };
  enum {
    TypeID    = CTYP_LONG
  };
  enum {
    ByteSize  = 4
  };
  enum {
    BitSize   = 32
  };
  enum {
    Min       = MIN_LONG
  };
  enum {
    Max       = MAX_LONG
  };
  enum {
    SignBit   = 31
  };
  enum {
    SignMask  = 0x80000000UL
  };
  //
  // Related types
  typedef LONG  Signed;
  typedef ULONG Unsigned; 
};
template<>
struct IDTrait<CTYP_LONG> {
  typedef LONG Type;
};
///

/// TypeTrait<UQUAD>
template<>
struct TypeTrait<UQUAD> {
  //
  enum {
    isSigned  = false
  };
  enum {
    isFloat   = false
  };
  enum {
    ByteSize  = 8
  };
  enum {
    BitSize   = 64
  };
  //
  // Related types
  typedef QUAD  Signed;
  typedef UQUAD Unsigned;  
};
///

/// TypeTrait<QUAD>
template<>
struct TypeTrait<QUAD> {
  //
  enum {
    isSigned  = true
  };
  enum {
    isFloat   = false
  };
  enum {
    ByteSize  = 8
  };
  enum {
    BitSize   = 64
  };
  enum {
    SignBit   = 63
  };
  //
  // Related types
  typedef QUAD  Signed;
  typedef UQUAD Unsigned;  
};
///

/// TypeTrait<FLOAT>
template<>
struct TypeTrait<FLOAT> {
  //
  enum {
    isSigned  = true
  };
  enum {
    isFloat   = true
 };
  enum {
    TypeID    = CTYP_FLOAT
  };
  enum {
    ByteSize  = sizeof(FLOAT)
  };
  enum {
    BitSize   = sizeof(FLOAT) << 3
  };
  // Implicit integer bit in IEEE numbers
  enum {
    ImplicitOne     = 0x00800000UL
  };
  // Mask to extract the mantissa from IEEE numbers
  enum {
    MantissaMask    = 0x007fffffUL
  };
  // Exponent bias to zero. If the exponent is this, the first bit of the
  // mantissa has the meaning "1.0".
  enum {
    ExponentBias    = 0x7f
  };
  // The following masks out the sign bit
  enum {
    SignMask        = 0x80000000
  };
  // The bit containing the sign.
  enum {
    SignBit         = 31
  };
  // The following masks out the exponent without the sign bit after
  // the exponent has been shifted down
  enum {
    ExponentMask    = 0xff
  };
  // First bit of the exponent in the representation of IEEE numbers.
  enum {
    ExponentBit     = 23
  };
  enum {
    Max             = ((1L << 23) - 1)
  };
};
template<>
struct IDTrait<CTYP_FLOAT> {
  typedef FLOAT Type;
};
///

///
#endif
