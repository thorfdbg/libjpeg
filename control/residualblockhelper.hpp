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
** This class computes, prepares or includes residual data for block
** based processing. It abstracts parts of the residual coding
** process.
**
** $Id: residualblockhelper.hpp,v 1.32 2014/09/30 08:33:16 thor Exp $
**
*/


#ifndef CONTROL_RESIDUALBLOCKHELPER_HPP
#define CONTROL_RESIDUALBLOCKHELPER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "marker/frame.hpp"
///

/// Forwards
struct ImageBitMap;
class DCT;
///

/// Class ResidualBlockHelper
// This class computes, prepares or includes residual data for block
// based processing. It abstracts parts of the residual coding
// process.
class ResidualBlockHelper : public JKeeper {
  //
  // The frame that contains the image.
  class Frame         *m_pFrame;
  //
  // The residual frame that contains the image.
  class Frame         *m_pResidualFrame;
  //
  // Number of components in the frame, number of components we handle.
  UBYTE                m_ucCount;
  //
  // The DCT for the components, with quantization filled in
  // In case no DCT is run, this is left as NULL.
  class DCT           *m_pDCT[4];
  //
  // The quantization values for luma and chroma. There is only
  // one per component - this is in case the DCT is turned off.
  UWORD                m_usQuantization[4];
  //
  // Noise shaping parameters, one per parameter.
  bool                 m_bNoiseShaping[4];
  //
  // Maximum error for noise masking. Keep at zero.
  UBYTE                m_ucMaxError;
  //
  // Do we have the quantization values?
  bool                 m_bHaveQuantizers;
  //
  // Color transformer buffer
  LONG                 m_ColorBuffer[4][64];
  LONG                *m_pBuffer[4];
  //
  // Find the quantization table for residual component i (index, not label).
  const UWORD *FindQuantizationFor(UBYTE i) const;
  //
  // Find the DCT for the given component, when DCT is enabled. 
  class DCT   *FindDCTFor(UBYTE i) const;
  //
  // Allocate the temporary buffers to hold the residuals and their bitmaps.
  // Only required during encoding.
  void AllocateBuffers(void);
  //
public:
  //
  // Construct the helper from the frame and its residual version.
  ResidualBlockHelper(class Frame *frame,class Frame *residualframe);
  //
  ~ResidualBlockHelper(void);
  //
  // Dequantize the already decoded residual (possibly taking the decoded
  // image as predictor) and return it, ready for the color transformation.
  void DequantizeResidual(const LONG *legacy,LONG *target,const LONG *residual,UBYTE i);
  //
  // Quantize the residuals of a block given the DCT data
  void QuantizeResidual(const LONG *legacy,LONG *residual,UBYTE i,LONG bx,LONG by);
  //
  // Return the frame this is part of which is extended by a residual
  class Frame *FrameOf(void) const
  {
    return m_pFrame;
  }
  //
  // Return the residual frame this is part of and which extends the above
  // frame by residuals.
  class Frame *ResidualFrameOf(void) const
  {
    return m_pResidualFrame;
  }
};
///

///
#endif
