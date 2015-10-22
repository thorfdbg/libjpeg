/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
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
**
** Integer DCT operation plus scaled quantization.
** This DCT is based entirely on lifting and is hence always invertible.
** It is taken from the article "Integer DCT-II by Lifting Steps", by
** G. Plonka and M. Tasche, with a couple of corrections and adaptions.
** The A_4(1) matrix in the article rotates the wrong elements.
** Furthermore, the DCT in this article is range-extending by a factor of
** two in each dimension, which we cannot effort. Instead, the butterflies
** remain unscaled and are replaced by lifting rotations.
** Proper rounding is required as we cannot use fractional bits.
**
** $Id: liftingdct.hpp,v 1.8 2015/05/09 20:09:21 thor Exp $
**
*/

#ifndef DCT_LIFTINGDCT_HPP
#define DCT_LIFTINGDCT_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "dct/dct.hpp"
///

/// Forwards
class Quantization;
struct ImageBitMap;
///

/// class LiftingDCT
// This class implements the integer based DCT. The template parameter is the number of
// preshifted bits coming in from the color transformer.
template<int preshift,typename T,bool deadzone>
class LiftingDCT : public DCT {
  //
  // Bit assignment
  enum {
    QUANTIZER_BITS    = 30  // bits for representing the quantizer
  };
  //
  // The (inverse) quantization tables, i.e. multipliers.
  LONG  m_plInvQuant[64];
  //
  // The quantizer tables, already scaled to range
  LONG  m_plQuant[64];
  //
  // Quantize a floating point number with a multiplier, round correctly.
  // Must remove INTER_BITS + 3
  static inline LONG Quantize(T n,LONG qnt,bool dc)
  {
    if (deadzone == false || dc) {
      return (n * QUAD(qnt) + (QUAD(1) << (QUANTIZER_BITS - 1)) - 
              (((typename TypeTrait<T>::Unsigned)(n)) >> TypeTrait<T>::SignBit))
        >> (QUANTIZER_BITS);
    } else {
      QUAD m = n >> TypeTrait<T>::SignBit;
      QUAD o = m << (QUANTIZER_BITS - 2);
      return (n * QUAD(qnt) + ((~o) & m) + (QUAD(3) << (QUANTIZER_BITS - 3)))
        >> (QUANTIZER_BITS);
    }
  }
  //
  //
public:
  LiftingDCT(class Environ *env);
  //
  ~LiftingDCT(void);
  //
  // Use the quantization table defined here, scale them to the needs of the DCT and scale them
  // to the right size.
  virtual void DefineQuant(const UWORD *table);
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const LONG *source,LONG *target,LONG dcoffset);
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset);
};
///

///
#endif
