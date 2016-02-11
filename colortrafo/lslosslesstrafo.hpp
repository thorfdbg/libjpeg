/*
** This file provides the transformation from RGB to YCbCr
**
** $Id: lslosslesstrafo.hpp,v 1.22 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef COLORTRAFO_LSLOSSLESSTRAFO_HPP
#define COLORTRAFO_LSLOSSLESSTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
///

/// Forwards
class Frame;
class LSColorTrafo;
///

/// Class LSLosslessTrafo
// This class implements the color transformation specified in 
// the JPEG-LS part 2 standard.
template<typename external,int count>
class LSLosslessTrafo : public ColorTrafo {
  //
  // The maximum value of the external sample value.
  LONG    m_lMaxTrans;
  //
  // The modulo of the input = maxtrans + 1
  LONG    m_lModulo;
  //
  // The offset when adding to components.
  LONG    m_lOffset;
  //
  // The near value on encoding.
  LONG    m_lNear;
  //
  // On decoding, the component indices (not labels!) that
  // are used as input, in the order they appear. This array,
  // when using the index i, delivers the component index j
  // used as i'th input to the transformation.
  UBYTE   m_ucInternal[count];
  //
  // The inverse of the above array, given the internal color
  // transformation index, delivers the component index (not
  // label) it maps to.
  UBYTE   m_ucInverse[count];
  //
  // The right shift values.
  UBYTE   m_ucRightShift[count];
  //
  // Subtract or add to the base component? If
  // true, the output is subtracted, otherwise
  // added. Also, if false, the half offset is added.
  bool    m_bCentered[count];
  //
  // Matrix multipliers. The first index
  // runs over the output (external) components, the
  // second over the internal components.
  UWORD   m_usMatrix[count][count-1]; 
  //
public:
  LSLosslessTrafo(class Environ *env,LONG dcshift,LONG max,LONG rdcshift,LONG rmax,LONG outshift,LONG outmax);
  //
  virtual ~LSLosslessTrafo(void);
  //
  // Install the transformation from an LSColorTrafo marker, one of
  // the JPEG LS extensions markers.
  void InstallMarker(const class LSColorTrafo *marker,const class Frame *frame);
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 Buffer target);
  // 
  // Buffer the original data unaltered. Required for residual coding, for some modes of
  // it at least.
  virtual void RGB2RGB(const RectAngle<LONG> &,const struct ImageBitMap *const *,
		       Buffer)
  {
    // This does not implement residual coding, code should not go here.
    JPG_THROW(INVALID_PARAMETER,"LSLosslessTrafo::RGB2RGB",
	      "JPEG LS lossless color transformation does not allow residual coding");
  }
  //
  // In case the user already provided a tone-mapped version of the image, this call already
  // takes the LDR version of the image, performs no tone-mapping but only a color
  // decorrelation transformation and injects it as LDR image.
  virtual void LDRRGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 Buffer target)
  {
    // There is no tonemapping anyhow...
    RGB2YCbCr(r,source,target);
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
  //
  // Return the number of fractional bits this color transformation requires.
  // None, this is integer to integer.
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
};
///

///
#endif
