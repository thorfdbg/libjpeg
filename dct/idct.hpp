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
** Integer DCT operation plus scaled quantization.
**
** $Id: idct.hpp,v 1.20 2016/10/28 13:58:54 thor Exp $
**
*/

#ifndef DCT_IDCT_HPP
#define DCT_IDCT_HPP

/// Includes
#include "tools/environment.hpp"
#include "dct/dct.hpp"
#include "tools/traits.hpp"
///

/// Forwards
class Quantization;
struct ImageBitMap;
///

/// class IDCT
// This class implements the integer based DCT. The template parameter is the number of
// preshifted bits coming in from the color transformer.
template<int preshift,typename T,bool deadzone,bool optimize>
class IDCT : public DCT {
  //
  // Bit assignment
  enum {
    FIX_BITS          = 9,  // fractional bits for fixpoint in the calculation
    // 9 is the maximum for 16bpp input or 12 + 4(color bits)
    // This is because 12+4+3+3+9 = 31
    INTERMEDIATE_BITS = 0,  // fractional bits for representing the intermediate result
    // (none required)
    QUANTIZER_BITS    = 30  // bits for representing the quantizer
  };
  //
  // The (inverse) quantization tables, i.e. multipliers.
  LONG  m_plInvQuant[64];
  //
  // The quantizer tables.
  LONG  m_plQuant[64];
  //
  // Local buffer for the scaled unquantized data. This allows an R/D optimization.
  LONG m_lTransform[64];
  //
  // Quantize a floating point number with a multiplier, round correctly.
  // Must remove FIX_BITS + INTER_BITS + 3
  inline LONG Quantize(LONG n,LONG qnt,int band)
  {
    if (optimize) {
      // If the optimization is on, also store the raw unqantized data.
      // It just has to be scaled correctly.
      // The preshift is not removed since it is part of the quantization
      // settings to remove it.
      m_lTransform[band] = n >> (FIX_BITS + INTERMEDIATE_BITS + 3);
    }
    
    if (deadzone == false || band == 0) {
      return (n * QUAD(qnt) + (ULONG(-n) >> TypeTrait<LONG>::SignBit) + 
              (QUAD(1) << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3 - 1)))
        >> (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3);
    } else {
      QUAD m = n >> TypeTrait<LONG>::SignBit;
      QUAD o = m << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3 - 2);
      return (n * QUAD(qnt) + ((~o) & m) +
              (QUAD(3) << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3 - 3)))
        >> (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3);
    }
  }
  //
public:
  IDCT(class Environ *env);
  //
  ~IDCT(void);
  //
  // Use the quantization table defined here, scale them to the needs of the DCT and scale them
  // to the right size.
  virtual void DefineQuant(class QuantizationTable *table);
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const LONG *source,LONG *target,LONG dcoffset);
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset);
  //
  // Estimate a critical slope (lambda) from the unquantized data.
  // Or to be precise, estimate lambda/delta^2, the constant in front of
  // delta^2.
  virtual DOUBLE EstimateCriticalSlope(void);
  //
  // Return (in case optimization is enabled) a pointer to the unquantized
  // but DCT transformed data. The data is potentially preshifted.
  virtual const LONG *TransformedBlockOf(void) const
  {
    assert(optimize);
    return m_lTransform;
  } 
  //
  // Return (in case optimization is enabled) a pointer to the effective
  // quantization step sizes.
  virtual const LONG *BucketSizes(void) const
  {
    return m_plQuant;
  } 
  //
  // The prescaling of the DCT. This is the number of bits the input data
  // is upshifted compared to the regular input.
  virtual int PreshiftOf(void) const
  {
    return preshift;
  }
};
///

///
#endif
