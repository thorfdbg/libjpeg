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
** This box defines the parameters for the LDR/residual merging process.
**
** $Id: mergingspecbox.cpp,v 1.33 2015/03/13 15:14:06 thor Exp $
**
*/

/// Includes
#include "boxes/mergingspecbox.hpp"
#include "boxes/namespace.hpp"
#include "boxes/parametrictonemappingbox.hpp"
#include "boxes/refinementspecbox.hpp"
#include "boxes/outputconversionbox.hpp"
#include "boxes/dctbox.hpp"
#include "boxes/colortrafobox.hpp"
#include "boxes/nonlineartrafobox.hpp"
#include "boxes/matrixbox.hpp"
#include "boxes/lineartransformationbox.hpp"
#include "boxes/floattransformationbox.hpp"
#include "tools/numerics.hpp"
#include "codestream/tables.hpp"
#include "io/bytestream.hpp"
#include "io/decoderstream.hpp"
#include "io/memorystream.hpp"
#include "std/math.hpp"
///


/// MergingSpecBox::MergingSpecBox
// Create a new Merging Specification Box
MergingSpecBox::MergingSpecBox(class Tables *tables,class Box *&boxlist,ULONG boxtype)
  : SuperBox(tables->EnvironOf(),boxlist,boxtype), m_pNameSpace(NULL),
    m_pRefinementSpec(NULL), m_pOutputConversion(NULL),
    m_pBaseDCT(NULL), m_pBaseTransformation(NULL), m_pBaseNonlinearity(NULL), 
    m_pColorTransformation(NULL), m_p2ndBaseNonlinearity(NULL),
    m_pResidualDCT(NULL), m_pResidualNonlinearity(NULL), m_pResidualTransformation(NULL),
    m_pIntermediateResidualNonlinearity(NULL), m_pResidualColorTransformation(NULL), 
    m_p2ndResidualNonlinearity(NULL), 
    m_pPrescalingTransformation(NULL), m_pPrescalingNonlinearity(NULL), 
    m_pPostscalingNonlinearity(NULL), m_pAlphaMode(NULL)
{ 
  switch(boxtype) {
  case SpecType:
    m_pNameSpace = tables->ImageNamespace();
    break;
  case AlphaType:
    m_pNameSpace = tables->AlphaNamespace();
    break;
  default:
    assert(!"found illegal type for MergingSpecBox");
    break;
  }
  assert(m_pNameSpace);
  
  SuperBox::RegisterNameSpace(m_pNameSpace);
}
///

/// MergingSpecBox::~MergingSpecBox
MergingSpecBox::~MergingSpecBox(void)
{
  // the subbox list is automatically deleted by the superbox.
}
///

/// MergingSpecBox::CreateBox
// Create a new subbox of the subox.
class Box *MergingSpecBox::CreateBox(class Box *&boxlist,ULONG tbox)
{
  switch(tbox) {
  case RefinementSpecBox::Type:
    if (m_pRefinementSpec != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double refinement specification box");
    return m_pRefinementSpec = new(m_pEnviron) class RefinementSpecBox(m_pEnviron,boxlist);
  case OutputConversionBox::Type:
    if (m_pOutputConversion != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double output conversion box");
    return m_pOutputConversion = new (m_pEnviron) class OutputConversionBox(m_pEnviron,boxlist);
    //
  case DCTBox::Base_Type:
    if (m_pBaseDCT != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double base DCT box");
    return m_pBaseDCT = new (m_pEnviron) class DCTBox(m_pEnviron,boxlist,tbox);
  case ColorTrafoBox::Base_Type:
    if (m_pBaseTransformation != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double base transformation box");
    return m_pBaseTransformation = new (m_pEnviron) class ColorTrafoBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::Base_Type:
    if (m_pBaseNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double base non-linear point transformation box");
    return m_pBaseNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);
  case ColorTrafoBox::Color_Type:
    if (m_pColorTransformation != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double color transformation box");
    return m_pColorTransformation = new (m_pEnviron) class ColorTrafoBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::Base2_Type:
    if (m_p2ndBaseNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double secondary base non-linear point transformation box");
    return m_p2ndBaseNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);
    //
  case DCTBox::Residual_Type:
    if (m_pResidualDCT != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double residual DCT box");
    return m_pResidualDCT = new (m_pEnviron) class DCTBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::Residual_Type:
    if (m_pResidualNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double residual non-linear point transformation box");
    return m_pResidualNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);
  case ColorTrafoBox::Residual_Type:
    if (m_pResidualTransformation != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double residual transformation box");
    return m_pResidualTransformation = new (m_pEnviron) class ColorTrafoBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::ResidualI_Type:
    if (m_pIntermediateResidualNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double intermediate residual non-linear point transformation box");
    return m_pIntermediateResidualNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);
  case ColorTrafoBox::ResidualColor_Type:
    if (m_pResidualColorTransformation != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double residual transformation box");
    return m_pResidualColorTransformation = new (m_pEnviron) class ColorTrafoBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::Residual2_Type:
    if (m_p2ndResidualNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double secondary residual non-linear point transformation box");
    return m_p2ndResidualNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);  
    //
  case ColorTrafoBox::Prescaling_Type:
    if (m_pPrescalingTransformation != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double prescaling transformation box");
    return m_pPrescalingTransformation = new (m_pEnviron) class ColorTrafoBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::Prescaling_Type:
    if (m_pPrescalingNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double prescaling non-linear point transformation box");
    return m_pPrescalingNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);
  case NonlinearTrafoBox::Postscaling_Type:
    if (m_pPostscalingNonlinearity != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double postscaling non-linear point transformation box");
    return m_pPostscalingNonlinearity = new (m_pEnviron) class NonlinearTrafoBox(m_pEnviron,boxlist,tbox);
    //
  case ParametricToneMappingBox::Type:
    // There can be multiple boxes of this type here in.
    return new(m_pEnviron) ParametricToneMappingBox(m_pEnviron,boxlist);
  case LinearTransformationBox::Type:
    return new(m_pEnviron) LinearTransformationBox(m_pEnviron,boxlist);
  case FloatTransformationBox::Type:
    return new(m_pEnviron) FloatTransformationBox(m_pEnviron,boxlist); 
  case AlphaBox::Type:
    if (m_pAlphaMode != NULL)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found a double alpha channel composition box");
    if (BoxTypeOf() != AlphaType)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::CreateBox",
                "Malformed JPEG stream - found an alpha channel composition box outside of "
                "the alpha channel merging specification box");
    return m_pAlphaMode = new(m_pEnviron) class AlphaBox(m_pEnviron,boxlist);
  default:
    // All other box types are ignored.
    return NULL;
  }
}
///

/// MergingSpecBox::AcknowledgeBox
// This is called as soon as a new box is created. It sorts the box into the list
// of available boxes if necessary.
// Inform the superbox that the box is now created. Does nothing by default,
// can be overloaded to sort the new box in.
void MergingSpecBox::AcknowledgeBox(class Box *box,ULONG tbox)
{
  //
  // Only parametric tone mapping boxes need to be sorted in here.
  //
  assert(m_pNameSpace);
  //
  switch(tbox) {
  case ParametricToneMappingBox::Type:
    {
      class ParametricToneMappingBox *curve = (class ParametricToneMappingBox *)box;
      UBYTE idx = curve->TableDestinationOf();
      //
      // Note that the box is already enqueued in the box list, thus it is not lost.
      if (!m_pNameSpace->isUniqueNonlinearity(idx))
        JPG_THROW(MALFORMED_STREAM,"SuperBox::AcknowledgeBox",
                  "Malformed JPEG stream - found an double parametric curve box for the same index");
    }
    break;
  case LinearTransformationBox::Type:
  case FloatTransformationBox::Type:
    {
      class MatrixBox *matrix = (class MatrixBox *)box;
      UBYTE idx = matrix->IdOf();
      //
      if (!m_pNameSpace->isUniqueMatrix(idx))
        JPG_THROW(MALFORMED_STREAM,"SuperBox::AcknowledgeBox",
                  "Malformed JPEG stream - found an double linear transformation for the same index");
    }
    break;
  default:
    break;
  }
}
///

/// MergingSpecBox::DefineHiddenBits
// Define the number of hidden refinement bits in the legacy stream.
void MergingSpecBox::DefineHiddenBits(UBYTE hiddenbits)
{
  assert(hiddenbits <= 4);

  if (hiddenbits > 0 && m_pRefinementSpec == NULL)
    SuperBox::CreateBox(RefinementSpecBox::Type);
  if (m_pRefinementSpec)
    m_pRefinementSpec->DefineBaseRefinementScans(hiddenbits);
}
///

/// MergingSpecBox::HiddenBits
// Return the number of hidden DCT bits.
UBYTE MergingSpecBox::HiddenBitsOf(void) const
{
  if (m_pRefinementSpec)
    return m_pRefinementSpec->BaseRefinementScansOf();

  return 0;
}
///

/// MergingSpecBox::DefineHiddenResidualBits
// Define the number of hidden bits in the residual stream
void MergingSpecBox::DefineHiddenResidualBits(UBYTE hiddenbits)
{
  assert(hiddenbits <= 4); 

  if (hiddenbits > 0 && m_pRefinementSpec == NULL)
    SuperBox::CreateBox(RefinementSpecBox::Type);
  if (m_pRefinementSpec)
    m_pRefinementSpec->DefineResidualRefinementScans(hiddenbits);
}
///

/// MergingSpecBox::HiddenResidualBitsOf
// Return the number of hidden residual bits.
UBYTE MergingSpecBox::HiddenResidualBitsOf(void) const
{
  if (m_pRefinementSpec)
    return m_pRefinementSpec->ResidualRefinementScansOf();
  
  return 0;
}
///

/// MergingSpecBox::DefineResidualBits
// Define the additional number of bits in the spatial domain.
// These are the number of bits on top of the bits in the legacy
// domain, made available by other means. The total bit precision
// of the image is r_b + 8.
void MergingSpecBox::DefineResidualBits(UBYTE residualbits)
{
  assert(residualbits <= 8);
  
  if (residualbits > 0 && m_pOutputConversion == NULL)
    SuperBox::CreateBox(OutputConversionBox::Type);
  if (m_pOutputConversion)
    m_pOutputConversion->DefineResidualBits(residualbits);
}
///

/// MergingSpecBox::ResidualBitsOf
// Return the additional number of bits in the spatial domain.
UBYTE MergingSpecBox::ResidualBitsOf(void) const
{
  if (m_pOutputConversion)
    return m_pOutputConversion->ResidualBitsOf();

  return 0;
}
///

/// MergingSpecBox::DefineLTable
// Define the LUT for the component comp in the L-Tables,
// requires also the LUT to use and the scaling method for
// the LUT. 
void MergingSpecBox::DefineLTable(UBYTE component,UBYTE tableidx)
{
  assert(component < 4);

  if (m_pBaseNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::Base_Type);

  m_pBaseNonlinearity->DefineTransformationIndex(component,tableidx);
}
///

/// MergingSpecBox::LTableIndexOf
// Return the LUT table index for the L-table for the given component
// and the type of how to apply it. Return MAX_UBYTE in case the
// base transformation is not used.
UBYTE MergingSpecBox::LTableIndexOf(UBYTE component) const
{
  assert(component < 4);

  if (m_pBaseNonlinearity)
    return m_pBaseNonlinearity->TransformationIndexOf(component);

  return MAX_UBYTE;
} 
///

/// MergingSpecBox::DefineLTransformation
// Define the L-multi-component decorrelation transformation.
// This is the LDR decorrelation transformation.
void MergingSpecBox::DefineLTransformation(DecorrelationType method)
{
  assert(method < 16);

  if (m_pBaseTransformation == NULL)
    SuperBox::CreateBox(ColorTrafoBox::Base_Type);

  if (m_pBaseTransformation)
    m_pBaseTransformation->DefineTransformationIndex(method);
}
///

/// MergingSpecBox::LTransformationOf
// Return the L transformation type.
MergingSpecBox::DecorrelationType MergingSpecBox::LTransformationOf(void) const
{
  if (m_pBaseTransformation)
    return DecorrelationType(m_pBaseTransformation->TransformationIndexOf());

  return Undefined;
}
///

/// MergingSpecBox::DefineQTable
// Define the LUT for the component comp in the Q-Tables,
// requires also the LUT to use and the scaling method for
// the LUT.
void MergingSpecBox::DefineQTable(UBYTE component,UBYTE tableidx)
{
  assert(component < 4);

  if (m_pResidualNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::Residual_Type);

  m_pResidualNonlinearity->DefineTransformationIndex(component,tableidx);
}
///

/// MergingSpecBox::QTableIndexOf
// Return the LUT table index for the Q-table for the given component
// or return MAX_UBYTE in case the table is not defined.
UBYTE MergingSpecBox::QTableIndexOf(UBYTE component) const
{
  assert(component < 4);

  if (m_pResidualNonlinearity)
    return m_pResidualNonlinearity->TransformationIndexOf(component);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::DefineRTable
// Define the LUT for the component comp in the R-Tables,
// requires also the LUT to use and the scaling method for
// the LUT.
void MergingSpecBox::DefineRTable(UBYTE component,UBYTE tableidx)
{
  assert(component < 4);

  if (m_pIntermediateResidualNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::ResidualI_Type);

  m_pIntermediateResidualNonlinearity->DefineTransformationIndex(component,tableidx);
}
///

/// MergingSpecBox::RTableIndexOf
// Return the LUT table index for the R-table for the given component
// or return MAX_UBYTE in case the table is not defined.
UBYTE MergingSpecBox::RTableIndexOf(UBYTE component) const
{
  assert(component < 4);

  if (m_pIntermediateResidualNonlinearity)
    return m_pIntermediateResidualNonlinearity->TransformationIndexOf(component);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::DefineRTransformation
// Define the R-multi-component decorrelation transformation.
void MergingSpecBox::DefineRTransformation(DecorrelationType method)
{
  assert(method < 16);

  if (m_pResidualTransformation == NULL)
    SuperBox::CreateBox(ColorTrafoBox::Residual_Type);

  if (m_pResidualTransformation)
    m_pResidualTransformation->DefineTransformationIndex(method);
}
///

/// MergingSpecBox::RTransformationOf
// Return the R transformation type.
MergingSpecBox::DecorrelationType MergingSpecBox::RTransformationOf(void) const
{
  if (m_pResidualTransformation)
    return DecorrelationType(m_pResidualTransformation->TransformationIndexOf());

  return Undefined;
}
///

/// MergingSpecBox::DefineCTransformation
// Define the C-multi-component decorrelation transformation.
// This is the LDR decorrelation transformation.
void MergingSpecBox::DefineCTransformation(DecorrelationType method)
{
  assert(method < 16);

  if (m_pColorTransformation == NULL)
    SuperBox::CreateBox(ColorTrafoBox::Color_Type);

  if (m_pColorTransformation)
    m_pColorTransformation->DefineTransformationIndex(method);
}
///

/// MergingSpecBox::CTransformationOf
// Return the C transformation type.
MergingSpecBox::DecorrelationType MergingSpecBox::CTransformationOf(void) const
{
  if (m_pColorTransformation)
    return DecorrelationType(m_pColorTransformation->TransformationIndexOf());

  return Undefined;
}
///

/// MergingSpecBox::DefineDTransformation
// Define the D-multi-component decorrelation transformation.
// This is the residual color transformation.
void MergingSpecBox::DefineDTransformation(DecorrelationType method)
{
  assert(method < 16);

  if (m_pResidualColorTransformation == NULL)
    SuperBox::CreateBox(ColorTrafoBox::ResidualColor_Type);

  if (m_pResidualColorTransformation)
    m_pResidualColorTransformation->DefineTransformationIndex(method);
}
///

/// MergingSpecBox::DTransformationOf
// Return the D transformation type.
MergingSpecBox::DecorrelationType MergingSpecBox::DTransformationOf(void) const
{
  if (m_pResidualColorTransformation)
    return DecorrelationType(m_pResidualColorTransformation->TransformationIndexOf());

  return Undefined;
}
///

/// MergingSpecBox::DefineLDCTProcess
// Define the DCT process in the L-chain. 
void MergingSpecBox::DefineLDCTProcess(DCTBox::DCTType dct)
{
  if (m_pBaseDCT == NULL)
    SuperBox::CreateBox(DCTBox::Base_Type);

  m_pBaseDCT->DefineDCT(dct);
}
///

/// MergingSpecBox::LDCTProcessOf
// Return the DCT process for component i.
DCTBox::DCTType MergingSpecBox::LDCTProcessOf(void) const
{
  if (m_pBaseDCT)
    return m_pBaseDCT->DCTTypeOf();

  // The default is the FDCT.
  return DCTBox::FDCT;
}
///

/// MergingSpecBox::DefineRDCTProcess
// Define the DCT process in the R-chain. 
void MergingSpecBox::DefineRDCTProcess(DCTBox::DCTType dct)
{
  if (m_pResidualDCT == NULL)
    SuperBox::CreateBox(DCTBox::Residual_Type);

  m_pResidualDCT->DefineDCT(dct);
}
///

/// MergingSpecBox::RDCTProcessOf
// Return the DCT process for component i.
DCTBox::DCTType MergingSpecBox::RDCTProcessOf(void) const
{
  if (m_pResidualDCT)
    return m_pResidualDCT->DCTTypeOf();

  // The default is the FDCT.
  return DCTBox::FDCT;
}
///

/// MergingSpecBox::DefineNoiseShaping
// Define the noise shaping in the R-channel.
void MergingSpecBox::DefineNoiseShaping(bool enable_noise_shaping)
{ 
  if (m_pResidualDCT == NULL)
    SuperBox::CreateBox(DCTBox::Residual_Type);

  m_pResidualDCT->DefineNoiseShaping(enable_noise_shaping);
}
///

/// MergingSpecBox::isNoiseShapingEnabled
// Check whether the residual data of component i undergoes noise shaping.
bool MergingSpecBox::isNoiseShapingEnabled(void) const
{
  if (m_pResidualDCT)
    return m_pResidualDCT->isNoiseShapingEnabled();

  return false;
}
///

/// MergingSpecBox::DefineLossless
// Specify whether the process is lossy or not.
void MergingSpecBox::DefineLossless(bool lossless)
{
  if (m_pOutputConversion == NULL)
    SuperBox::CreateBox(OutputConversionBox::Type);

  m_pOutputConversion->DefineLossless(lossless);
}
///

/// MergingSpecBox::isLossless
// Return the state of the lossless flag.
bool MergingSpecBox::isLossless(void) const
{
  if (m_pOutputConversion)
    return m_pOutputConversion->isLossless();

  return false;
}
///

/// MergingSpecBox::DefineOutputConversion
// Set whether the output shall be casted from int to float.
void MergingSpecBox::DefineOutputConversion(bool convert)
{
  if (m_pOutputConversion == NULL)
    SuperBox::CreateBox(OutputConversionBox::Type);

  m_pOutputConversion->DefineOutputConversion(convert);
}
///

/// MergingSpecBox::usesOutputConversion
// Check whether the encoded data uses output conversion from int to IEEE half float.
bool MergingSpecBox::usesOutputConversion(void) const
{
  if (m_pOutputConversion)
    return m_pOutputConversion->usesOutputConversion();

  return false;
}
///

/// MergingSpecBox::DefineClipping
// Specify whether the output shall be clipped to range.
void MergingSpecBox::DefineClipping(bool convert)
{
  if (m_pOutputConversion == NULL)
    SuperBox::CreateBox(OutputConversionBox::Type);

  m_pOutputConversion->DefineClipping(convert);
}
///

/// MergingSpecBox::usesClipping
// Check whether the encoded data uses clipping. Required for int coding.
bool MergingSpecBox::usesClipping(void) const
{
  if (m_pOutputConversion)
    return m_pOutputConversion->usesClipping();

  return true;
}
///

/// MergingSpecBox::DefineOutputConversionTable
// Defines the output conversion from a Lookup table index.
void MergingSpecBox::DefineOutputConversionTable(UBYTE component,UBYTE table)
{
  if (m_pOutputConversion == NULL)
    SuperBox::CreateBox(OutputConversionBox::Type);

  if (m_pOutputConversion)
    m_pOutputConversion->DefineOutputConversionTable(component,table);
}
///

/// MergingSpecBox::OutputConversionLookupOf
// Returns the output conversion for the indicated component or MAX_UBYTE in case
// this table is not used.
UBYTE MergingSpecBox::OutputConversionLookupOf(UBYTE component) const
{
  if (m_pOutputConversion)
    return m_pOutputConversion->OutputConversionLookupOf(component);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::DefinePTransformation
// Define the P transformation type. This is the
// color transformation that, for profile A, computes the luminance
// from the precursor image.
void MergingSpecBox::DefinePTransformation(DecorrelationType method)
{ 
  assert(method < 16);

  if (m_pPrescalingTransformation == NULL)
    SuperBox::CreateBox(ColorTrafoBox::Prescaling_Type);
  
  if (m_pPrescalingTransformation)
    m_pPrescalingTransformation->DefineTransformationIndex(method);
}
///

/// MergingSpecBox::PTransformationOf
// Return the P transformation type or MAX_UYTE in case it is not
// defined. This is the transformation that computes from the
// precursor image a luminance value for scaling the chrominance
// residuals.
MergingSpecBox::DecorrelationType MergingSpecBox::PTransformationOf(void) const
{
  if (m_pPrescalingTransformation)
    return DecorrelationType(m_pPrescalingTransformation->TransformationIndexOf());

  return Undefined;
}
///

/// MergingSpecBox::DefineL2Table
// Define the curve/entry for the second base transformation.
void MergingSpecBox::DefineL2Table(UBYTE comp,UBYTE tableidx)
{
  if (m_p2ndBaseNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::Base2_Type);

  if (m_p2ndBaseNonlinearity)
    m_p2ndBaseNonlinearity->DefineTransformationIndex(comp,tableidx);
}
///

/// MergingSpecBox::L2TableIndexOf
// Return the table index for the 2nd base transformation table or
// MAX_UBYTE in case it is not defined.
UBYTE MergingSpecBox::L2TableIndexOf(UBYTE comp) const
{
  if (m_p2ndBaseNonlinearity)
    return m_p2ndBaseNonlinearity->TransformationIndexOf(comp);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::DefineR2Table
// Define the curve/entry for the second residual transformation.
void MergingSpecBox::DefineR2Table(UBYTE comp,UBYTE tableidx)
{
  if (m_p2ndResidualNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::Residual2_Type);

  if (m_p2ndResidualNonlinearity)
    m_p2ndResidualNonlinearity->DefineTransformationIndex(comp,tableidx);
}
///

/// MergingSpecBox::R2TableIndexOf
// Return the table index for the 2nd residual transformation table or
// MAX_UBYTE in case it is not defined.
UBYTE MergingSpecBox::R2TableIndexOf(UBYTE comp) const
{
  if (m_p2ndResidualNonlinearity)
    return m_p2ndResidualNonlinearity->TransformationIndexOf(comp);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::DefinePTable
// Defines the nonlinearity for the prescaling transformation. This goes for
// all components, i.e. there is only a single table.
void MergingSpecBox::DefinePTable(UBYTE tableidx)
{
  if (m_pPrescalingNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::Prescaling_Type);

  if (m_pPrescalingNonlinearity)
    m_pPrescalingNonlinearity->DefineTransformationIndex(0,tableidx);
}
///

/// MergingSpecBox::PTableIndexOf
// Return the table index of the pre-scaling non-linearity or MAX_UBYTE
// in case this table is not defined.
UBYTE MergingSpecBox::PTableIndexOf(void) const
{
  if (m_pPrescalingNonlinearity)
    return m_pPrescalingNonlinearity->TransformationIndexOf(0);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::DefineSTable
// Defines the non-linearity for the postscaling transformation. This
// transformation computes the luminance scale factor for all components
// after merging the specs.
void MergingSpecBox::DefineSTable(UBYTE tableidx)
{
  if (m_pPostscalingNonlinearity == NULL)
    SuperBox::CreateBox(NonlinearTrafoBox::Postscaling_Type);

  if (m_pPostscalingNonlinearity)
    m_pPostscalingNonlinearity->DefineTransformationIndex(0,tableidx);
}
///

/// MergingSpecBox::STableIndexOf
// Return the table indexof the post-scaling non-linearity or MAX_UBYTE
// in case this table is not defined. This
// transformation computes the luminance scale factor for all components
// after merging the specs.
UBYTE MergingSpecBox::STableIndexOf(void) const
{
  if (m_pPostscalingNonlinearity)
    return m_pPostscalingNonlinearity->TransformationIndexOf(0);

  return MAX_UBYTE;
}
///

/// MergingSpecBox::isProfileA
// Check whether this is (likely) a profile A codec.
bool MergingSpecBox::isProfileA(void) const
{
  // Say it's profile A if we have the diagonal transformations.
  if (m_pPostscalingNonlinearity || m_pPrescalingNonlinearity || m_pPrescalingTransformation)
    return true;

  return false;
}
///

/// MergingSpecBox::isProfileB
// Check whether this is likely a profile B codec.
bool MergingSpecBox::isProfileB(void) const
{
  // Say it's profile B if we have the L2 transformations
  if (m_p2ndBaseNonlinearity) /* || m_p2ndResidualNonlinearity) */
    return true;

  return false;
}
///


/// MergingSpecBox::CreatesRGBCurve
// Create an sRGB type nonlinearity for the given table index.
UBYTE MergingSpecBox::CreatesRGBCurve(UBYTE e,FLOAT p1,FLOAT p2,FLOAT p3)
{
  const class ParametricToneMappingBox *ccurve;
  
  ccurve = m_pNameSpace->FindNonlinearity(ParametricToneMappingBox::Gamma,e,p1,p2,p3);
  
  if (ccurve == NULL) {
    class ParametricToneMappingBox *curve;
    UBYTE nextid         = m_pNameSpace->AllocateNonlinearityID();

    curve = (class ParametricToneMappingBox *)SuperBox::CreateBox(ParametricToneMappingBox::Type);
    assert(curve);
    
    curve->DefineTable(nextid,ParametricToneMappingBox::Gamma,e,p1,p2,p3);
    
    ccurve = curve;
  }

  return ccurve->TableDestinationOf();
}
///

/// MergingSpecBox::CreateLinearRamp
// Create a linear ramp (non-)linearity from the points P1 to P2.
UBYTE MergingSpecBox::CreateLinearRamp(UBYTE e,FLOAT p1,FLOAT p2)
{ 
  const class ParametricToneMappingBox *ccurve;

  ccurve = m_pNameSpace->FindNonlinearity(ParametricToneMappingBox::Linear,e,p1,p2);

  if (ccurve == NULL) {
    class ParametricToneMappingBox *curve;
    UBYTE nextid         = m_pNameSpace->AllocateNonlinearityID();
    
    curve = (class ParametricToneMappingBox *)SuperBox::CreateBox(ParametricToneMappingBox::Type);
    assert(curve);
  
    curve->DefineTable(nextid,ParametricToneMappingBox::Linear,e,p1,p2);
    
    ccurve = curve;
  }
  
  return ccurve->TableDestinationOf();
}
///

/// MergingSpecBox::CreateExponentialRamp
// Create an exponental ramp P3*exp((P2-P1)*x+P1)+P4
UBYTE MergingSpecBox::CreateExponentialRamp(UBYTE e,FLOAT p1,FLOAT p2,FLOAT p3,FLOAT p4)
{
  const class ParametricToneMappingBox *ccurve;

  ccurve = m_pNameSpace->FindNonlinearity(ParametricToneMappingBox::Exponential,e,p1,p2,p3,p4);
  
  if (ccurve == NULL) {
    class ParametricToneMappingBox *curve;
    UBYTE nextid         = m_pNameSpace->AllocateNonlinearityID();
    
    curve = (class ParametricToneMappingBox *)SuperBox::CreateBox(ParametricToneMappingBox::Type);
    assert(curve);
    
    curve->DefineTable(nextid,ParametricToneMappingBox::Exponential,e,p1,p2,p3,p4);

    ccurve = curve;
  }
  
  return ccurve->TableDestinationOf();
}
///

/// MergingSpecBox::CreateLogarithmicMap
// Create a logarithmic map sign(P1)*log((|P1|*x)^P2+P3)+P4
UBYTE MergingSpecBox::CreateLogarithmicMap(UBYTE e,FLOAT p1,FLOAT p2,FLOAT p3,FLOAT p4)
{
  const class ParametricToneMappingBox *ccurve;

  ccurve = m_pNameSpace->FindNonlinearity(ParametricToneMappingBox::Logarithmic,e,p1,p2,p3,p4);
  
  if (ccurve == NULL) {
    class ParametricToneMappingBox *curve;
    UBYTE nextid         = m_pNameSpace->AllocateNonlinearityID();
    
    curve = (class ParametricToneMappingBox *)SuperBox::CreateBox(ParametricToneMappingBox::Type);
    assert(curve);
    
    curve->DefineTable(nextid,ParametricToneMappingBox::Logarithmic,e,p1,p2,p3,p4);
    
    ccurve = curve;
  }
  
  return ccurve->TableDestinationOf();
}
///

/// MergingSpecBox::CreatePowerMap
// Create a power map (p2-p1)*x^p3+p1
UBYTE MergingSpecBox::CreatePowerMap(UBYTE e,FLOAT p1,FLOAT p2,FLOAT p3)
{
  const class ParametricToneMappingBox *ccurve;

  ccurve = m_pNameSpace->FindNonlinearity(ParametricToneMappingBox::GammaOffset,e,p1,p2,p3);
  
  if (ccurve == NULL) {
    class ParametricToneMappingBox *curve;
    UBYTE nextid         = m_pNameSpace->AllocateNonlinearityID();
    
    curve = (class ParametricToneMappingBox *)SuperBox::CreateBox(ParametricToneMappingBox::Type);
    assert(curve);
    
    curve->DefineTable(nextid,ParametricToneMappingBox::GammaOffset,e,p1,p2,p3);
    
    ccurve = curve;
  }
  
  return ccurve->TableDestinationOf();
}
///

/// MergingSpecBox::CreateIdentity
// Create an identity mapping with the given rounding mode. Note that this is actually scaling.
UBYTE MergingSpecBox::CreateIdentity(UBYTE rounding_mode)
{
  const class ParametricToneMappingBox *ccurve;

  ccurve = m_pNameSpace->FindNonlinearity(ParametricToneMappingBox::Identity,rounding_mode);

  if (ccurve == NULL) {
    class ParametricToneMappingBox *curve;
    UBYTE nextid = m_pNameSpace->AllocateNonlinearityID();

    curve = (class ParametricToneMappingBox *)SuperBox::CreateBox(ParametricToneMappingBox::Type);
    assert(curve);
    
    curve->DefineTable(nextid,ParametricToneMappingBox::Identity,rounding_mode);
    
    ccurve = curve;
  }
    
  return ccurve->TableDestinationOf();
}
///

/// MergingSpecBox::ParseFreeFormTransformation
// Build a free-form linear transformation that converts between color spaces. 
// Matrix coefficients start at the given tag-base and will be parsed off.
// This call will create an appropriate LinearTransformationBox and will return its ID, to be
// put into the MergingSpecBox.
MergingSpecBox::DecorrelationType MergingSpecBox::ParseFreeFormTransformation(const struct JPG_TagItem *tags,
                                                                              ULONG tagbase)
{ 
  class LinearTransformationBox *lbox;
  UBYTE nextid         = m_pNameSpace->AllocateMatrixID();
  LONG entries[9]; // the matrix entries.
  int i;

  for(i = 0;i < 9;i++) {
    const struct JPG_TagItem *entry = tags->FindTagItem(tagbase + i);
    if (entry == NULL) {
      JPG_THROW(OBJECT_DOESNT_EXIST,"MergingSpecBox::ParseFreeFormTransformation",
                "not all entries of a free-form linear transformation are given, cannot create the matrix");
    } else {
      entries[i] = entry->ti_Data.ti_lData;
      if (entries[i] < MIN_WORD || entries[i] > MAX_WORD)
        JPG_THROW(OVERFLOW_PARAMETER,"MergingSpecBox::ParseFreeFormTransformation",
                  "matrix entries of the linear transformation are out of range, "
                  "absolute value must be smaller than four");
    }
  }

  // Now define the box
  // The box inserts itself into the boxlist.
  lbox = (class LinearTransformationBox *)SuperBox::CreateBox(LinearTransformationBox::Type);
  assert(lbox);

  lbox->DefineMatrix(nextid,entries);

  return MergingSpecBox::DecorrelationType(lbox->IdOf());
}
///

/// MergingSpecBox::ParseFreeFormFloatTransformation
// Build a floating point version of a matrix and place it here. This is similar to the above
// except that the coefficients are not fixpoint numbers.
MergingSpecBox::DecorrelationType MergingSpecBox::ParseFreeFormFloatTransformation(const struct JPG_TagItem *tags,
                                                                                   ULONG tagbase)
{ 
  class FloatTransformationBox *lbox;
  UBYTE nextid         = m_pNameSpace->AllocateMatrixID();
  FLOAT entries[9]; // the matrix entries.
  int i;

  for(i = 0;i < 9;i++) {
    const struct JPG_TagItem *entry = tags->FindTagItem(tagbase + i);
    if (entry == NULL) {
      JPG_THROW(OBJECT_DOESNT_EXIST,"MergingSpecBox::ParseFreeFormTransformation",
                "not all entries of a free-form linear transformation are given, cannot create the matrix");
    } else {
      entries[i] = entry->ti_Data.ti_lData;
      if (entries[i] < MIN_WORD || entries[i] > MAX_WORD)
        JPG_THROW(OVERFLOW_PARAMETER,"MergingSpecBox::ParseFreeFormTransformation",
                  "matrix entries of the linear transformation are out of range, "
                  "absolute value must be smaller than four");
    }
  }

  // Now define the box
  // The box inserts itself into the boxlist.
  lbox = (class FloatTransformationBox *)SuperBox::CreateBox(FloatTransformationBox::Type);
  assert(lbox);

  lbox->DefineMatrix(nextid,entries);

  return MergingSpecBox::DecorrelationType(lbox->IdOf());
}
///

/// MergingSpecBox::AlphaModeOf
// Return the current setting of the alpha box if we have one. Return a negative value in case 
// the box is not defined. Returns the matte color as side information.
BYTE MergingSpecBox::AlphaModeOf(ULONG &matte_r,ULONG &matte_g,ULONG &matte_b)
{
  assert(BoxTypeOf() == AlphaType);

  if (m_pAlphaMode) {
    matte_r = m_pAlphaMode->MatteColorOf(0);
    matte_g = m_pAlphaMode->MatteColorOf(1);
    matte_b = m_pAlphaMode->MatteColorOf(2);
    
    return m_pAlphaMode->CompositingMethodOf();
  } else {
    return -1;
  }
}
///

/// MergingSpecBox::SetAlphaMode
// Define the alpha compositing box box with the given settings and the given matte colors.
void MergingSpecBox::SetAlphaMode(AlphaBox::Method mode,ULONG r,ULONG g,ULONG b)
{
  assert(BoxTypeOf() == AlphaType); 

  if (m_pAlphaMode == NULL)
    SuperBox::CreateBox(AlphaBox::Type);

  m_pAlphaMode->SetCompositingMethod(mode);
  m_pAlphaMode->SetMatteColor(0,r);
  m_pAlphaMode->SetMatteColor(1,g);
  m_pAlphaMode->SetMatteColor(2,b);
}
///
