/*
** This file provides the no-transformation case for the AdobeRGB marker.
**
** $Id: trivialtrafo.hpp,v 1.18 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef COLORTRAFO_TRIVIALTRAFO_HPP
#define COLORTRAFO_TRIVIALTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
///

/// Class Trivialrafo
// This class provides the tranformation from RGB to RGB in case
// the AdobeRGB marker is present. It does only perform output
// clipping on reverse transformation and reorganization of
// the data structures. The residual is only added here and
// never transformed. Conceptionally, the residual is also
// level shifted to unsigned values.
template<typename internal,typename external,int count>
class TrivialTrafo : public ColorTrafo {
  //
public:
  TrivialTrafo(class Environ *env,LONG dcshift,LONG max);
  //
  virtual ~TrivialTrafo(void);
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 Buffer target);
  //
  // In case the user already provided a tone-mapped version of the image, this call already
  // takes the LDR version of the image, performs no tone-mapping but only a color
  // decorrelation transformation and injects it as LDR image.
  virtual void LDRRGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			    Buffer target)
  {
    RGB2YCbCr(r,source,target);
  }
  //
  // Buffer the original data unaltered. Required for residual coding, for some modes of
  // it at least.
  virtual void RGB2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
		       Buffer target)
  {
    RGB2YCbCr(r,source,target);
  }
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift. Additionally may integrate a residual stream.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
			 Buffer source,Buffer res); 
  // 
  // Compute the residual from the original image and the decoded LDR image, place result in
  // the output buffer. This depends rather on the coding model.
  virtual void RGB2Residual(const RectAngle<LONG> &,const struct ImageBitMap *const *,
			    Buffer,Buffer)
  {
    JPG_THROW(INVALID_PARAMETER,"TrivialTrafo::RGB2Residual",
	      "the trivial transformation does not support residual coding");
  }
  //
  // Return the number of fractional bits this color transformation requires.
  virtual UBYTE FractionalLBitsOf(void) const
  {
    return 0;
  }
  //
  // Return the number of fractional bits this color transformation requires.
  // None, this is integer to integer.
  virtual UBYTE FractionalRBitsOf(void) const
  {
    return 0;
  }
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
