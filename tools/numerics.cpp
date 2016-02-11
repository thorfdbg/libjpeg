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
