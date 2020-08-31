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
** This class builds the proper color transformer from the information
** in the MergingSpecBox
**
** $Id: colortransformerfactory.hpp,v 1.24 2017/11/28 13:08:07 thor Exp $
**
*/

#ifndef COLORTRAFO_COLORTRANSFORMERFACTORY_HPP
#define COLORTRAFO_COLORTRANSFORMERFACTORY_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
///

/// Forwards
class MergingSpecBox;
class Tables;
class Frame;
class ParametricToneMappingBox;
class IntegerTrafo;
class FloatTrafo;
///

/// class ColorTransformerFactory
// This class builds the proper color transformer from the information
// in the MergingSpecBox
class ColorTransformerFactory : public JKeeper {
  //
  // The color transformation itself - is kept here.
  class ColorTrafo                 *m_pTrafo;
  //
  // Additional coding tables containing the LUTs required
  // for the color transformation.
  class Tables                     *m_pTables;
  //
  // The identity tone mapper that is used for the L-Lut in case no specs are there.
  // This is the identity transformation that preserves the neutral value.
  class Box                        *m_pIdentity0;
  //
  // This is the identity transformation that preserves min and max.
  class Box                        *m_pIdentity1;
  //
  // The zero-mapping for the R-tables in case the scaling type is set to zero.
  class Box                        *m_pZero;
  //
  // Given a LUT index, construct the tone mapping represenging it.
  // The e parameter defines the rounding mode for the default table. 
  // For e=0, the neutral value is preserved, for e=1, the extremes are preserved.
  class ToneMapperBox *FindToneMapping(UBYTE idx,UBYTE e);
  //
  // Build transformations that require only L and R.
  class IntegerTrafo *BuildIntegerTransformation(UBYTE etype,class Frame *frame,class Frame *residualframe,
                                                 class MergingSpecBox *specs,
                                                 UBYTE ocflags,int ltrafo,int rtrafo);
  // 
  //
  // Build a transformation using the JPEG-LS color transformation back-end.
  // This works only without a residual (why a residual anyhow?)
  class ColorTrafo *BuildLSTransformation(UBYTE type,
                                          class Frame *frame,class Frame *residualframe,
                                          class MergingSpecBox *,
                                          UBYTE ocflags,int ltrafo,int rtrafo);
  //
  //
  // More helpers to cut down the possibilities a bit.
  template<int count,typename type>
  IntegerTrafo *BuildIntegerTransformationSimple(class Frame *frame,class Frame *residualframe,
                                                       class MergingSpecBox *specs,
                                                       UBYTE ocflags,int ltrafo,int rtrafo);
  //
  template<typename type>
  IntegerTrafo *BuildIntegerTransformationFour(class Frame *frame,
                                               class Frame *residualframe,
                                               class MergingSpecBox *,
                                               UBYTE oc,int ltrafo,int rtrafo);
  //
  template<int count,typename type>
  IntegerTrafo *BuildIntegerTransformationExtensive(class Frame *frame,class Frame *residualframe,
                                                          class MergingSpecBox *specs,
                                                          UBYTE ocflags,int ltrafo,int rtrafo);
  //
  // Install the parameters to fully define a profile C encoder/decoder
  void InstallIntegerParameters(class IntegerTrafo *trafo,
                                class MergingSpecBox *specs,
                                int count,bool encoding,bool residual,
                                UBYTE inbpp,UBYTE outbpp,UBYTE resbpp,UBYTE rbits,
                                MergingSpecBox::DecorrelationType ltrafo,
                                MergingSpecBox::DecorrelationType rtrafo,
                                MergingSpecBox::DecorrelationType ctrafo);
  //
  //
  // Fill in a default matrix from its decorrelation type. This is the fixpoint version.
  void GetStandardMatrix(MergingSpecBox::DecorrelationType dt,LONG matrix[9]);
  //
  //
  // Return the inverse of a standard matrix in fixpoint.
  void GetInverseStandardMatrix(MergingSpecBox::DecorrelationType dt,LONG matrix[9]);
  //
  //
  //
public:
  // Build a color transformation factory - requires the tables
  // that contain most of the data.
  ColorTransformerFactory(class Tables *tables);
  //
  // Destroy the factory and all that it produced.
  ~ColorTransformerFactory(void);
  //
  // Build a color transformer from the merging specifications
  // passed in. These might be NULL in case there is none and
  // the JPEG stream is non-extended. Also requires the number
  // of components.
  // Residual may be NULL if there is no residual frame.
  // Returns the color transformation class.
  // Note that there is at most one color transformer in the system
  // (there are no tiles here as in JPEG 2000).
  // dctbits is the number of bits "closer to the DCT side", i.e. on decoding, the input bits,
  // spatialbits is the bit precision "closer to the image", i.e. on decoding, the output bits.
  class ColorTrafo *BuildColorTransformer(class Frame *frame,class Frame *residual,
                                          class MergingSpecBox *specs,
                                          UBYTE dctbits,UBYTE spatialbits,
                                          UBYTE external_type,bool encoding,
                                          bool disabletorgb);
  //
};
///

///
#endif
