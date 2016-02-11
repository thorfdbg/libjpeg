/*
** This file provides the transformation from RGB to YCbCr
**
** $Id: ycbcrtrafo.hpp,v 1.34 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef COLORTRAFO_YCBCRTRAFO_HPP
#define COLORTRAFO_YCBCRTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/integertrafo.hpp"
///

/// Class YCbCrTrafo
// This class provides the tranformation from RGB to YCbCr following
// the JFIF guidelines, plus all other reasonable combinations of the
// L and R transformations without S.
template<typename external,int count,UBYTE oc,int regulartrafo,int residualtrafo>
class YCbCrTrafo : public IntegerTrafo {
  // 
  // A private helper to implement the identity transformation.
  TrivialTrafo<LONG,external,count> m_TrivialHelper;
  //
public:
  YCbCrTrafo(class Environ *env,LONG dcshift,LONG max,LONG rdcshift,LONG rmax,LONG outshift,LONG outmax);
  //
  virtual ~YCbCrTrafo(void);
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,Buffer target);
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
			    Buffer reconstructed,Buffer residuals);
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
			 Buffer source,Buffer residuals);
  //
  // Return the pixel type of this transformer.
  virtual UBYTE PixelTypeOf(void) const
  {
    return TypeTrait<external>::TypeID;
  }
  //
};
///

///
#endif
