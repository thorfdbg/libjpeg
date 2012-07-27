/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** $Id: idct_alt.hpp,v 1.2 2012-06-02 10:27:14 thor Exp $
**
*/

#ifndef DCT_IDCT_HPP
#define DCT_IDCT_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "dct/dct.hpp"
///

/// Forwards
class Quantization;
struct ImageBitMap;
///

/// class IDCT
// This class implements the integer based DCT.
class IDCT : public DCT {
  //
  // Bit assignment
  enum {
    FIX_BITS          = 13, // fractional bits for fixpoint in the calculation
    INTERMEDIATE_BITS = 0,  // fractional bits for representing the intermediate result (none required)
    QUANTIZER_BITS    = 30  // bits for representing the quantizer
  };
  //
  // The (inverse) quantization tables, i.e. multipliers.
  LONG  m_plInvQuant[64];
  //
  // The quantizer tables.
  WORD  m_psQuant[64];
  //
  // Inverse DCT filter core.
  void InverseFilterCore(const LONG *source,LONG *d);
  //
  // Inverse DCT filter core.
  void InverseFilterCore(const LONG *source,LONG *d,const LONG *residual);
  //
  // Perform the forwards filtering
  void ForwardFilterCore(LONG *d,LONG *target);
  //
  // Perform the forwards filtering
  void ForwardFilterCore(LONG *d,LONG *target,LONG *residual);
  //
  // Quantize a floating point number with a multiplier, round correctly.
  // Must remove FIX_BITS + INTER_BITS + 3
  static inline LONG Quantize(LONG n,LONG qnt)
  {
    if (n >= 0) {
      return   (n * QUAD(qnt) + (QUAD(1) << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + 3 - 1))) >> 
	(FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + 3);
    } else {
      // The -1 makes this the same rounding rule as a shift.
      return -((-n * QUAD(qnt) - 1 + (QUAD(1) << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + 3 - 1))) >> 
	       (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + 3));
    }
  }
  //
  // Quantize a floating point number with a multiplier, round correctly.
  // Must remove FIX_BITS + INTER_BITS + 3
  static inline LONG Quantize(LONG no,LONG qnt,LONG &residual);
  //
  static inline LONG Dequantize(LONG src,WORD qnt,LONG residual)
  {
    return ((src * qnt) << 3) + residual;
  }
  //
public:
  IDCT(class Environ *env);
  //
  ~IDCT(void);
  //
  // Use the quantization table defined here, scale them to the needs of the DCT and scale them
  // to the right size.
  virtual void DefineQuant(const UWORD *table);
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const struct ImageBitMap *source,const RectAngle<LONG> &r,
			      LONG *target,LONG max,LONG dcoffset);
  //
  // Run the DCT on an 8x8 block, store the error in a separate block.
  virtual void TransformBlockWithResidual(const struct ImageBitMap *source,const RectAngle<LONG> &r,
					  LONG *target,LONG max,LONG dcoffset,LONG *residual);
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(const struct ImageBitMap *target,const RectAngle<LONG> &r,
				     const LONG *source,LONG max,LONG dcoffset);
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlockWithResidual(const struct ImageBitMap *target,const RectAngle<LONG> &r,
						 const LONG *source,LONG max,LONG dcoffset,const LONG *residual);
};
///

///
#endif
