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
** This class is the base class for all floating-point based transformers,
** most notably profiles A and B of 18477-7.
**
** $Id: floattrafo.hpp,v 1.3 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef COLORTRAFO_FLOATTRAFO_HPP
#define COLORTRAFO_FLOATTRAFO_HPP

/// Includes
#include "colortrafo/colortrafo.hpp"
#include "boxes/parametrictonemappingbox.hpp"
#include "tools/traits.hpp"
#include "std/string.hpp"
///

/// FloatTrafo
class FloatTrafo : public ColorTrafo {
  //
protected:
  //
  // The output transformations. These are always parametric.
  // They implement the last step in the decoding of the samples.
  class ParametricToneMappingBox *m_pOutputTrafo[4];
  //
  // Secondary base and residual transformations. These are the
  // second to last step, just one step before the merging takes
  // place.
  class ParametricToneMappingBox *m_pSecondBase[4];
  class ParametricToneMappingBox *m_pSecondResidual[4];
  //
  // The intermediate residual transformation. This goes between
  // the residual color (not decorrelation) transformation and the 
  // second residual transformation. It is typically a gamma map
  // or an identity.
  class ParametricToneMappingBox *m_pIntermediateResidual[4];
  //
  // The prescaling transformation. This transformation computes the
  // from the luminance of the precursor image a scale factor for the
  // luminance components of the residual.
  class ParametricToneMappingBox *m_pPrescalingLUT;
  //
  // The postscaling transformation. This transformation computes from
  // the luminance of the residual a scale factor for the entire image.
  class ParametricToneMappingBox *m_pDiagonalLUT;
  //
  // This also exists as a lookup table for decoding.
  const FLOAT                    *m_pfDecodingDiagonalLUT;
  //
  // The scaling transformation (Q-transformation) that relates the
  // decoded residual samples with the correction factors for the chroma
  // components. The output of these scaling factors are the input to
  // the scaling procedure by nu.
  class ParametricToneMappingBox *m_pResidualLUT[4];
  //
  // The "L transformation" or base transformation that computes from the
  // samples in the legacy domain a linear luminance image. This exists
  // as a 256-byte lookup table and as a parametric version. It is typically
  // a gamma mapping.
  class ParametricToneMappingBox *m_pDecoding[4];
  FLOAT                          *m_pfDecodingLUT[4]; 
  //
  // The color transformation in the legacy coding path. Usually an
  // identity or a transformation from 601 colors to a larger gammut
  // color space.
  FLOAT                           m_fC[9];
  //
  // The inverse (encoding) version of the larger-gammut to 601-color
  // colorspace.
  FLOAT                           m_fInvC[9];
  //
  // The decoding version of the R-transformation, i.e. the color
  // transformation in the residual coding path. This is typically 
  // the usual ICT, YCbCr to 601-RGB.
  FLOAT                           m_fR[9];
  //
  // The inverse (encoding) version of the R-transformation. These are the
  // coefficients from RGB to 601 (or something related to that.
  FLOAT                           m_fRInv[9];
  //
  // The prescaling linear transformation, prescaling matrix: This is
  // used to compute the luminance from the three components of the precursor
  // image.
  FLOAT                           m_fP[9];
  //
public:
  FloatTrafo(class Environ *env,LONG dcshift,LONG max,LONG rdcshift,LONG rmax,LONG outshift,LONG outmax)
    : ColorTrafo(env,dcshift,max,rdcshift,rmax,outshift,outmax)
  { }
  //
  virtual ~FloatTrafo(void)
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
  virtual UBYTE PixelTypeOf(void) const
  {
    return TypeTrait<FLOAT>::TypeID; // this is always FLOAT.
  }
  //
  // Various methods to define the paremeters of this transformation.
  //
  // Define the base transformation, L-Transformation in the legacy domain.
  // This defines the transformation as parametric curve - this is only required
  // for encoding.
  void DefineBaseTransformation(class ParametricToneMappingBox *curves[4])
  {
    memcpy(m_pDecoding,curves,sizeof(m_pDecoding));
  }
  //
  // Define the base transformation, L-Transformation in the legacy domain as a lookup
  // table. This is necessary for encoding and decoding.
  void DefineBaseTransformation(const FLOAT *luts[4])
  {
    memcpy(m_pfDecodingLUT,luts,sizeof(m_pfDecodingLUT));
  }
  //
  // Define the residual NLT transformation, Q-Transformation in the residual
  // domain. This is always a curve for profiles A and B.
  void DefineResidualTransformation(class ParametricToneMappingBox *curves[4])
  {
    memcpy(m_pResidualLUT,curves,sizeof(m_pResidualLUT));
  }
  //
  // Define the second base transformation. This is either the identity and
  // usused for profile A, or it is a log map for profile B.
  void DefineSecondBaseTransformation(class ParametricToneMappingBox *curves[4])
  {
    memcpy(m_pSecondBase,curves,sizeof(m_pSecondBase));
  }
  //
  // Define the second residual transformation. This is also either the identity
  // and unused for profile A, or it is a log map for profile B.
  void DefineSecondResidualTransformation(class ParametricToneMappingBox *curves[4])
  {
    memcpy(m_pSecondResidual,curves,sizeof(m_pSecondResidual));
  }
  //
  // Define the intermediate residual transformation. This is either the identity for
  // profile A, or it is a gamma map for profile B.
  void DefineIntermediateResidualTransformation(class ParametricToneMappingBox *curves[4])
  {
    memcpy(m_pIntermediateResidual,curves,sizeof(m_pIntermediateResidual));
  }
  //
  // Define the prescaling transformation to compute \nu from the luminance of the
  // precursor image.
  void DefinePrescalingTransformation(class ParametricToneMappingBox *curve)
  {
    m_pPrescalingLUT = curve;
  }
  //
  // Define the postscaling transformation that computes \mu from the luminance of
  // the residual image. This is the encoder version which requires a parametric
  // curve. Decoding is good enough with a LUT.
  void DefinePostscalingTransformation(class ParametricToneMappingBox *curve)
  {
    m_pDiagonalLUT = curve;
  }
  //
  // Defines the output conversion that computes the final output value from the
  // merged ldr/hdr sample value.
  void DefineOutputTransformation(class ParametricToneMappingBox *curves[4])
  {
    memcpy(m_pOutputTrafo,curves,sizeof(m_pOutputTrafo));
  }
  //
  // Define the postscaling transformation as a floating-point based lookup table.
  void DefinePostscalingTransformation(const FLOAT *lut)
  {
    m_pfDecodingDiagonalLUT = lut;
  }
  //
  // Defines the residual transformation. This is the transformation from the Q-values
  // to the residual color space. It is typically just the ICT (YCbCr to RGB), but it
  // can be something different in profile B.
  void DefineResidualDecodingMatrix(const FLOAT trafo[9])
  {
    memcpy(m_fR,trafo,sizeof(m_fR));
  }
  //
  // Define the inverse of the R-matrix, for encoding.
  void DefineResidualEncodingMatrix(const FLOAT trafo[9])
  {
    memcpy(m_fRInv,trafo,sizeof(m_fRInv));
  }
  //
  // Define the color transformation for decoding. This linear transformation (matrix) transforms the 
  // 601-colors of the base transformation to the extended color gammut of the HDR image.
  void DefineColorDecodingMatrix(const FLOAT trafo[9])
  {
    memcpy(m_fC,trafo,sizeof(m_fC));
  }
  //
  // Define the color transformation for encoding. This is the inverse of the C matrix. 
  void DefineColorEncodingMatrix(const FLOAT trafo[9])
  {
    memcpy(m_fInvC,trafo,sizeof(m_fInvC));
  }
  //
  // Define the prescaling matrix. This matrix (or rather, vector, as only the first three
  // components are used) computes the luminance from the input vector, the precursor image,
  // and computes from that a prescaling factor.
  void DefinePrescalingMatrix(const FLOAT trafo[9])
  {
    memcpy(m_fP,trafo,sizeof(m_fP));
  }
};
///

///
#endif
