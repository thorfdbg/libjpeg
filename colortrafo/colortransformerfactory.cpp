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
** $Id: colortransformerfactory.cpp,v 1.80 2023/02/21 10:17:46 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "tools/numerics.hpp"
#include "codestream/tables.hpp"
#include "boxes/matrixbox.hpp"
#include "boxes/lineartransformationbox.hpp"
#include "boxes/floattransformationbox.hpp"
#include "boxes/mergingspecbox.hpp"
#include "boxes/tonemapperbox.hpp"
#include "boxes/floattonemappingbox.hpp"
#include "boxes/parametrictonemappingbox.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/integertrafo.hpp"
#include "colortrafo/floattrafo.hpp"
#include "colortrafo/ycbcrtrafo.hpp"
#include "colortrafo/lslosslesstrafo.hpp"
#include "colortrafo/multiplicationtrafo.hpp"
#include "colortrafo/colortransformerfactory.hpp"
#include "marker/lscolortrafo.hpp"
#include "marker/frame.hpp"
#define FIX_BITS ColorTrafo::FIX_BITS
///

/// ColorTransformerFactory::ColorTransformerFactory
// Build a color transformation factory - requires the tables
// that contain most of the data.
ColorTransformerFactory::ColorTransformerFactory(class Tables *tables)
  : JKeeper(tables->EnvironOf()), m_pTrafo(NULL), m_pTables(tables), 
    m_pIdentity0(NULL), m_pIdentity1(NULL), m_pZero(NULL)
{
}
///

/// ColorTransformerFactory::~ColorTransformerFactory
ColorTransformerFactory::~ColorTransformerFactory(void)
{
  delete m_pTrafo;
  delete m_pIdentity0;
  delete m_pIdentity1;
  delete m_pZero;
}
///

/// ColorTransformerFactory::FindToneMapping
// Given a LUT index, construct the tone mapping represenging it.
class ToneMapperBox *ColorTransformerFactory::FindToneMapping(UBYTE idx,UBYTE e)
{
  if (idx == MAX_UBYTE) { // undefined, thus the identity or zero.
    class Box *&id = (e == 0)?(m_pIdentity0):(m_pIdentity1);
    if (id == NULL) {
      class ParametricToneMappingBox *nid = new(m_pEnviron) class ParametricToneMappingBox(m_pEnviron,id);
      assert(id == nid);
      nid->DefineTable(0,ParametricToneMappingBox::Identity,e);
    }
    return (class ToneMapperBox *)id;
  } else {
    return m_pTables->FindToneMapping(idx);
  }
}
///

/// ColorTransformerFactory::GetStandardMatrix
// Fill in a default matrix from its decorrelation type. This is the fixpoint version.
void ColorTransformerFactory::GetStandardMatrix(MergingSpecBox::DecorrelationType dt,LONG matrix[9])
{
  const LONG *df = NULL;
  
  switch(dt) {
  case MergingSpecBox::Zero:
    {
      static const LONG ZeroMatrix[9] = {0,0,0,
                                         0,0,0,
                                         0,0,0};
      df = ZeroMatrix;
    }
    break;
  case MergingSpecBox::Identity:
    {
      static const LONG IdentityMatrix[9]  = {TO_FIX(1.0),TO_FIX(0.0),TO_FIX(0.0),
                                              TO_FIX(0.0),TO_FIX(1.0),TO_FIX(0.0),
                                              TO_FIX(0.0),TO_FIX(0.0),TO_FIX(1.0)};
      df = IdentityMatrix;
    }
    break;
  case MergingSpecBox::YCbCr:
    {
      static const LONG YCbCrToRGB[9] = {TO_FIX(1.0), TO_FIX(0.0)         , TO_FIX(1.40200),
                                         TO_FIX(1.0),-TO_FIX(0.3441362861),-TO_FIX(0.7141362859),
                                         TO_FIX(1.0), TO_FIX(1.772)       , TO_FIX(0.0)};
      df = YCbCrToRGB;
    }
    break;
  default:
    df = NULL;
  }
  assert(df != NULL);

  memcpy(matrix,df,sizeof(LONG) * 9);
}
///

/// ColorTransformerFactory::GetStandardMatrix
// Fill in a default matrix from its decorrelation type. This is the floating point version.
///

/// ColorTransformerFactory::GetInverseStandardMatrix
// Return the inverse of a standard matrix in fixpoint.
void ColorTransformerFactory::GetInverseStandardMatrix(MergingSpecBox::DecorrelationType dt,LONG matrix[9])
{ 
  const LONG *df = NULL;
  
  switch(dt) {
  case MergingSpecBox::Zero:
    {
      static const LONG ZeroMatrix[9] = {0,0,0,
                                         0,0,0,
                                         0,0,0};
      df = ZeroMatrix;
    }
    break;
  case MergingSpecBox::Identity:
    {
      static const LONG IdentityMatrix[9]  = {TO_FIX(1.0),TO_FIX(0.0),TO_FIX(0.0),
                                              TO_FIX(0.0),TO_FIX(1.0),TO_FIX(0.0),
                                              TO_FIX(0.0),TO_FIX(0.0),TO_FIX(1.0)};
      df = IdentityMatrix;
    }
    break;
  case MergingSpecBox::YCbCr:
    {
      static const LONG RGBToYCbCr[9] = { TO_FIX(0.29900)      ,TO_FIX(0.58700)      ,TO_FIX(0.11400),
                                          -TO_FIX(0.1687358916),-TO_FIX(0.3312641084), TO_FIX(0.5),
                                          TO_FIX(0.5)         ,-TO_FIX(0.4186875892),-TO_FIX(0.08131241085)};
      df = RGBToYCbCr;
    }
    break;
  default:
    df = NULL;
  }
  assert(df != NULL);

  memcpy(matrix,df,sizeof(LONG) * 9);
}
///

/// ColorTransformerFactory::GetInverseStandardMatrix
// Return the inverse of a standard matrix in floating point.
///

/// ColorTransformerFactory::BuildColorTransformer
// Build a color transformer from the merging specifications
// passed in. These might be NULL in case there is none and
// the JPEG stream is non-extended. 
// Returns the color transformation class.
// Note that there is at most one color transformer in the system
// (there are no tiles here as in JPEG 2000).
class ColorTrafo *ColorTransformerFactory::BuildColorTransformer(class Frame *frame,class Frame *residual,
                                                                 class MergingSpecBox *specs,
                                                                 UBYTE inbpp,UBYTE outbpp,
                                                                 UBYTE etype,bool encoding,
                                                                 bool disabletorgb)
{
  MergingSpecBox::DecorrelationType rtrafo = MergingSpecBox::Zero;
  MergingSpecBox::DecorrelationType ltrafo = MergingSpecBox::YCbCr;
  MergingSpecBox::DecorrelationType ctrafo = MergingSpecBox::Identity;
  UBYTE rbits    = 0;     // fractional bits required for the r-transformation.
  UBYTE count    = frame->DepthOf();
  UBYTE resbpp   = inbpp; // bits per pixel in the residual image.
  UBYTE ocflags  = 0;
  
  if (m_pTrafo)
    return m_pTrafo;

  if (etype == 0)
    return NULL;
  
  ltrafo = m_pTables->LTrafoTypeOf(count);
  rtrafo = m_pTables->RTrafoTypeOf(count);
  ctrafo = m_pTables->CTrafoTypeOf(count);
  rbits  = m_pTables->FractionalRBitsOf(count,frame->isDCTBased());

  if (ltrafo == MergingSpecBox::YCbCr && disabletorgb)
    ltrafo = MergingSpecBox::Identity;

  if (specs) {
    ocflags |= ColorTrafo::Extended;
  } else if (ltrafo != MergingSpecBox::JPEG_LS) {
    // Standard JPEG has clamping semantics, not wrap-around.
    ocflags |= ColorTrafo::ClampFlag;
  }
  
  if (residual) {
    resbpp   = residual->HiddenPrecisionOf();
    ocflags |= ColorTrafo::Residual | ColorTrafo::Extended;
  }

  //
  // If the merging spec box defines output clipping, then the external type should be an integer
  // type. Otherwise, it must be a float type.
  if (specs && specs->usesClipping()) {
    ocflags |= ColorTrafo::ClampFlag;
  }
  //
  // Float may require an additional clamping
  // This clamping step avoids the generation of INFs or NANs on
  // lossy decoding.
  if (specs && specs->usesOutputConversion())
    ocflags |= ColorTrafo::Float;
  //
  if (ltrafo == MergingSpecBox::JPEG_LS && ocflags == 0) {
    BuildLSTransformation(etype,frame,residual,specs,ocflags,ltrafo,rtrafo);
  } else {
    class IntegerTrafo *itrafo = NULL; // for profile C
    class FloatTrafo   *atrafo = NULL; // for profile A
    class FloatTrafo   *btrafo = NULL; // for profile B
    //
    // Further refinement depends on the type of transformation. Integer or floating point profile
    // types.
    // If there is a pre or postscaling transformation, this is probably profile A.
    if (specs && specs->isProfileA()) {
      NOREF(atrafo);
      JPG_THROW(NOT_IMPLEMENTED,"ColorTransformerFactory::BuildColorTransformer",
                "Profile A support not available due to patented IPRs");
    } else if (specs && specs->isProfileB()) {
      NOREF(btrafo); 
      JPG_THROW(NOT_IMPLEMENTED,"ColorTransformerFactory::BuildColorTransformer",
                "Profile B support not available due to patented IPRs");
    } else if (residual) {
      itrafo = BuildIntegerTransformation(etype,frame,residual,specs,ocflags,ltrafo,rtrafo);
    } else {
      itrafo = BuildIntegerTransformation(etype,frame,residual,specs,ocflags,ltrafo,MergingSpecBox::Zero);
    }
    if (itrafo) {
      InstallIntegerParameters(itrafo,specs,count,encoding,(residual != NULL)?true:false,inbpp,outbpp,resbpp,rbits,
                               ltrafo,rtrafo,ctrafo);
    } else { 
    }
  }
  
  if (m_pTrafo == NULL)
    JPG_THROW(INVALID_PARAMETER,"ColorTransformationFactory::BuildRTransformation",
              "The combination of L and R transformation is non-standard and not supported");


  return m_pTrafo;
}
///

/// ColorTransformerFactory::InstallIntegerParameters
// Install the parameters to fully define a profile C encoder/decoder
void ColorTransformerFactory::InstallIntegerParameters(class IntegerTrafo *trafo,
                                                       class MergingSpecBox *specs,
                                                       int count,bool encoding,bool residual,
                                                       UBYTE inbpp,UBYTE outbpp,UBYTE resbpp,UBYTE rbits,
                                                       MergingSpecBox::DecorrelationType ltrafo,
                                                       MergingSpecBox::DecorrelationType rtrafo,
                                                       MergingSpecBox::DecorrelationType ctrafo)
{
  int i;
  const LONG *tonemapping[4],*inverse[4];
  LONG tableshift = 0;
  //
  // Install the L-tables.
  for(i = 0;i < 4;i++) {
    UBYTE idx;
    const LONG *table = NULL;
    const LONG *inv   = NULL;
    class ToneMapperBox *box = NULL;
    
    if (i < count) {
      if (specs) { 
        idx = specs->LTableIndexOf(i);
      } else {
        // L-tables default to identities if no specs marker is there.
        idx = MAX_UBYTE;
      }
      box = FindToneMapping(idx,1);
      if (box == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the L lookup table specified in the codestream does not exist");
      //
      // L-tables are int-to-int
      table = box->ScaledTableOf(inbpp,outbpp,0,0);
      if (table == NULL)
        JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::InstallIntegerParameters",
                  "found a floating point table in the integer coding profile, this is not allowed");
      if (encoding) {
        inv = box->InverseScaledTableOf(inbpp,outbpp,0,0);
        // If we are encoding float, make sure that the upper half of the
        // table is zero to map out of gamut (negative) colors to zero.
        if (specs && specs->usesOutputConversion()) {
          memset(const_cast<LONG *>(inv) + ((1 << outbpp) >> 1),0,
                 sizeof(LONG) * ((1 << outbpp) >> 1));
        }
      }
    }
    tonemapping[i] = table;
    inverse[i]     = inv;
  }
  trafo->DefineDecodingTables(tonemapping);
  if (encoding)
      trafo->DefineEncodingTables(inverse);
  
  if (ltrafo != MergingSpecBox::JPEG_LS) {
    LONG matrix[9];
    LONG inverse[9];
    class MatrixBox *box;
    class LinearTransformationBox *lbox;
    
    switch(ltrafo) {
    case MergingSpecBox::Undefined:
      if (count > 1) {
        GetStandardMatrix(MergingSpecBox::YCbCr   ,matrix);
      } else {
        GetStandardMatrix(MergingSpecBox::Identity,matrix);
      }
      if (encoding) {
        if (count > 1) {
          GetInverseStandardMatrix(MergingSpecBox::YCbCr   ,inverse);
        } else {
          GetInverseStandardMatrix(MergingSpecBox::Identity,inverse);
        }
      }
      break;
    case MergingSpecBox::Identity:
    case MergingSpecBox::YCbCr:
      GetStandardMatrix(ltrafo,matrix);
      if (encoding)
        GetInverseStandardMatrix(ltrafo,inverse);
      break;
    case MergingSpecBox::Zero:
    case MergingSpecBox::JPEG_LS:
    case MergingSpecBox::RCT:
      JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::InstallIntegerParameters",
                "the base transformation specified in the codestream is invalid");
      break;
    default: // Freeform.
      box = m_pTables->FindMatrix(ltrafo);
      if (box == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the base transformation specified in the codestream does not exist");
      lbox = dynamic_cast<LinearTransformationBox *>(box);
      if (lbox == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the base transformation specified in the codestream is not of fix point type");
      memcpy(matrix,lbox->MatrixOf(),9 * sizeof(LONG));
      if (encoding) {
        memcpy(inverse,lbox->InverseMatrixOf(),9 * sizeof(LONG));
      }
      break;
    }
    trafo->DefineLTransformation(matrix);
    if (encoding)
      trafo->DefineFwdLTransformation(inverse);
    
    if (ctrafo == MergingSpecBox::Undefined || ctrafo == MergingSpecBox::Identity) {
      GetStandardMatrix(MergingSpecBox::Identity,matrix);
      if (encoding)
        GetInverseStandardMatrix(MergingSpecBox::Identity,inverse);
    } else if (ctrafo >= MergingSpecBox::FreeForm) {
      box = m_pTables->FindMatrix(ctrafo);
      if (box == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the color transformation specified in the codestream does not exist");
      lbox = dynamic_cast<LinearTransformationBox *>(box);
      if (lbox == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the color transformation specified in the codestream is not of fix point type");
      memcpy(matrix,lbox->MatrixOf(),9 * sizeof(LONG));
      if (encoding) {
        memcpy(inverse,lbox->InverseMatrixOf(),9 * sizeof(LONG));
      }
    } else {
      JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::InstallIntegerParameters",
                "the color transformation specified in the codestream is invalid");
    }
    trafo->DefineCTransformation(matrix);
    if (encoding)
      trafo->DefineFwdCTransformation(inverse);
  }

  //
  // Now go for the Q-tables. If a table is the identity, install a NULL to avoid clipping.
  // Q-tables do not exist if there is no specs marker.
  for(i = 0;i < 4;i++) {
    UBYTE idx;
    const LONG *table = NULL;
    const LONG *inv   = NULL;
    class ToneMapperBox *box = NULL;
    class ParametricToneMappingBox *parm = NULL;
    UBYTE inbits = resbpp; 
    //
    // The RCT is special in the sense that the additional
    // bit it takes is not a fractional bit but a precision
    // bit. We could check the transformation type, but
    // the RCT is the only transformation that has rbits=1.
    if (rbits == 1) {
      inbits = resbpp - rbits;
    }
    // Otherwise, this is just an implementation detail of the DCT that is not relevant
    // that we have fractional bits. The standard has always rbits = 0 here
    // for all transformations. It is ok to use other values for the fractional bits
    // because they are scaled away for all lookup type of transformations.
    // For table lookups, this is non-conforming.
    // (But what to do? We cannot fix the DCT for part-6/7, and thus cannot
    //  define Re and Rf there...)
    if (specs && i < count && residual) {
      idx   = specs->QTableIndexOf(i);
      box   = FindToneMapping(idx,0); // yes, e is really zero here.
      if (box == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the r lookup table specified in the codestream does not exist");
      parm  = dynamic_cast<class ParametricToneMappingBox *>(box); 
      table = box->ScaledTableOf(inbits,outbpp,rbits,rbits);
      // If the table is zero, then this is a floating point table we cannot really use here.
      if (table == NULL)
        JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::InstallIntegerParameters",
                  "found a floating point table an integer coding profile, this is not allowed");
      if (encoding) {
        // If this is the zero-table, do not try to build an inverse. It is not used
        // anyhow (hopefully!).
        if (parm == NULL || parm->CurveTypeOf() != ParametricToneMappingBox::Zero) {
          // Do not build the inverse if S is there.
          inv = box->InverseScaledTableOf(inbits,outbpp,rbits,rbits);
        }
      }
    }
      
    tonemapping[i] = table;
    inverse[i]     = inv;
  }
  trafo->DefineResidualDecodingTables(tonemapping);
  trafo->DefineResidualEncodingTables(inverse);
  
  //
  // Get the R-tables and install them.
  // This is only for lossy. Near-lossless may use Q, though.
  if (specs && specs->usesClipping()) {
    // Now go for the R-tables. 
    for(i = 0;i < 4;i++) {
      UBYTE idx;
      const LONG *table = NULL;
      const LONG *inv   = NULL;
      class ToneMapperBox *box = NULL;
      class ParametricToneMappingBox *parm = NULL;
      
      if (specs && i < count && residual) {
        
        idx   = specs->R2TableIndexOf(i);
        box   = FindToneMapping(idx,0);
        
        if (box == NULL)
          JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                    "the R lookup table specified in the codestream does not exist");
        //
        // Check whether it is the identity. Then do not install a table.
        // R-tables are upfront the color transformation and thus may have
        // fractional bits.
        // Note that R-tables are also int-to-int and do not extend the dynamic
        // range, input and output are the final bpp
        // Added: These are (of course) only used in the lossy
        // case and come with one preshifted bit.
        parm  = dynamic_cast<class ParametricToneMappingBox *>(box);
        table = box->ScaledTableOf(outbpp,outbpp,rbits,0);
        if (table == NULL)
          JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::InstallIntegerParameters",
                    "found a floating point table in an integer coding profile, this is not allowed");
        if (encoding) {
          // If this is the zero-table, do not try to build an inverse. It is not used
          // anyhow (hopefully!).
          if (parm == NULL || parm->CurveTypeOf() != ParametricToneMappingBox::Zero) {
            // Currently, only parametric versions are allowed here. In principle,
            // one could try to use LUTs here as well, though they would not support
            // the extended range output required here.
            if (parm == NULL)
              JPG_THROW(NOT_IN_PROFILE,"ColorTransformerFactory::InstallIntegerParameters",
                        "only parametric curves are supported for the secondary residual NLT transformation");
            tableshift = (1UL << outbpp) >> 1;
            inv        = parm->ExtendedInverseScaledTableOf(outbpp,outbpp,rbits,0,tableshift,outbpp + 1);
          }
        }
      }
      
      tonemapping[i] = table;
      inverse[i]     = inv;
    }
    trafo->DefineTableShift(tableshift);
    trafo->DefineResidual2DecodingTables(tonemapping);
    trafo->DefineResidual2EncodingTables(inverse);
  }

  if (residual) {
    LONG matrix[9];
    LONG inverse[9];
    class MatrixBox *box;
    class LinearTransformationBox *lbox;
    
    switch(rtrafo) {
    case MergingSpecBox::Undefined:
    case MergingSpecBox::RCT: // actually, the RCT does not use matrix passed over. Provide something.
      if (count > 1) {
        GetStandardMatrix(MergingSpecBox::YCbCr   ,matrix);
      } else {
        GetStandardMatrix(MergingSpecBox::Identity,matrix);
      }
      if (encoding) {
        if (count > 1) {
          GetInverseStandardMatrix(MergingSpecBox::YCbCr   ,inverse);
        } else {
          GetInverseStandardMatrix(MergingSpecBox::Identity,inverse);
        }
      }
      break;
    case MergingSpecBox::Identity:
    case MergingSpecBox::YCbCr:
    case MergingSpecBox::Zero:
      GetStandardMatrix(rtrafo,matrix);
      if (encoding)
        GetInverseStandardMatrix(rtrafo,inverse);
      break;
    case MergingSpecBox::JPEG_LS:
      JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::InstallIntegerParameters",
                "the residual transformation specified in the codestream is invalid");
      break;
    default: // Freeform.
      box = m_pTables->FindMatrix(rtrafo);
      if (box == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the residual transformation specified in the codestream does not exist");
      lbox = dynamic_cast<LinearTransformationBox *>(box);
      if (lbox == NULL)
        JPG_THROW(OBJECT_DOESNT_EXIST,"ColorTransformerFactory::InstallIntegerParameters",
                  "the residual transformation specified in the codestream is not of fix point type");
      memcpy(matrix,lbox->MatrixOf(),9 * sizeof(LONG));
      if (encoding) {
        memcpy(inverse,lbox->InverseMatrixOf(),9 * sizeof(LONG));
      }
      break;
    }
    trafo->DefineRTransformation(matrix);
    if (encoding)
      trafo->DefineFwdRTransformation(inverse);
  }
}
///

/// ColorTransformerFactory::BuildLSTransformation
// Build a transformation using the JPEG-LS color transformation back-end.
// This works only without a residual (why a residual anyhow?)
class ColorTrafo *ColorTransformerFactory::BuildLSTransformation(UBYTE type,
                                                                 class Frame *frame,class Frame *residualframe,
                                                                 class MergingSpecBox *,
                                                                 UBYTE ocflags,int ltrafo,int rtrafo)
{
  if (residualframe == NULL && rtrafo == MergingSpecBox::Zero && ocflags == 0 && ltrafo == MergingSpecBox::JPEG_LS) { 
    UBYTE count   = frame->DepthOf();
    ULONG outmax  = (1UL << (frame->PrecisionOf() + frame->PointPreShiftOf())) - 1;
    ULONG maxval  = (1UL << frame->HiddenPrecisionOf()) - 1;
    ULONG rmaxval = (ocflags)?((1UL << residualframe->HiddenPrecisionOf()) - 1):(0);
    //
    switch(count) {
    case 1:
      switch(type) {
      case CTYP_UBYTE:
        if (outmax > MAX_UBYTE) {
          JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildLSTransformation",
                    "invalid data type selected for the image, image precision is deeper than 8 bits");
        } else {
          m_pTrafo = new(m_pEnviron) class TrivialTrafo<LONG,UBYTE,1>(m_pEnviron,(outmax + 1) >> 1,outmax);
          return m_pTrafo;
        }
        break;
      case CTYP_UWORD:
        if (outmax > MAX_UWORD) {
          JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildLSTransformation",
                    "invalid data type selected for the image, image precision is deeper than 16 bits");
        } else {
          m_pTrafo = new(m_pEnviron) class TrivialTrafo<LONG,UWORD,1>(m_pEnviron,(outmax + 1) >> 1,outmax);
          return m_pTrafo;
        }
        break;
      }
      break;
    case 3:
      const class LSColorTrafo *marker = m_pTables->LSColorTrafoOf();
      if (marker) {
        if (marker->DepthOf() != 3) {
          JPG_THROW(MALFORMED_STREAM,"ColorTransformerFactory::BuildLSTransformation",
                    "JPEG LS color transformation component count does not match frame depth");
          break;
        }
      } else {
        assert(!"JPEG LS trafo indicated, but not included in the tables");
        break;
      }
      switch(type) {
      case CTYP_UBYTE:
        if (outmax > MAX_UBYTE) {
          JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildLSTransformation",
                    "invalid data type selected for the image, image precision is deeper than 8 bits");
        } else {
          LSLosslessTrafo<UBYTE,3> *trafo = new(m_pEnviron) LSLosslessTrafo<UBYTE,3>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,(outmax + 1) >> 1,outmax);
          m_pTrafo = trafo;
          trafo->InstallMarker(m_pTables->LSColorTrafoOf(),frame);
          return trafo;
        }
        break;
      case CTYP_UWORD:
        if (outmax > MAX_UWORD) {
          JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildLSTransformation",
                    "invalid data type selected for the image, image precision is deeper than 16 bits");
        } else {
          LSLosslessTrafo<UWORD,3> *trafo = new(m_pEnviron) LSLosslessTrafo<UWORD,3>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,(outmax + 1) >> 1,outmax);
          m_pTrafo = trafo;
          trafo->InstallMarker(m_pTables->LSColorTrafoOf(),frame);
          return trafo;
        }
        break;
      }
    }
  }

  return NULL;
}
///

/// ColorTransformerFactory::BuildRTransformationSimple
// Build the color transformer for the case that the ltrafo is the identity and the rtrafo is the identity or zero.
template<int count,typename type>
IntegerTrafo *ColorTransformerFactory::BuildIntegerTransformationSimple(class Frame *frame,
                                                                        class Frame *residualframe,
                                                                        class MergingSpecBox *,
                                                                        UBYTE oc,int ltrafo,int rtrafo)
{
  ULONG maxval   = (1UL << frame->HiddenPrecisionOf()) - 1;
  ULONG outmax   = (1UL << (frame->PrecisionOf() + frame->PointPreShiftOf())) - 1;
  ULONG outshift = (outmax + 1) >> 1;
  ULONG rmaxval  = (residualframe)?((1UL << residualframe->HiddenPrecisionOf()) - 1):(0);
  class IntegerTrafo *t = NULL;

  switch(ltrafo) {
  case MergingSpecBox::Identity:
    switch(rtrafo) {
    case MergingSpecBox::Zero:
      if (oc == ColorTrafo::ClampFlag) {
        m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::ClampFlag,
                                                  MergingSpecBox::Identity,
                                                  MergingSpecBox::Zero>
          (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      } else if (oc == (ColorTrafo::ClampFlag | ColorTrafo::Extended)) {
        m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Extended | ColorTrafo::ClampFlag,
                                                  MergingSpecBox::Identity,
                                                  MergingSpecBox::Zero>
          (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      } else if (oc == (ColorTrafo::ClampFlag | ColorTrafo::Float)) {
        if (TypeTrait<type>::TypeID == CTYP_UWORD)
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::ClampFlag | 
                                                    ColorTrafo::Float,
                                                    MergingSpecBox::Identity,
                                                    MergingSpecBox::Zero>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      } else if (oc == (ColorTrafo::ClampFlag | ColorTrafo::Extended | ColorTrafo::Float)) {
        if (TypeTrait<type>::TypeID == CTYP_UWORD)
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::Extended | 
                                                    ColorTrafo::ClampFlag | ColorTrafo::Float,
                                                    MergingSpecBox::Identity,
                                                    MergingSpecBox::Zero>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      }
      break;
    case MergingSpecBox::Identity:
      if (oc == (ColorTrafo::Residual | ColorTrafo::Extended)) {
        m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | ColorTrafo::Extended,
                                              MergingSpecBox::Identity,
                                              MergingSpecBox::Identity>
          (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      } else if (oc == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag)) {
        m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | 
                                                  ColorTrafo::Extended | ColorTrafo::ClampFlag,
                                                  MergingSpecBox::Identity,
                                                  MergingSpecBox::Identity>
          (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      } else if (oc == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float)) {
        if (TypeTrait<type>::TypeID == CTYP_UWORD)
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,
                                                    ColorTrafo::Residual | 
                                                    ColorTrafo::Extended | 
                                                    ColorTrafo::ClampFlag | ColorTrafo::Float,
                                                    MergingSpecBox::Identity,
                                                    MergingSpecBox::Identity>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      } else if (oc == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float)) {
        if (TypeTrait<type>::TypeID == CTYP_UWORD)
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,
                                                    ColorTrafo::Residual | 
                                                    ColorTrafo::Extended | 
                                                    ColorTrafo::Float,
                                                    MergingSpecBox::Identity,
                                                    MergingSpecBox::Identity>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
      }
    }
  }

  return t;
}
///

/// ColorTransformerFactory::BuildRTransformationFour
// Build the color transformer for the case that the ltrafo is the identity and the rtrafo is zero.
// This covers the four component cases that are currently not included in JPEG XT.
template<typename type>
IntegerTrafo *ColorTransformerFactory::BuildIntegerTransformationFour(class Frame *frame,
                                                                      class Frame *residualframe,
                                                                      class MergingSpecBox *,
                                                                      UBYTE oc,int ltrafo,int rtrafo)
{
  ULONG maxval   = (1UL << frame->HiddenPrecisionOf()) - 1;
  ULONG outmax   = (1UL << (frame->PrecisionOf() + frame->PointPreShiftOf())) - 1;
  ULONG outshift = (outmax + 1) >> 1;
  ULONG rmaxval  = (residualframe)?((1UL << residualframe->HiddenPrecisionOf()) - 1):(0);
  class IntegerTrafo *t = NULL;

  if (rtrafo != MergingSpecBox::Zero)
    return NULL;
  if (oc != ColorTrafo::ClampFlag)
    return NULL;
  if (ltrafo != MergingSpecBox::Identity)
    return NULL;

  m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,4,ColorTrafo::ClampFlag,
                                            MergingSpecBox::Identity,
                                            MergingSpecBox::Zero>
    (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
  
  return t;
}
///

/// ColorTransformerFactory::BuildRTransformationExtensive
template<int count,typename type>
IntegerTrafo *ColorTransformerFactory::BuildIntegerTransformationExtensive(class Frame *frame,
                                                                                 class Frame *residualframe,
                                                                                 class MergingSpecBox *specs,
                                                                                 UBYTE ocflags,
                                                                                 int ltrafo,int rtrafo)
{
  
  if (ltrafo == MergingSpecBox::Identity && 
      (rtrafo == MergingSpecBox::Zero || rtrafo == MergingSpecBox::Identity)) {
    return BuildIntegerTransformationSimple<count,type>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
  } else {
    ULONG maxval   = (1UL << frame->HiddenPrecisionOf()) - 1;
    ULONG outmax   = (1UL << (frame->PrecisionOf() + frame->PointPreShiftOf())) - 1; 
    ULONG outshift = (outmax + 1) >> 1;
    ULONG rmaxval  = (residualframe)?((1UL << residualframe->HiddenPrecisionOf()) - 1):(0);
    class IntegerTrafo *t = NULL;
    
    // Free-form is here handled as a sub-type of YCbCr because the matrix coefficients
    // can be freely specified.
    if (ltrafo >= MergingSpecBox::FreeForm)
      ltrafo = MergingSpecBox::YCbCr;
    if (rtrafo >= MergingSpecBox::FreeForm)
      rtrafo = MergingSpecBox::YCbCr;

    switch(ltrafo) {
    case MergingSpecBox::Identity:
      if (ocflags & ColorTrafo::Residual) {
        switch(rtrafo) {
        case MergingSpecBox::YCbCr:
          if (ocflags == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag)) {
            m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | ColorTrafo::Extended | 
                                                      ColorTrafo::ClampFlag,
                                                      MergingSpecBox::Identity,
                                                      MergingSpecBox::YCbCr>
              (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
            return t;
          } else if (ocflags==(ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float)) {
            if (TypeTrait<type>::TypeID == CTYP_UWORD)
              m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::Residual | 
                                                        ColorTrafo::Extended | 
                                                        ColorTrafo::ClampFlag | 
                                                        ColorTrafo::Float,
                                                        MergingSpecBox::Identity,
                                                        MergingSpecBox::YCbCr>
                (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
            return t;
          }
          break;
        case MergingSpecBox::RCT:
          if (ocflags == (ColorTrafo::Residual | ColorTrafo::Extended)) {
            m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | ColorTrafo::Extended,
                                                      MergingSpecBox::Identity,
                                                      MergingSpecBox::RCT>
              (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
            return t;
          } else if (ocflags == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float)) {
            if (TypeTrait<type>::TypeID == CTYP_UWORD)
              m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,
                                                        ColorTrafo::Residual | 
                                                        ColorTrafo::Float |
                                                        ColorTrafo::Extended,
                                                        MergingSpecBox::Identity,
                                                        MergingSpecBox::RCT>
                (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
            return t;
          }
          break;
        }
      }
      break;
    case MergingSpecBox::YCbCr: // Switch by LTrafo
      switch(rtrafo) { // By rtrafo
      case MergingSpecBox::Zero:
        if (ocflags == ColorTrafo::ClampFlag) {
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::ClampFlag,
                                                    MergingSpecBox::YCbCr,
                                                    MergingSpecBox::Zero>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
          return t;
        } else if (ocflags == (ColorTrafo::ClampFlag | ColorTrafo::Extended)) {
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::ClampFlag | 
                                                    ColorTrafo::Extended,
                                                    MergingSpecBox::YCbCr,
                                                    MergingSpecBox::Zero>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
          return t;
        } else if (ocflags == (ColorTrafo::ClampFlag | ColorTrafo::Float)) {
          if (TypeTrait<type>::TypeID == CTYP_UWORD)
            m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::ClampFlag | 
                                                      ColorTrafo::Float,
                                                      MergingSpecBox::YCbCr,
                                                      MergingSpecBox::Zero>
              (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
          return t;
        } else if (ocflags == (ColorTrafo::ClampFlag | ColorTrafo::Extended | ColorTrafo::Float )) {
          if (TypeTrait<type>::TypeID == CTYP_UWORD)
            m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,
                                                      ColorTrafo::ClampFlag | 
                                                      ColorTrafo::Extended | ColorTrafo::Float,
                                                      MergingSpecBox::YCbCr,
                                                      MergingSpecBox::Zero>
              (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
          return t;
        }
        break;
      case MergingSpecBox::Identity: // rtrafo switch
        if (ocflags & ColorTrafo::Residual) {
          if (ocflags & ColorTrafo::ClampFlag) {
            if (ocflags & ColorTrafo::Float) {
              if (TypeTrait<type>::TypeID == CTYP_UWORD)
                m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,
                                                          ColorTrafo::Residual | 
                                                          ColorTrafo::Extended | 
                                                          ColorTrafo::ClampFlag | 
                                                          ColorTrafo::Float,
                                                          MergingSpecBox::YCbCr,
                                                          MergingSpecBox::Identity>
                  (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,
                   outshift,outmax);
              return t;
            } else {
              m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,
                                                        ColorTrafo::Residual | ColorTrafo::Extended | 
                                                        ColorTrafo::ClampFlag,
                                                        MergingSpecBox::YCbCr,
                                                        MergingSpecBox::Identity>
                (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
              return t;
            }
          } else {
            if (ocflags & ColorTrafo::Float) {
              if (TypeTrait<type>::TypeID == CTYP_UWORD)
                m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::Residual | 
                                                          ColorTrafo::Float |
                                                          ColorTrafo::Extended,
                                                          MergingSpecBox::YCbCr,
                                                          MergingSpecBox::Identity>
                  (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,
                   outshift,outmax);
              return t;
            } else {
              m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | 
                                                        ColorTrafo::Extended,
                                                        MergingSpecBox::YCbCr,
                                                        MergingSpecBox::Identity>
                (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
              return t;
            }
          }
        }
        break;
      case MergingSpecBox::YCbCr:
        if (ocflags == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag)) {
          m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | 
                                                    ColorTrafo::Extended | 
                                                    ColorTrafo::ClampFlag,
                                                    MergingSpecBox::YCbCr,
                                                    MergingSpecBox::YCbCr>
            (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
          return t;
        } else if (ocflags == (ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float)) {
          if (TypeTrait<type>::TypeID == CTYP_UWORD)
            m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::Residual | 
                                                      ColorTrafo::Extended | 
                                                      ColorTrafo::ClampFlag | ColorTrafo::Float,
                                                      MergingSpecBox::YCbCr,
                                                      MergingSpecBox::YCbCr>
              (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
          return t;
        }
        break;
      case MergingSpecBox::RCT: 
        if (ocflags & ColorTrafo::Residual) {
          if ((ocflags & ColorTrafo::ClampFlag) == 0) {
            if (ocflags & ColorTrafo::Float) {
              if (TypeTrait<type>::TypeID == CTYP_UWORD)
                m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<UWORD,count,ColorTrafo::Residual | 
                                                          ColorTrafo::Float |
                                                          ColorTrafo::Extended,
                                                          MergingSpecBox::YCbCr,
                                                          MergingSpecBox::RCT>
                  (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,
                   outshift,outmax);
              return t;
            } else {
              m_pTrafo = t = new(m_pEnviron) YCbCrTrafo<type,count,ColorTrafo::Residual | 
                                                        ColorTrafo::Extended,
                                                        MergingSpecBox::YCbCr,
                                                        MergingSpecBox::RCT>
                (m_pEnviron,(maxval + 1) >> 1,maxval,(rmaxval + 1) >> 1,rmaxval,outshift,outmax);
              return t;
            }
          }
        }
        break;
      }
      break;
    }
  }
  
  return NULL;
}
///

/// ColorTransformerFactory::BuildIntegerTransformation
// Build transformations that require only L and R.
class IntegerTrafo *ColorTransformerFactory::BuildIntegerTransformation(UBYTE type,
                                                                        class Frame *frame,class Frame *residualframe,
                                                                        class MergingSpecBox *specs,
                                                                        UBYTE ocflags,int ltrafo,int rtrafo)
{
  UBYTE count  = frame->DepthOf();
  ULONG outmax = (1UL << (frame->PrecisionOf() + frame->PointPreShiftOf())) - 1;


  switch(count) {
  case 1:
    switch(type) {
    case CTYP_UBYTE:
      if (outmax > MAX_UBYTE)
        JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                  "invalid data type selected for the image, image precision is deeper than 8 bits");
      return BuildIntegerTransformationSimple<1,UBYTE>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
    case CTYP_UWORD:
      if (outmax > MAX_UWORD)
        JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                  "invalid data type selected for the image, image precision is deeper than 16 bits");
      return BuildIntegerTransformationSimple<1,UWORD>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
    }
    break;
   case 2:
     switch(type) {
     case CTYP_UBYTE:
       if (outmax > MAX_UBYTE)
         JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                   "invalid data type selected for the image, image precision is deeper than 8 bits");
       return BuildIntegerTransformationSimple<2,UBYTE>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
     case CTYP_UWORD:
       if (outmax > MAX_UWORD)
         JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                   "invalid data type selected for the image, image precision is deeper than 16 bits");
       return BuildIntegerTransformationSimple<2,UWORD>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
     }
     break;
  case 3:
    switch(type) {
    case CTYP_UBYTE:
      if (outmax > MAX_UBYTE)
        JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                  "invalid data type selected for the image, image precision is deeper than 8 bits");
      return BuildIntegerTransformationExtensive<3,UBYTE>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
    case CTYP_UWORD:
      if (outmax > MAX_UWORD)
        JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                  "invalid data type selected for the image, image precision is deeper than 16 bits");
      return BuildIntegerTransformationExtensive<3,UWORD>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
    }
    break;
    //
    // For JPEG reference testing, we also need four components. It may also come handy for CMYK images.
  case 4:
    switch(type) {
    case CTYP_UBYTE:
      if (outmax > MAX_UBYTE)
        JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                  "invalid data type selected for the image, image precision is deeper than 8 bits");
      return BuildIntegerTransformationFour<UBYTE>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
    case CTYP_UWORD:
      if (outmax > MAX_UWORD)
        JPG_THROW(OVERFLOW_PARAMETER,"ColorTransformerFactory::BuildRTransformation",
                  "invalid data type selected for the image, image precision is deeper than 16 bits");
      return BuildIntegerTransformationFour<UWORD>(frame,residualframe,specs,ocflags,ltrafo,rtrafo);
    }
    break;
  }

  return NULL;
}
///

/// ColorTransformerFactory::BuildFloatTransformation
// Build a floating-point type of transformation. Leave all the variable coding
// parameters defined by boxes still undefined and just build the core code.
///

/// ColorTransformerFactory::InstallProfileAParameters
// Install all the coding parameters for a profile A encoder or decoder.
// Get all the parameters from the MergingSpecBox.
///

/// ColorTransformerFactory::InstallProfileBParameters
// Install all the coding parameters for a profile A encoder or decoder.
// Get all the parameters from the MergingSpecBox.
///
