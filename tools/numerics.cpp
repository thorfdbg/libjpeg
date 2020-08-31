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
** $Id: numerics.cpp,v 1.6 2014/09/30 08:33:18 thor Exp $
**
*/

/// Includes
#include "tools/numerics.hpp"
#include "tools/traits.hpp"
#include "std/math.hpp"
///

/// IEEEDecode(ULONG)
// Convert an encoded big-endian IEEE number into a FLOAT.
FLOAT IEEEDecode(ULONG bits)
{
  bool sign = false;
  LONG exp;
  LONG man;
  FLOAT f;
  //
  if (bits & TypeTrait<FLOAT>::SignMask)
    sign = true;
  //
  if (bits & ~TypeTrait<FLOAT>::SignMask) {
    exp = (bits >> TypeTrait<FLOAT>::ExponentBit) & 0xff;
    man = bits & ((1UL << TypeTrait<FLOAT>::ExponentBit) - 1);
    if (exp == 0xff) {
      f = HUGE_VAL;
      return (sign)?(-f):(f);
      // Nan or Inf. Not in J2k, please.
    } else if (exp) {
      man += 1UL << TypeTrait<FLOAT>::ExponentBit;
      // implicit one bit if not denormalized
    } else {
      exp++;
      // denormalized number, offset exponent
    }
    // Exponent bias plus additional shift
    exp -= TypeTrait<FLOAT>::ExponentBias + TypeTrait<FLOAT>::ExponentBit;
    f    = ldexp(FLOAT(man),exp);
  } else {
    f    = +0.0;
  }
  if (sign)
    f = -f;
  return f;
}
///

/// IEEEDecode(UQUAD)
DOUBLE IEEEDecode(UQUAD bits)
{  
  bool sign = false;
  LONG exp;
  QUAD man;
  DOUBLE f;
  //
  if (bits & (UQUAD(1) << 63))
    sign = true;
  //
  if (bits & ((UQUAD(1) << 63) - 1)) {
    exp = (bits >> 52) & 0x7ff;
    man = bits & ((UQUAD(1) << 52) - 1);
    if (exp == 0x7ff) {
      // Nan or Inf. Not in J2k, please.
      f = HUGE_VAL;
      return (sign)?(-f):(f);
    } else if (exp) {
      man += UQUAD(1) << 52;
      // implicit one bit if not denormalized
    } else {
      exp++;
      // denormalized number, offset exponent
    }
    // Exponent bias plus additional shift
    exp -= 1023 + 52;
    f    = ldexp(DOUBLE(man),exp);
  } else {
    f    = +0.0;
  }
  if (sign)
    f = -f;
  return f;
}
///

/// IEEEEncode(FLOAT)
ULONG  IEEEEncode(FLOAT number)
{ 
  bool sign = false;
  int exp;
  LONG man;
  //
  if (number < 0.0) {
    number = -number;
    sign   = true;
  }
  if (number != 0.0) {
    man = LONG(frexp(number,&exp) * (1L << (TypeTrait<FLOAT>::ExponentBit + 1)));
    // according to the frexp specs, man should now be at least 1 << 23 and
    // at most 1 << 24 - 1; however, make sure we got it right...
    while (man >= 1L << (TypeTrait<FLOAT>::ExponentBit + 1)) {
      man >>= 1;
      exp++;
    }
    // Add up exponent bias
    exp += TypeTrait<FLOAT>::ExponentBias - 1;
    if (exp >= TypeTrait<FLOAT>::ExponentMask) {
      // Make this is a signed INF.
      man = 0;
      exp = TypeTrait<FLOAT>::ExponentMask;
    } else if (exp <= 0) {
      // Denormalize the number.
      man >>= 1 - exp;
      // encode as zero to indicate denormalization
      exp = 0;
    } else {
      // Remove the implicit one-bit
      man &= TypeTrait<FLOAT>::MantissaMask;
    }
    // Encode the number.
    man |= LONG(exp) << TypeTrait<FLOAT>::ExponentBit;
  } else {
    // Make this a zero
    man  = 0;
  }
  if (sign) {
    return ULONG(man) | (1UL << 31);
  } else {
    return ULONG(man);
  }
}
///

/// IEEEEncode(DOUBLE)
UQUAD IEEEEncode(DOUBLE number)
{
  bool sign = false;
  int exp;
  LONG man;
  //
  if (number < 0.0) {
    number = -number;
    sign   = true;
  }
  if (number != 0.0) {
    man = LONG(frexp(number,&exp) * (1L << (TypeTrait<FLOAT>::ExponentBit + 1)));
    // according to the frexp specs, man should now be at least 1 << 23 and
    // at most 1 << 24 - 1; however, make sure we got it right...
    while (man >= 1L << (TypeTrait<FLOAT>::ExponentBit + 1)) {
      man >>= 1;
      exp++;
    }
    // Add up exponent bias
    exp += TypeTrait<FLOAT>::ExponentBias - 1;
    if (exp >= TypeTrait<FLOAT>::ExponentMask) {
      // Make this is a signed INF.
      man = 0;
      exp = TypeTrait<FLOAT>::ExponentMask;
    } else if (exp <= 0) {
      // Denormalize the number.
      man >>= 1 - exp;
      // encode as zero to indicate denormalization
      exp = 0;
    } else {
      // Remove the implicit one-bit
      man &= TypeTrait<FLOAT>::MantissaMask;
    }
    // Encode the number.
    man |= LONG(exp) << TypeTrait<FLOAT>::ExponentBit;
  } else {
    // Make this a zero
    man  = 0;
  }
  if (sign) {
    return ULONG(man) | (1UL << 31);
  } else {
    return ULONG(man);
  }
}
///
