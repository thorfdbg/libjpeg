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
** Several helper functions that are related to native IO of pixel values
**
** $Id: iohelpers.hpp,v 1.9 2022/06/15 06:03:09 thor Exp $
**
*/

#ifndef CMD_IOHELPERS_HPP
#define CMD_IOHELPERS_HPP

/// Includes
#include "interface/types.hpp"
#include "std/stdio.hpp"
#include "std/math.hpp"
///

/// Prototypes
// Interpret a 16-bit integer as half-float casted to int and
// return its double interpretation.
double inline HalfToDouble(UWORD h)
{
  bool sign      = (h & 0x8000)?(true):(false);
  UBYTE exponent = (h >> 10) & ((1 << 5) - 1);
  UWORD mantissa = h & ((1 << 10) - 1);
  double v;

  if (exponent == 0) { // denormalized
    v = ldexp(float(mantissa),-14-10);
  } else if (exponent == 31) {
    v = HUGE_VAL;
  } else {
    v = ldexp(float(mantissa | (1 << 10)),-15-10+exponent);
  }

  return (sign)?(-v):(v);
}
///

// Convert a double to half-precision IEEE and return the bit-pattern
// as a 16-bit unsigned integer.
UWORD inline DoubleToHalf(double v)
{
  bool sign = (v < 0.0)?(true):(false);
  int  exponent = 0; // Silence the compiler
  int  mantissa;

  if (v < 0.0) v = -v;

  if (isinf(v)) {
    exponent = 31;
    mantissa = 0;
  } else if (v == 0.0) {
    exponent = 0;
    mantissa = 0;
  } else {
    double man = 2.0 * frexp(v,&exponent); // must be between 1.0 and 2.0, not 0.5 and 1.
    // Add the exponent bias.
    exponent  += 15 - 1; // exponent bias
    // Normalize the exponent by modifying the mantissa.
    if (exponent >= 31) { // This must be denormalized into an INF, no chance.
      exponent = 31;
      mantissa = 0;
    } else if (exponent <= 0) {
      man *= 0.5; // mantissa does not have an implicit one bit.
      while(exponent < 0) {
        man *= 0.5;
        exponent++;
      }
      mantissa = int(man * (1 << 10));
    } else {
      mantissa = int(man * (1 << 10)) & ((1 << 10) - 1);
    }
  }

  return ((sign)?(0x8000):(0x0000)) | (exponent << 10) | mantissa;
}
///

/// readFloat
// Read an IEEE floating point number from a PFM file
double inline readFloat(FILE *in,bool bigendian)
{
  LONG dt1,dt2,dt3,dt4;
  union {
    LONG  long_buf;
    FLOAT float_buf;
  } u;

  dt1 = getc(in);
  dt2 = getc(in);
  dt3 = getc(in);
  dt4 = getc(in);

  if (dt4 < 0)
    return nan("");

  if (bigendian) {
    u.long_buf = (ULONG(dt1) << 24) | (ULONG(dt2) << 16) | 
      (ULONG(dt3) <<  8) | (ULONG(dt4) <<  0);
  } else {
    u.long_buf = (ULONG(dt4) << 24) | (ULONG(dt3) << 16) | 
      (ULONG(dt2) <<  8) | (ULONG(dt1) <<  0);
  }

  return u.float_buf;
}
///

/// writeFloat
// Write a floating point number to a file
void inline writeFloat(FILE *out,FLOAT f,bool bigendian)
{ 
  union {
    LONG  long_buf;
    FLOAT float_buf;
  } u;

  u.float_buf = f;

  if (bigendian) {
    putc(u.long_buf >> 24,out);
    putc(u.long_buf >> 16,out);
    putc(u.long_buf >>  8,out);
    putc(u.long_buf >>  0,out);
  } else {
    putc(u.long_buf >>  0,out);
    putc(u.long_buf >>  8,out);
    putc(u.long_buf >> 16,out);
    putc(u.long_buf >> 24,out);
  }
}
///

// Read an RGB triple from the stream, convert properly.
extern bool ReadRGBTriple(FILE *in,int &r,int &g,int &b,double &y,int depth,int count,bool flt,bool bigendian,bool xyz);
//
// Open a PPM/PFM file and return its dimensions and properties.
extern FILE *OpenPNMFile(const char *file,int &width,int &height,int &depth,int &precision,bool &isfloat,bool &bigendian);

// Prepare the alpha component for reading, return a file in case it was
// opened successfully
extern FILE *PrepareAlphaForRead(const char *alpha,int width,int height,int &prec,bool &flt,bool &big,
                                 bool alpharesidual,int &hiddenbits,
                                 UWORD ldrtohdr[65536]);
// Parse a quantization matrix from a file
extern bool ParseQuantizationSteps(LONG lumamatrix[64],LONG chromamatrix[64],const char *filename);
///

///
#endif

