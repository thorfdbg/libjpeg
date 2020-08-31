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
** This file provides the base class for all integer-based transformations#
** typically Profile C of 18477-7, and -6 and -8.
**
** $Id: integertrafo.hpp,v 1.4 2015/01/20 09:05:36 thor Exp $
**
*/

#ifndef COLORTRAFO_INTEGERTRAFO_HPP
#define COLORTRAFO_INTEGERTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/trivialtrafo.hpp"
///

/// Class IntegerTrafo
// This is the base class for a series of color transformers that work entirely
// in the integer domain, typically Profile C of 18477-7, and -6 and -8.
class IntegerTrafo : public ColorTrafo {
  //
protected:
  //
  // The reconstruction L-Transformation matrix.
  LONG  m_lL[9];
  //
  // The reconstruction R-Transformation matrix.
  LONG  m_lR[9];
  //
  // The reconstruction C-Transformation matrix.
  LONG  m_lC[9];
  //
  // The forwards L-Transformation matrix.
  LONG  m_lLFwd[9];
  //
  // The forwards R-Transfromation matrix.
  LONG  m_lRFwd[9];
  //
  // The forwards C-Transformation matrix.
  LONG  m_lCFwd[9];
  //
  // Encoding and decoding tone mapping lookup tables.
  // These tables are only populated when required.
  //
  // The decoding LUT that maps LDR to HDR.
  const LONG *m_plDecodingLUT[4];
  //
  // The LUTS responsible for Residual decoding, this is the
  // one upfront the color transformation on decoding.
  const LONG *m_plResidualLUT[4];
  //
  // The residual LUT that goes after the tone mapping.
  const LONG *m_plResidual2LUT[4];
  //
  // The encoding LUT that maps HDR to LDR.
  const LONG *m_plEncodingLUT[4];
  //
  // The LUTs that create the residual on encoding.
  // This is the one that goes in after applying the color
  // transformation on encoding.
  const LONG *m_plCreatingLUT[4];
  //
  // The residual LUT that is applied before going into the
  // color transformer.
  const LONG *m_plCreating2LUT[4];
  //
  // An additional offset that is added before going into the Creating2LUT
  ULONG       m_lCreating2Shift;
  //
public:
  IntegerTrafo(class Environ *env,LONG dcshift,LONG max,LONG rdcshift,LONG rmax,LONG outshift,LONG outmax)
    : ColorTrafo(env,dcshift,max,rdcshift,rmax,outshift,outmax),
      m_lCreating2Shift(outshift)
  {
  }
  //
  virtual ~IntegerTrafo(void)
  {
  }
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,Buffer target) = 0;
  // 
  // In case the user already provided a tone-mapped version of the image, this call already
  // takes the LDR version of the image, performs no tone-mapping but only a color
  // decorrelation transformation and injects it as LDR image.
  virtual void LDRRGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
                            Buffer target) = 0;
  // 
  // Buffer the original data unaltered. Required for residual coding, for some modes of
  // it at least.
  virtual void RGB2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
                       Buffer target) = 0;
  //
  // Compute the residual from the original image and the decoded LDR image, place result in
  // the output buffer. This depends rather on the coding model.
  virtual void RGB2Residual(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
                            Buffer reconstructed,Buffer residuals) = 0;
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
                         Buffer source,Buffer residuals) = 0;
  //
  // Return the pixel type of this transformer.
  virtual UBYTE PixelTypeOf(void) const = 0;
  //
  // Define the encoding LUTs.
  void DefineEncodingTables(const LONG *encoding[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_plEncodingLUT[i] = encoding[i];
    }
  }
  //
  // Define the decoding LUTs.
  void DefineDecodingTables(const LONG *decoding[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_plDecodingLUT[i] = decoding[i];
    }
  }
  //
  // Define the residual LUTs on decoding.
  void DefineResidualDecodingTables(const LONG *residual[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_plResidualLUT[i] = residual[i];
    }
  }
  //
  // Define the secondary residual LUTs on decoding.
  void DefineResidual2DecodingTables(const LONG *residual[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_plResidual2LUT[i] = residual[i];
    }
  }
  //
  // Define the residual LUT on encoding.
  void DefineResidualEncodingTables(const LONG *residual[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_plCreatingLUT[i] = residual[i];
    }
  }
  //
  // Define the secondary residual LUTs on encoding.
  void DefineResidual2EncodingTables(const LONG *residual[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_plCreating2LUT[i] = residual[i];
    }
  }
  //
  // Define the inverse L-Transformation.
  void DefineLTransformation(const LONG matrix[9])
  {
    memcpy(m_lL,matrix,sizeof(m_lL));
  }
  //
  // Define the inverse R-Transformation
  void DefineRTransformation(const LONG matrix[9])
  {
    memcpy(m_lR,matrix,sizeof(m_lR));
  }
  //
  // Define the inverse C-Transformation
  void DefineCTransformation(const LONG matrix[9])
  {
    memcpy(m_lC,matrix,sizeof(m_lC));
  }
  //
  // Define the forwards L-Transformation
  void DefineFwdLTransformation(const LONG matrix[9])
  {
    memcpy(m_lLFwd,matrix,sizeof(m_lLFwd));
  }
  //
  // Define the forwards R-Transformation
  void DefineFwdRTransformation(const LONG matrix[9])
  {
    memcpy(m_lRFwd,matrix,sizeof(m_lRFwd));
  }
  //
  // Define the forward C-Transformation
  void DefineFwdCTransformation(const LONG matrix[9])
  {
    memcpy(m_lCFwd,matrix,sizeof(m_lCFwd));
  }
  //
  // Define the additional input table shift required for the Creating2Lut.
  // This is an offset added to the residual before it goes into the
  // table.
  void DefineTableShift(LONG tableshift)
  {
    m_lCreating2Shift = tableshift + m_lOutDCShift;
  }
};
///

///
#endif
