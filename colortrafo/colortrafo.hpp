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
** This file provides the transformation from RGB to YCbCr for
** the integer coding modes. Floating point coding modes (profiles A and B)
** are handled by the floattrafo.
**
** $Id: colortrafo.hpp,v 1.37 2014/12/30 13:29:35 thor Exp $
**
*/

#ifndef COLORTRAFO_COLORTRAFO_HPP
#define COLORTRAFO_COLORTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "std/string.hpp"
///

/// Forwards
class ParametricToneMappingBox;
///

/// Class ColorTrafo
// The base class for all decorrelation transformation
class ColorTrafo : public JKeeper {
  //
public:
  // Number of bits preshifted for the color-transformed channels
  enum {
    COLOR_BITS = 4,
    FIX_BITS   = 13
  }; 
  //
  // Flags for the various color transformations.
  enum OutputFlags {
    ClampFlag  = 1,   // Clamp to range (instead of wrap-around)
    Float      = 32,  // set in case the output should be converted to IEEE float
    Extended   = 64,  // should always be set unless no merging spec box is there.
    Residual   = 128  // set if there is a residual
  };
  //
  // A buffer consists of four pointer to 8x8 blocks of data, ordered
  // in R,G,B,alpha or Y,Cb,Cr,Alpha
  typedef LONG *Buffer[4];
  typedef LONG Block[64];
  //
protected:
  //
  // DC-Shift in the legacy domain before applying the LUT.
  LONG  m_lDCShift;
  //
  // Maximum value in the legacy domain before applying the LUT.
  LONG  m_lMax;
  //
  // DC-Shift in the residual domain before applying the LUT
  LONG  m_lRDCShift;
  //
  // Maximum value in the residual domain before applying the LUT.
  LONG  m_lRMax;
  // 
  // DC-shift in the spatial domain.
  LONG m_lOutDCShift;
  //
  // Maximum value in the output (spatial, image) domain.
  LONG  m_lOutMax;
  //
public:
  //
  // Construct a color transformer. Arguments are dcshift and maximum in the legacy domain,
  // dcshift and maximum in the residual domain, both before appyling L and R lut, plus
  // maximum in the image domain.
  ColorTrafo(class Environ *env,LONG dcshift,LONG max,LONG rdcshift,LONG rmax,LONG outshift,LONG outmax)
    : JKeeper(env), m_lDCShift(dcshift), m_lMax(max), 
      m_lRDCShift(rdcshift), m_lRMax(rmax), m_lOutDCShift(outshift), m_lOutMax(outmax)
  { }
  //
  virtual ~ColorTrafo(void)
  { }
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift. This call computes a LDR image from the given input data
  // and moves that into the target buffer. Shift and max values are the clamping
  // of the LDR data.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
                         Buffer target) = 0;
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
                            Buffer reconstructed,Buffer residual) = 0;
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
                         Buffer source,Buffer residual) = 0;
  //
  // Return the external pixel type of this trafo.
  virtual UBYTE PixelTypeOf(void) const = 0;
};
///

///
#endif
