/*
** This file provides the transformation from RGB to YCbCr in the floating
** point profiles, profiles A and B of 18477-7.
**
** $Id: multiplicationtrafo.hpp,v 1.5 2014/10/13 13:06:42 thor Exp $
**
*/

#ifndef COLORTRAFO_MULTIPLICATIONTRAFO_HPP
#define COLORTRAFO_MULTIPLICATIONTRAFO_HPP

/// Includes
#include "tools/traits.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/trivialtrafo.hpp"
#if ISO_CODE
///

/// Forwards
class ParametricToneMappingBox;
///

/// Class MultiplicationTrafo
// The transformer for floating point based transformations.
// This has always a residual, and matrices and LUTs have floating
// point entries.
template<int count,int ltrafo,int rtrafo,bool diagonal>
class MultiplicationTrafo : public FloatTrafo {
  //
  //
  // A private helper to implement the identity transformation.
  TrivialTrafo<FLOAT,FLOAT,count> m_TrivialHelper;
  //
public:
  MultiplicationTrafo(class Environ *env,LONG dcshift,LONG max,LONG rdcshift,LONG rmax,LONG outshift,LONG outmax);
  //
  virtual ~MultiplicationTrafo(void);
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift. This call computes a LDR image from the given input data
  // and moves that into the target buffer. Shift and max values are the clamping
  // of the LDR data.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 Buffer target);
  //
  // In case the user already provided a tone-mapped version of the image, this call already
  // takes the LDR version of the image, performs no tone-mapping but only a color
  // decorrelation transformation and injects it as LDR image.
  virtual void LDRRGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 Buffer target);
  //
  // Buffer the original data unaltered. Required for residual coding, for some modes of
  // it at least.
  virtual void RGB2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
		       Buffer target)
  {
    m_TrivialHelper.RGB2RGB(r,source,target);
  }
  //
  // Compute the residual from the original image and the decoded LDR image, place result in
  // the output buffer. This depends rather on the coding model.
  virtual void RGB2Residual(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			    Buffer reconstructed,Buffer residual);
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
			 Buffer source,Buffer residual);
};
///

///
#endif
#endif
