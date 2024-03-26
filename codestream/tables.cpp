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
** This class keeps all the coding tables, Huffman, AC table, quantization
** and other side information.
**
** $Id: tables.cpp,v 1.213 2024/03/26 10:04:47 thor Exp $
**
*/

/// Include
#include "codestream/tables.hpp"
#include "marker/quantization.hpp"
#include "marker/quantizationtable.hpp"
#include "marker/huffmantable.hpp"
#include "marker/component.hpp"
#include "marker/actable.hpp"
#include "marker/adobemarker.hpp"
#include "marker/restartintervalmarker.hpp"
#include "marker/jfifmarker.hpp"
#include "marker/exifmarker.hpp"
#include "marker/thresholds.hpp"
#include "marker/lscolortrafo.hpp"
#include "marker/frame.hpp"
#include "boxes/box.hpp"
#include "boxes/databox.hpp"
#include "boxes/tonemapperbox.hpp"
#include "boxes/inversetonemappingbox.hpp"
#include "boxes/floattonemappingbox.hpp"
#include "boxes/parametrictonemappingbox.hpp"
#include "boxes/lineartransformationbox.hpp"
#include "boxes/alphabox.hpp"
#include "boxes/floattransformationbox.hpp"
#include "boxes/checksumbox.hpp"
#include "boxes/mergingspecbox.hpp"
#include "boxes/filetypebox.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/actemplate.hpp"
#include "io/bytestream.hpp"
#include "io/checksumadapter.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/colortransformerfactory.hpp"
#include "tools/traits.hpp"
#include "tools/numerics.hpp"
#include "tools/checksum.hpp"
#include "dct/dct.hpp"
#include "dct/idct.hpp"
#include "dct/liftingdct.hpp"
#define FIX_BITS ColorTrafo::FIX_BITS
///

/// Defines
#define LOSSYDCT IDCT
#define LOSSLESSDCT LiftingDCT
///

/// Tables::Tables
Tables::Tables(class Environ *env)
  : JKeeper(env), m_pResidualTables(NULL), m_pParent(NULL), m_pAlphaTables(NULL), m_pMaster(NULL),
    m_pQuant(NULL), m_pHuffman(NULL), m_pConditioner(NULL),
    m_pRestart(NULL), m_pColorInfo(NULL), m_pResolutionInfo(NULL), m_pCameraInfo(NULL), 
    m_pBoxList(NULL), m_NameSpace(env), m_AlphaNameSpace(env), m_pColorFactory(NULL),
    m_pAlphaData(NULL), m_pResidualData(NULL), m_pRefinementData(NULL), m_pColorTrafo(NULL), 
    m_pThresholds(NULL), m_pLSColorTrafo(NULL), m_pResidualSpecs(NULL), m_pAlphaSpecs(NULL),
    m_pIdentityMapping(NULL), m_pChecksumBox(NULL),
    m_ucMaxError(0), m_bTruncateColor(false), m_bRefinement(false), 
    m_bOpenLoop(false), m_bDeadZone(false), m_bOptimize(false), m_bDeRing(false),
    m_bFoundExp(false), m_bHorizontalExpansion(false), m_bVerticalExpansion(false)
{
  m_NameSpace.DefineSecondaryLookup(&m_pBoxList);
  m_AlphaNameSpace.DefineSecondaryLookup(&m_pBoxList);
}
///

/// Tables::~Tables
Tables::~Tables(void)
{
  class Box *box;

  while ((box = m_pBoxList)) {
    m_pBoxList = box->NextOf();
    delete box;
  }
  
  delete m_pIdentityMapping; // not part of the box list.
  delete m_pLSColorTrafo;
  delete m_pThresholds;
  delete m_pQuant;
  delete m_pHuffman;
  delete m_pConditioner;
  delete m_pColorInfo;
  delete m_pResolutionInfo;
  delete m_pCameraInfo;
  delete m_pColorFactory; // also deletes the transformation
  delete m_pRestart;
  delete m_pResidualTables;
  delete m_pAlphaTables;
}
///

/// Tables::CreateResidualTables
// Create residual tables for the side channel.
class Tables *Tables::CreateResidualTables(void)
{
  if (m_pResidualTables == NULL) {
    m_pResidualTables = new(m_pEnviron) class Tables(m_pEnviron);
    m_pResidualTables->m_pParent = this;
    m_pResidualTables->m_pMaster = m_pMaster;
  }
  
  return m_pResidualTables;
}
///

/// Tables::CreateAlphaTables
// Create tables/side information for the alpha channel.
class Tables *Tables::CreateAlphaTables(void)
{
  assert(m_pParent == NULL);
  
  if (m_pAlphaTables == NULL) {
    m_pAlphaTables = new (m_pEnviron) class Tables(m_pEnviron);
    m_pAlphaTables->m_pMaster = this;
  }

  return m_pAlphaTables;
}
///

/// Tables::InstallDefaultTables
// For writing, install the standard suggested tables.
// Create tables for the residuals if this flag is set.
void Tables::InstallDefaultTables(UBYTE precision,UBYTE rangebits,const struct JPG_TagItem *tags)
{
  class FileTypeBox *profile = NULL;
  LONG frametype   = tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE);
  ULONG quality    = tags->GetTagData(JPGTAG_IMAGE_QUALITY,80);
  ULONG hdrquality = tags->GetTagData(JPGTAG_RESIDUAL_QUALITY,MAX_ULONG);
  ULONG maxerror   = tags->GetTagData(JPGTAG_IMAGE_ERRORBOUND); // also the near value for LS
  ULONG depth      = tags->GetTagData(JPGTAG_IMAGE_DEPTH,(m_pMaster)?1:3); // Default 1 for alpha  
  UBYTE hiddenbits = tags->GetTagData(JPGTAG_IMAGE_HIDDEN_DCTBITS,0);
  UBYTE hiddenresidualbits= tags->GetTagData(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,0);
  ULONG colortrafo = tags->GetTagData(JPGTAG_MATRIX_LTRAFO,
                                     (depth > 1)?JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR:
                                     JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE);
  ULONG rtrafo     = tags->GetTagData(JPGTAG_MATRIX_RTRAFO,colortrafo);
  bool residual    = (frametype & JPGFLAG_RESIDUAL_CODING)?true:false;
  LONG resflags    = tags->GetTagData(JPGTAG_RESIDUAL_FRAMETYPE,JPGFLAG_RESIDUAL);
  bool losslessdct = tags->GetTagData(JPGTAG_IMAGE_LOSSLESSDCT,false)?true:false;
  ULONG restart    = tags->GetTagData(JPGTAG_IMAGE_RESTART_INTERVAL);
  LONG maxerr      = tags->GetTagData(JPGTAG_IMAGE_ERRORBOUND,0);
  LONG levels      = tags->GetTagData(JPGTAG_IMAGE_RESOLUTIONLEVELS,0);
  // transformation in the legacy domain, possibly overridden by the rgb marker
  MergingSpecBox::DecorrelationType ltrafo    = MergingSpecBox::YCbCr; 
  // transformation in the linear (or log) domain, also possibly overridden
  MergingSpecBox::DecorrelationType ctrafo    = MergingSpecBox::Identity;
  bool profilea    = false; // set if this is likely profile A (diagonal transformations)
  bool profileb    = false; // set if this is likely profile B (2ndary TMOs)
  bool dopart8     = false; // set if we are encoding in part 8 (lossless, near lossless)
  bool dopart9     = false;
  ULONG comp;

  //
  // Alpha channel support?
  if (m_pMaster || tags->GetTagPtr(JPGTAG_ALPHA_TAGLIST))
    dopart9 = true;
    
  //
  // If any of these are set, we arein profile B likely.
  for(comp = 0;comp < depth;comp++) {
    if ((tags->FindTagItem(JPGTAG_TONEMAPPING_R2_TYPE(comp)) && 
         (tags->GetTagData(JPGTAG_TONEMAPPING_R2_TYPE(comp)) != JPGFLAG_TONEMAPPING_LINEAR)) ||
        tags->FindTagItem(JPGTAG_TONEMAPPING_L2_TYPE(comp))) {
      profileb = true;
      break;
    }
  }
  //
  // If there is a diagonal transformation, we are likely in profile A.
  if (tags->FindTagItem(JPGTAG_TONEMAPPING_S_TYPE) || tags->FindTagItem(JPGTAG_TONEMAPPING_S_FLUT) ||
      tags->FindTagItem(JPGTAG_TONEMAPPING_P_TYPE))
    profilea = true;

  if (residual) {
    switch(resflags & 7) {
    case JPGFLAG_RESIDUAL:
    case JPGFLAG_RESIDUALPROGRESSIVE:
    case JPGFLAG_RESIDUALDCT:
      dopart8 = true;
      if (profileb || profilea)
        JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                  "residual coding modes are not available in profiles A and B");
      break;
    }
  }
  
  if (quality > 100)
    JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
              "image quality can be at most 100");

  if (hdrquality != MAX_ULONG && hdrquality > 100)
    JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
              "quality of the extensions layer can be at most 100");

  if (maxerr < 0 || maxerr > MAX_UBYTE)
    JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
              "The maximum error must be non-negative and can be at most 255");

  if (m_pQuant != NULL || m_pHuffman != NULL || m_pColorInfo != NULL || m_pResolutionInfo != NULL ||
      m_pRestart != NULL)
    JPG_THROW(OBJECT_EXISTS,"Tables::InstallDefaultTables","Huffman and quantization tables are already defined");

  //
  if (m_pParent) {
    if (hiddenresidualbits)
      m_bRefinement = true;
  } else {
    if (hiddenbits)
      m_bRefinement  = true;
  }

  if (losslessdct)
    dopart8 = true;
  
  //
  // Get the color truncation flag. No longer supported.
  //m_bTruncateColor = tags->GetTagData(JPGTAG_TRUxNCATE_RESIDUALS)?true:false;
  // Set if the encoder uses the original and not the reconstructed samples
  // for computing the residuals. No quantization done then...
  if (m_pParent) {
    m_bOpenLoop = m_pParent->m_bOpenLoop;
    m_bDeadZone = m_pParent->m_bDeadZone;
    m_bOptimize = m_pParent->m_bOptimize;
    m_bDeRing   = false; // never on the residual channel.
  } else {
    m_bOpenLoop = tags->GetTagData(JPGTAG_OPENLOOP_ENCODER)?true:false;
    m_bDeadZone = tags->GetTagData(JPGTAG_DEADZONE_QUANTIZER)?true:false;
    m_bOptimize = tags->GetTagData(JPGTAG_OPTIMIZE_QUANTIZER)?true:false;
    m_bDeRing   = tags->GetTagData(JPGTAG_IMAGE_DERINGING)?true:false;
  }
  //
  // Install the maximum error.
  m_ucMaxError     = UBYTE(maxerr);
  //
  // Lossy modes require a DQT table.
  switch(frametype & 0x07) {
  case JPGFLAG_BASELINE:
  case JPGFLAG_SEQUENTIAL:
  case JPGFLAG_PROGRESSIVE:
    bool rct   = false; // use the range-expanding RCT?  
    m_pQuant = new(m_pEnviron) Quantization(m_pEnviron); 
    if (rtrafo != JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE) {
      // No longer make this decision depending on the DCT.
      // DCT works now also in the lossless mode.
      // DCT is off in the residual domain.
      if (dopart8) {
        if (depth == 3 && tags->GetTagData(JPGTAG_RESIDUAL_DCT,false) == false) {
          // Note that we cannot use the range extension if the DCT is on because
          // it is not exactly linear, due to approximations, which would
          // create loss.
          rct     = true;
        }
      }
    }
    {
      LONG matrix = tags->GetTagData((m_pParent)?
                                     JPGTAG_RESIDUALQUANT_MATRIX:
                                     JPGTAG_QUANTIZATION_MATRIX ,JPGFLAG_QUANTIZATION_ANNEX_K);
      const LONG *lumatable = (const LONG *)tags->GetTagPtr((m_pParent)?
                                                            JPGTAG_RESIDUALQUANT_LUMATABLE:
                                                            JPGTAG_QUANTIZATION_LUMATABLE, NULL);
      const LONG *chromatable = (const LONG *)tags->GetTagPtr((m_pParent)?
                                                              JPGTAG_RESIDUALQUANT_CHROMATABLE:
                                                              JPGTAG_QUANTIZATION_CHROMATABLE,NULL);
      if (m_pParent) {
        m_pQuant->InitDefaultTables(quality,hdrquality,
                                    rtrafo != JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE,
                                    false,true,rct,matrix,precision,lumatable,chromatable);
      } else {
        m_pQuant->InitDefaultTables(quality,hdrquality,
                                    colortrafo != JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE,
                                    false,false,rct,matrix,precision,lumatable,chromatable);
      }
    }
    break;
  }

  //
  // The color information marker is only created in the legacy image.
  if (m_pParent == NULL) {
    // The file format box comes first.
    // AC coding, hierarchical is not part of the XT spec.
    if (m_pMaster == NULL) {
      switch(frametype & 0x3f) {
      case JPGFLAG_BASELINE:
      case JPGFLAG_SEQUENTIAL:
      case JPGFLAG_PROGRESSIVE:
        profile = new(m_pEnviron) class FileTypeBox(m_pEnviron,m_pBoxList);
        break;
      }
    }
    //
    switch(colortrafo) {
    case JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE:
      if (m_pMaster == NULL) {
        m_pColorInfo = new(m_pEnviron) AdobeMarker(m_pEnviron);
        m_pColorInfo->SetColorSpace(AdobeMarker::None);
      }
      ltrafo = MergingSpecBox::Identity; // THOR: Fix. Must remain at none, the marker is only here for legacy decoders.
      break;
    case JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR:
      // Also build the JFIF marker here if it is YCbCr.
      // Later on, we should be able to define the image resolution.
      //
      // If the colorspace is not RGB and we are not the alpha
      // channel and we are in a mode that is acceptable for JFIF, 
      // also generate a JFIF marker.
      if (m_pMaster == NULL) {
        switch(frametype & 0x3f) {
        case JPGFLAG_BASELINE:
        case JPGFLAG_SEQUENTIAL:
        case JPGFLAG_PROGRESSIVE:
          m_pResolutionInfo = new(m_pEnviron) JFIFMarker(m_pEnviron);
          m_pResolutionInfo->SetImageResolution(96,96);
          break;
        }
      }
      ltrafo = MergingSpecBox::YCbCr;
      break;
    case JPGFLAG_MATRIX_COLORTRANSFORMATION_FREEFORM:
      ltrafo = MergingSpecBox::FreeForm;
      break;
    case JPGFLAG_MATRIX_COLORTRANSFORMATION_LSRCT:
      ltrafo = MergingSpecBox::JPEG_LS;
      m_pLSColorTrafo = new(m_pEnviron) LSColorTrafo(m_pEnviron);
      m_pLSColorTrafo->InstallDefaults(precision,maxerror);
      break;
    default:
      JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                "the selected color transformation in the legacy decoding path is not valid");
      break;
    }
    //
    // Also check whether there is another linear color space transformation.
    if (tags->FindTagItem(JPGTAG_MATRIX_CMATRIX(0,0)) || tags->FindTagItem(JPGTAG_MATRIX_CFMATRIX(0,0)))
      ctrafo = MergingSpecBox::FreeForm; // Yup!
  }

  if (restart) {
    bool isls       = ((frametype & 0x07) == JPGFLAG_JPEG_LS)?true:false;
    m_pRestart      = new(m_pEnviron) class RestartIntervalMarker(m_pEnviron,isls);
    m_pRestart->InstallDefaults(restart);
  }

  if ((frametype & 0x07) == JPGFLAG_JPEG_LS) {
    if (maxerror > 0) {
      m_pThresholds = new(m_pEnviron) class Thresholds(m_pEnviron);
      m_pThresholds->InstallDefaults(precision,maxerror);
    }
  }
  //
  // Create tone mapping curves. Even if all components use identical curves, I'm
  // here writing the same table multiple times. Could be done simpler, indeed.
  // All the box magic is not repeated in the residual image, though.
  // All the testing here just checks whether this is some part of JPEG XT and
  // requires further support to setup all the boxes.
  if (m_pParent == NULL && (residual || hiddenbits || hiddenresidualbits || losslessdct || dopart8 || dopart9 ||
                            profilea || profileb || (((frametype & 0x07) != JPGFLAG_JPEG_LS  &&
                                                      (frametype & 0x07) != JPGFLAG_LOSSLESS &&
                                                      levels == 0 &&
                                                      (frametype & JPGFLAG_PYRAMIDAL) ==0) && 
                                                     (ltrafo != MergingSpecBox::YCbCr ||
                                                      ctrafo != MergingSpecBox::Identity)))) {
    //
    // Here we are a bit more constrained.
    if (depth != 1 && depth != 3)
      JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                "JPEG XT only supports one or three component images");
    //
    //
    if (m_pMaster == NULL) {
      // Create the MergingSpecBox.
      assert(m_pResidualSpecs == NULL);
      m_pResidualSpecs = new(m_pEnviron) class MergingSpecBox(this,m_pBoxList,MergingSpecBox::SpecType);
      //
      // Create alpha
      if (dopart9) {
        assert(m_pAlphaSpecs == NULL);
        m_pAlphaSpecs = new(m_pEnviron) class MergingSpecBox(this,m_pBoxList,MergingSpecBox::AlphaType);
      }
    }
    //
    // Filter a couple of impossible settings for profile A.
    if (profilea || profileb) {
      if (!residual)
        JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                  "Profiles A and B require a residual codestream");
      if (dopart8)
        JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                  "Profiles A and B do not allow lossless coding");
      //
      // This type of profile cannot be put into an alpha image. Thus, test whether we are the
      // alpha channel by testing whether we have a master image and warn.
      if (m_pMaster)
        JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                  "JPEG XT part 9 does not support JPEG XT part 7 profile A and B as alpha channel.");
      //
      //
      // Further refine the settings.
      if (profilea) {
        if ((depth == 1 && ltrafo != MergingSpecBox::Identity) || 
            (depth == 3 && (ltrafo != MergingSpecBox::YCbCr && ltrafo != MergingSpecBox::Identity))) 
          JPG_THROW(INVALID_PARAMETER,"Tables::InstallDefaultTables",
                    "Profile A requires the identity or the YCbCr transformation in the legacy codestream");
        JPG_THROW(NOT_IMPLEMENTED,"Tables::InstallDefaultTables",
                  "Profile A support not available due to patented IPRs");
      } else {
        JPG_THROW(NOT_IMPLEMENTED,"Tables::InstallDefaultTables",
                  "Profile B support not available due to patented IPRs");
      }
    } else {
      // Ok, then we're in profile C.
      CreateProfileCSettings(tags,profile,precision,rangebits,ltrafo,dopart8,dopart9);
    }
    //
    // Also build an EXIF marker if residual markers are included.
    // This is a bug work-around for eog. Yuck!
    if (m_pMaster == NULL && m_pCameraInfo == NULL)
      m_pCameraInfo = new(m_pEnviron) class EXIFMarker(m_pEnviron);
  }
}
///

/// Tables::CreateProfileASettings
// Parse off the tags for a profile A encoder.
///

/// Tables::CreateProfileBSettings
// Parse off the tags for a profile B encoder.
///

/// Tables::CreateProfileCSettings
// Parse off the tags for a profile C encoder
void Tables::CreateProfileCSettings(const struct JPG_TagItem *tags,class FileTypeBox *profile,
                                    UBYTE precision,UBYTE rangebits,MergingSpecBox::DecorrelationType ltrafo,
                                    bool dopart8,bool dopart9)
{    
  ULONG component;
  LONG frametype    = tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE);
  LONG resflags     = tags->GetTagData(JPGTAG_RESIDUAL_FRAMETYPE,JPGFLAG_RESIDUAL);
  UBYTE hiddenbits  = tags->GetTagData(JPGTAG_IMAGE_HIDDEN_DCTBITS,0);
  UBYTE hiddenresidualbits= tags->GetTagData(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,0);
  ULONG depth       = tags->GetTagData(JPGTAG_IMAGE_DEPTH,(m_pMaster)?1:3);  // Default depth is 1 for alpha
  ULONG hdrquality  = tags->GetTagData(JPGTAG_RESIDUAL_QUALITY,MAX_ULONG);
  ULONG colortrafo  = tags->GetTagData(JPGTAG_MATRIX_LTRAFO,
                                       (depth > 1)?JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR:
                                       JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE);
  bool noiseshaping = tags->GetTagData(JPGTAG_IMAGE_ENABLE_NOISESHAPING,false)?true:false;
  bool losslessdct  = tags->GetTagData(JPGTAG_IMAGE_LOSSLESSDCT,false)?true:false;
  bool residual     = (frametype & JPGFLAG_RESIDUAL_CODING)?true:false;
  bool isfloat      = tags->GetTagData(JPGTAG_IMAGE_IS_FLOAT,false)?true:false;
  bool isoc         = tags->GetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION,isfloat)?true:false;
  bool dodct        = true;  
  bool clipping     = true;
  bool profiled     = true; // set if it may be even profile D (more restricted)
  //
  // We do have either residual bits or hidden bits and thus need a specs box.
  // For the alpha codestream, this is a separate superbox.
  class MergingSpecBox *merger  = ResidualSpecsOf();
  class ToneMapperBox  *lut     = NULL;
  //
  // Check whether all the specs are correct. The merger must be present, and for alpha,
  // the depth must be one. This should be ensured by the image.
  assert(merger && (m_pMaster == NULL || depth == 1));
  //
  // If there is any residual information, it is surely not profile D.
  if (residual || hiddenresidualbits > 0)
    profiled = false;
  //
  // Insert elementary flags and settings into the mergingspecbox/alphamergingspecbox.
  merger->DefineHiddenBits(hiddenbits); 
  merger->DefineHiddenResidualBits(hiddenresidualbits);
  merger->DefineResidualBits(rangebits); 
  //
  switch(resflags & 7) {
  case JPGFLAG_RESIDUAL:
  case JPGFLAG_RESIDUALPROGRESSIVE:
    dodct    = false;
    // fall through
  case JPGFLAG_RESIDUALDCT:
    dopart8  = true;
    clipping = false;
    profiled = false;
    break;
  }
  //
  for(component = 0;component < depth;component++) {
    const struct JPG_TagItem *ttag;
    // Build the tone mapping boxes. 
    // Filter out variations we cannot handle.
    if (tags->FindTagItem(JPGTAG_TONEMAPPING_L_FLUT(component)))
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "parts 6,8,9 and part 7 profile C does not support floating point lookup tables");
    //
    if (tags->FindTagItem(JPGTAG_TONEMAPPING_O_TYPE(component)))
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "parts 6,8,9 and part 7 profile C does not support output conversion curves");
    //
    switch (tags->GetTagData(JPGTAG_TONEMAPPING_L_TYPE(component),JPGFLAG_TONEMAPPING_LUT)) {
    case JPGFLAG_TONEMAPPING_LUT:
      lut = BuildToneMapping(tags,JPGTAG_TONEMAPPING_L_TYPE(component),precision + hiddenbits,rangebits + 8);
      // If no LUT, use a linear scaling.
      if (lut) {
        // Define the L-table. Note that the default is no table, thus identity (or scaling).
        merger->DefineLTable(component,lut->TableDestinationOf());
      }
      break;
    case JPGFLAG_TONEMAPPING_IDENTITY:
      if (m_pMaster) { // If this is part of an alpha codestream, we may use parametric.
        assert(component == 0);
        merger->DefineLTable(component,merger->CreateIdentity(1));
      } else {
        JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                  "part 7 profile C requires a lookup table as base non-linear transformation");
      }
      break;
    case JPGFLAG_TONEMAPPING_LINEAR:
      if (m_pMaster) { // Only allowed if part of alpha codestream.
        FLOAT p1 = tags->GetTagFloat(JPGTAG_TONEMAPPING_L_P(component,0),0.0);
        FLOAT p2 = tags->GetTagFloat(JPGTAG_TONEMAPPING_L_P(component,1),1.0);
        merger->DefineLTable(component,merger->CreateLinearRamp(1,p1,p2));
      } else {
        JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                  "part 7 profile C requires a lookup table as base non-linear transformation");
      }
      break;
    default:
      if (m_pMaster) {
        JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                  "alpha channel coding only allows table lookup, identity and linear ramp as L table");
      } else {
        JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                  "part 7 profile C requires a lookup table as base non-linear transformation");
      }
      break;
    }
    //
    // Check whether there are Q or R tables. We don't have them. FIXME: These are allowed!
    // The Q-tables are not allowed in profile C = QPTS.
    ttag = tags->FindTagItem(JPGTAG_TONEMAPPING_Q_TYPE(component));
    if (ttag) {
      switch(ttag->ti_Data.ti_lData) {
      case JPGFLAG_TONEMAPPING_LINEAR: // That's the only allowable type for Q.
        if (!dopart8) {
          FLOAT p1 = tags->GetTagFloat(JPGTAG_TONEMAPPING_L_P(component,0),0.0);
          FLOAT p2 = tags->GetTagFloat(JPGTAG_TONEMAPPING_L_P(component,1),1.0);
          merger->DefineQTable(component,merger->CreateLinearRamp(0,p1,p2));
        } else {
          JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSetting",
                    "part 8 does not allow the usage of a residual non-linear point transformation");
        }
        profiled = false;
        break;
      default:
        JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                  "parts 6,8,9 and part 7 profile C only allow linear ramps as residual NLT transformations");
        break;
      }
    }
    //
    // Ditto for R-tables = DPTS, secondary NLT tables
    if (tags->FindTagItem(JPGTAG_TONEMAPPING_R_TYPE(component)))
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "parts 6,8,9 and part 7 profile C do not allow an intermediate residual non-linear point transformation");
    //
    /*
    // Ditto for R2-tables: NO, these can now be used.
    if (tags->FindTagItem(JPGTAG_TONEMAPPING_R2_TYPE(component)))
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "Profile C does not allow a secondary residual non-linear point transformation");
    */
  }
  //
  if (ltrafo == MergingSpecBox::FreeForm) {
    // Define a custom transformation
    if (depth == 3) {
      merger->DefineLTransformation(merger->ParseFreeFormTransformation(tags,JPGTAG_MATRIX_LMATRIX(0,0)));
    } else {
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "Free form transformations are only available for three-component images");
    }
  } else if (depth == 3) {
    // Must be YCbCr, even if it is disabled. THOR FIX: Nope. Specs say that the MergingSpecBox takes priority.
    merger->DefineLTransformation(ltrafo);
  }
  //
  // Define the C transformation if we have. Otherwise, leave the box undefined as identity
  // is the default anyhow.
  if (tags->FindTagItem(JPGTAG_MATRIX_CMATRIX(0,0))) {
    // Define a custom transformation as well.
    if (depth == 3) {
      merger->DefineCTransformation(merger->ParseFreeFormTransformation(tags,JPGTAG_MATRIX_CMATRIX(0,0)));
    } else {
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "Free form color transformations are only available for three-component images");
    }
    // Not in profile D
    profiled = false;
  }
  //
  // Define the DCT process. This is only valid in part 8 or if we are currently defining the part-9 
  // alpha channel which may also use this.
  if (dopart8 || m_pMaster) {
    if (losslessdct) {
      merger->DefineLDCTProcess(DCTBox::IDCT);
    } else {
      merger->DefineLDCTProcess(DCTBox::FDCT);
    }
  }
  // 
  // Define the residual path.
  if (hdrquality > 0 && residual) {
    LONG rtrafo;
    //
    // Define the R-transformation. This is always the ICT for profile C, though
    // other options exist. The default is NONE if the color transformation
    // is turned off in the legacy path. Otherwise, integer if DCT is disabled
    // in the residual path, otherwise imprecise.
    if (colortrafo == JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE) {
      rtrafo = colortrafo;
    } else {
      if (depth == 3) {
        // No longer depending on the DCT.
        if (dopart8 == false) {
          // DCT is ON. As this is not lossless anyhow, the
          // imprecise transformation works, too.
          rtrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR;
        } else {
          // Here we are encoding in part-8. YCbCr is not a
          // valid option here.
          rtrafo   = JPGFLAG_MATRIX_COLORTRANSFORMATION_RCT;
        }
      } else {
        rtrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE;
      }
    }
    rtrafo = tags->GetTagData(JPGTAG_MATRIX_RTRAFO,rtrafo);
    if (depth == 3) {
      switch(rtrafo) {
      case JPGFLAG_MATRIX_COLORTRANSFORMATION_RCT:
        merger->DefineRTransformation(MergingSpecBox::RCT);
        break;
      case JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE:
        merger->DefineRTransformation(MergingSpecBox::Identity);
        break;
      case JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR:
        merger->DefineRTransformation(MergingSpecBox::YCbCr);
        clipping = true; // creates loss.
        break;
      case JPGFLAG_MATRIX_COLORTRANSFORMATION_FREEFORM:
        // Experimental!
        merger->DefineRTransformation(merger->ParseFreeFormTransformation(tags,JPGTAG_MATRIX_RMATRIX(0,0)));
        clipping = true;
        break;
      default:
        JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                  "the selected color transformation is not available for the residual transformation");
        break;
      }
    } else if (rtrafo != JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE) {
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "The R-Transformation must be the identity for one-component images and the alpha channel");
    }
    // 
    // Define the R2-transformation. This must always be linear.
    for(component = 0;component < depth;component++) {
      const struct JPG_TagItem *jtag;
      FLOAT p1,p2;
      //
      // Find the R2-table.
      jtag = tags->FindTagItem(JPGTAG_TONEMAPPING_R2_TYPE(component));
      if (jtag) {
        if (dopart8)
          JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                    "Part 8 does not support the secondary non-linearity");
        if (jtag->ti_Data.ti_lData != JPGFLAG_TONEMAPPING_LINEAR)
          JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                    "Part 9 and part 7 profile C only supports linear transformations as secondary non-linearity");
        // Assume that *if* this beast is 
        p1 = tags->GetTagFloat(JPGTAG_TONEMAPPING_L2_P(component,0),-0.5);
        p2 = tags->GetTagFloat(JPGTAG_TONEMAPPING_L2_P(component,1),+1.5);
        if (p2 <= p1)
          JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                    "The start of the linear ramp for profile C secondary non-residual must be "
                    "below the end parameter");
        //
        merger->DefineR2Table(component,merger->CreateLinearRamp(0,p1,p2));
      }
    }
    //
    // Define the DCT process in the R-chain. This comes from the RESIDUALX_DCT tags.
    // The default is here to disable the DCT in case the quantization is not
    // relocated, which always happens if the frame type is residual.
    // We may also write this box in part-9 for the alpha channel.
    if (dopart8) {
      if (tags->GetTagData(JPGTAG_RESIDUAL_DCT,dodct)) {
        // Enable the DCT. This must be the lossless dct if we want to do lossless
        // compression.
        merger->DefineRDCTProcess(DCTBox::IDCT);
      } else {
        // Or the regular bypass mode for part-8.
        merger->DefineRDCTProcess(DCTBox::Bypass);
      }
    } else if (m_pMaster) {
      // Enable the DCT.
      merger->DefineRDCTProcess(DCTBox::FDCT);
    }
    //
    assert(m_pResidualData == NULL);
    // And create the container where the data goes. The box type depends on whether we are part
    // of the alpha codestream or not.
    if (m_pMaster) {
      // Part of the alpha codestream. Note that the box needs to be represented top-level.
      m_pResidualData = new(m_pEnviron) class DataBox(m_pEnviron,m_pMaster->m_pBoxList,
                                                      DataBox::AlphaResidualType);
    } else {
      // Part of the regular image data.
      m_pResidualData = new(m_pEnviron) class DataBox(m_pEnviron,m_pBoxList,DataBox::ResidualType);
    }
  }
  //
  // If this is part of the alpha codestream, also create the holder of the alpha codestream.
  assert(m_pAlphaData == NULL);
  if (m_pMaster) {
    // Note that the alpha data is referenced in the alpha tables (thus here) but goes into the boxlist
    // of the master where its lifetime is maintained.
    m_pAlphaData = new(m_pEnviron) class DataBox(m_pEnviron,m_pMaster->m_pBoxList,DataBox::AlphaType);
  }
  //
  // Defaults should be fine for no residual.
  //
  // Enable or disable noise shaping. This is currently consistent for
  // all components here.
  if (dopart8)
    merger->DefineNoiseShaping(noiseshaping);
  //
  // Enable or disable the float support.
  if (m_pMaster) {
    // If we have integer output in part-9, we also need to scale the data to range.
    if (isfloat) {
      // Ensure that the data comes out in float if we want to.
      merger->DefineOutputConversion(isoc);
    } else {
      // Scale the 
      merger->DefineOutputConversionTable(0,merger->CreateLinearRamp(1,0.0,1.0/((1<<(rangebits + 8))-1.0)));
    }
  } else {
    merger->DefineOutputConversion(isoc);
  }
  merger->DefineClipping(clipping);
  merger->DefineLossless(dopart8);
  //
  // Define the alpha settings, but in the alpha codestream.
  if (m_pMaster) {
    // Find the alpha mode and add to the specs.
    // Get the alpha mode. Default is regular.
    LONG mode = tags->GetTagData(JPGTAG_ALPHA_MODE,JPGFLAG_ALPHA_REGULAR);
    ULONG r   = tags->GetTagData(JPGTAG_ALPHA_MATTE(0),0);
    ULONG g   = tags->GetTagData(JPGTAG_ALPHA_MATTE(1),0);
    ULONG b   = tags->GetTagData(JPGTAG_ALPHA_MATTE(2),0);
    if (mode < JPGFLAG_ALPHA_OPAQUE || mode > JPGFLAG_ALPHA_MATTEREMOVAL)
      JPG_THROW(INVALID_PARAMETER,"Tables::CreateProfileCSettings",
                "the specified compositing mode for the alpha channel is invalid");
    merger->SetAlphaMode(AlphaBox::Method(mode),r,g,b);
  }
  //
  // Define the profile now.
  if (profile) {
    if (dopart8) {
      profile->addCompatibility(FileTypeBox::XT_LS);
    } else if (isoc) {
      if (profiled) {
        // Refinement only
        profile->addCompatibility(FileTypeBox::XT_HDR_D);
      } else { // We do have residual scans.
        profile->addCompatibility(FileTypeBox::XT_HDR_C);
      }
    } else {
      // Integer profile.
      profile->addCompatibility(FileTypeBox::XT_IDR);
    }
    if (dopart9) {
      if (isfloat || residual || losslessdct || noiseshaping || 
          (hdrquality > 0 && hdrquality != MAX_ULONG) || rangebits > 0 || hiddenbits > 0 || hiddenresidualbits > 0) {
        // Anything fancy requires full profile alpha
        profile->addCompatibility(FileTypeBox::XT_ALPHA_FULL);
      } else {
        // Just base profile alpha
        profile->addCompatibility(FileTypeBox::XT_ALPHA_BASE);
      }
    }
  }
}
///

/// Tables::AppendRefinementData
// Append a new refinement box on creating refinement scans.
class DataBox *Tables::AppendRefinementData(void)
{
  //
  // Is this the alpha codestream? If so, create the refinement in the parent.
  if (m_pMaster) {
    if (m_pParent) { // alpha residual refinement.
      return new(m_pEnviron) class DataBox(m_pEnviron,m_pMaster->m_pBoxList,DataBox::AlphaResidualRefinementType);
    } else {
      return new(m_pEnviron) class DataBox(m_pEnviron,m_pMaster->m_pBoxList,DataBox::AlphaRefinementType);
    }
  } else {
    // Not in alpha, residual refinement or refinement.
    if (m_pParent) {
      return new(m_pEnviron) class DataBox(m_pEnviron,m_pParent->m_pBoxList,DataBox::ResidualRefinementType);
    } else {
      return new(m_pEnviron) class DataBox(m_pEnviron,m_pBoxList,DataBox::RefinementType);
    }
  }
}
///

/// Tables::RefinementDataOf
// Scan for a refinement box in the box list of this tables class.
// Return the box if it is found, or NULL if no refinement exists. Note that the
// box list is maintained by the top-level tables that applies to the legacy
// codestream.
class DataBox *Tables::RefinementDataOf(UWORD index,ULONG boxtype) const
{
  class Box *box = m_pBoxList;

  //
  // This should really be called from the top-level tables.
  assert(m_pParent == NULL && m_pMaster == NULL);
    
  while(box) {
    if (box->BoxTypeOf()    == boxtype &&
        box->EnumeratorOf() == index) {
      class DataBox *dox = (DataBox *)(box);
      if (dox->isComplete())
        return dox;
    }
    box = box->NextOf();
  }

  return NULL;
}
///

/// Tables::RefinementDataOf
// Return the n'th refinement data if any.
class DataBox *Tables::RefinementDataOf(UWORD index) const
{
  if (m_pMaster) {
    // In the alpha codestream.
    if (m_pParent) {
      return m_pMaster->RefinementDataOf(index,DataBox::AlphaResidualRefinementType);
    } else {
      return m_pMaster->RefinementDataOf(index,DataBox::AlphaRefinementType);
    }
  } else {
    if (m_pParent) {
      return m_pParent->RefinementDataOf(index,DataBox::ResidualRefinementType);
    } else {
      return RefinementDataOf(index,DataBox::RefinementType);
    }
  }
  
  return NULL;
}
///

/// Tables::WriteTables
// Write the tables to the codestream.
void Tables::WriteTables(class ByteStream *io)
{
  if (m_pCameraInfo) {
    io->PutWord(0xffe1); // APP-1 marker
    m_pCameraInfo->WriteMarker(io);
  }
  
  if (m_pResolutionInfo) {
    io->PutWord(0xffe0); // APP-0 marker
    m_pResolutionInfo->WriteMarker(io);
  }
  
  if (m_pQuant) {
    io->PutWord(0xffdb); // DQT table.
    m_pQuant->WriteMarker(io);
  }
 
  if (m_pRestart) {
    io->PutWord(0xffdd);
    m_pRestart->WriteMarker(io);
  }

  if (m_pThresholds) {
    io->PutWord(0xfff8);
    m_pThresholds->WriteMarker(io);
  }

  if (m_pLSColorTrafo) {
    io->PutWord(0xfff8); // Also using LSE
    m_pLSColorTrafo->WriteMarker(io);
  }

  if (m_pColorInfo) {
    io->PutWord(0xffee); // APP-14 marker
    m_pColorInfo->WriteMarker(io);
  }
  
  //
  // Construct and write out all the boxes.
  // The box keeps care of all the 
  Box::WriteBoxMarkers(m_pBoxList,io);
}
///

/// Tables::ParseTables
// Parse off tables, including an application marker,
// comment, huffman tables or quantization tables.
// Returns on the first unknown marker.
void Tables::ParseTables(class ByteStream *io,class Checksum *chk,
                         bool allowexp,bool isls)
{
  bool repeat;
  //
  ParseTablesIncrementalInit(allowexp);
  
  do {
    // Continue reading markers until the end of the
    // tables/misc section has been found.
    repeat = ParseTablesIncremental(io,chk,allowexp,isls);
  } while(repeat);
}
///

/// Tables::ParseTablesIncrementalInit
// Prepare reading an incremental part of the tables. This here must be called first
// before continuing with one or multiple ParseTableIncremental calls.
void Tables::ParseTablesIncrementalInit(bool allowexp)
{
  // no exp-marker here yet. This condition here ensures that the scan header
  // does not overwrite the exp settings of the frame header in a hierarchical
  // scan pattern.
  if (allowexp) {
    m_bFoundExp            = false;
    m_bHorizontalExpansion = false;
    m_bVerticalExpansion   = false;
  }
}
///

/// Tables::ParseTablesIncremental
// Read an incremental part of the tables, namely the next marker.
// Returns true in case the tables/misc section is not yet complete,
// and this function should be called again. Returns false in case
// the tables/misc section is complete.
bool Tables::ParseTablesIncremental(class ByteStream *io,class Checksum *chk,
                                    bool allowexp,bool isls)
{
   LONG marker = io->PeekWord();
   
   switch(marker) {
   case 0xffdb: // DQT
     if (m_pQuant == NULL)
       m_pQuant = new(m_pEnviron) Quantization(m_pEnviron);
     if (chk && ChecksumTables()) {
       class ChecksumAdapter csa(io,chk,false);
       csa.GetWord();
       m_pQuant->ParseMarker(&csa);
     } else {
       io->GetWord();
       m_pQuant->ParseMarker(io);
     }
     break;
   case 0xffc4: // DHT 
     if (m_pHuffman == NULL)
       m_pHuffman = new(m_pEnviron) HuffmanTable(m_pEnviron);
     if (chk && ChecksumTables()) {
       class ChecksumAdapter csa(io,chk,false);
       csa.GetWord();
       m_pHuffman->ParseMarker(&csa);
     } else {
       io->GetWord();
       m_pHuffman->ParseMarker(io);
     }
     break;
   case 0xffcc: // DAC
     if (m_pConditioner == NULL)
       m_pConditioner = new(m_pEnviron) class ACTable(m_pEnviron);
     if (chk && ChecksumTables()) {
       class ChecksumAdapter csa(io,chk,false);
       csa.GetWord();
       m_pConditioner->ParseMarker(&csa);
     } else {
       io->GetWord();
       m_pConditioner->ParseMarker(io);
     }
     break;
   case 0xffdd: // DRI
     if (m_pRestart == NULL)
       m_pRestart = new(m_pEnviron) class RestartIntervalMarker(m_pEnviron,isls);
     if (chk && ChecksumTables()) {
       class ChecksumAdapter csa(io,chk,false);
       csa.GetWord();
       m_pRestart->ParseMarker(&csa);
     } else {
       io->GetWord();
       m_pRestart->ParseMarker(io);
     }
     break;
   case 0xfffe: // COM
     { // The COM-Marker is never checksummed.
       LONG size; 
       io->GetWord();
       size = io->GetWord();
       // Application marker.
       if (size == ByteStream::EOF)
         JPG_THROW(UNEXPECTED_EOF,"Tables::ParseTables","COM marker incomplete, stream truncated");
       //
       if (size <= 0x02)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","COM marker size out of range");
       //
       // Just skip the contents. For now. More later on.
       io->SkipBytes(size - 2);
     }
     break; 
   case 0xfff8: // LSE: JPEG LS extensions marker.
     if (isls) { // Not part of a XT stream, thus checksumming is not required.
       io->GetWord();
       LONG len = io->GetWord();
       if (len > 3) {
         UBYTE id = io->Get();
         if (id == 1) {
           // Thresholds marker.
           if (m_pThresholds == NULL)
             m_pThresholds = new(m_pEnviron) class Thresholds(m_pEnviron);
           m_pThresholds->ParseMarker(io,len);
           break;
         } else if (id == 2 || id == 3) {
           JPG_THROW(NOT_IMPLEMENTED,"Tables::ParseTables",
                     "JPEG LS mapping tables are not implemented by this code, sorry");
         } else if (id == 4) {
           JPG_THROW(NOT_IMPLEMENTED,"Tables::ParseTables",
                     "JPEG LS size extensions are not implemented by this code, sorry");
         } else if (id == 0x0d) {
           // LS Reversible Color transformation
           if (m_pLSColorTrafo)
             JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                       "found duplicate JPEG LS color transformation specification");
           m_pLSColorTrafo = new(m_pEnviron) class LSColorTrafo(m_pEnviron);
           m_pLSColorTrafo->ParseMarker(io,len);
           break;
         } else {
           JPG_WARN(NOT_IMPLEMENTED,"Tables::ParseMarker",
                    "skipping over unknown JPEG LS extensions marker");
         }
       }
       if (len <= 0x02)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       //
       // Just skip the contents. For now. More later on.
       io->SkipBytes(len - 2);
     } else {
       JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","found LSE marker outside of JPEG LS stream");
     }
     break;
   case 0xffe0: // APP0: Maybe the JFIF marker.
     { // APPx markers are not checksummed.
       io->GetWord();
       LONG len = io->GetWord();
       if (len >= 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1) { 
         const char *id = "JFIF";
         while(*id) {
           len--;
           if (io->Get() != *id)
             break;
           id++;
         }
         if (*id == 0) {
           len--;
           if (io->Get() == 0) {
             if (m_pResolutionInfo == NULL)
               m_pResolutionInfo = new(m_pEnviron) class JFIFMarker(m_pEnviron);
             m_pResolutionInfo->ParseMarker(io,len + 5);
             break;
           }
         }
       }
       if (len <= 0x02)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       //
       // Just skip the contents. For now. More later on.
       io->SkipBytes(len - 2);
     }
     break;  
   case 0xffe1: // APP1: Maybe the EXIF marker.
     { // APPx markers are not checksummed.
       io->GetWord();
       LONG len = io->GetWord();
       if (len >= 2 + 4 + 2 + 2 + 2 + 4 + 2) { 
         const char *id = "Exif";
         while(*id) {
           len--;
           if (io->Get() != *id)
             break;
           id++;
         }
         if (*id == 0) {
           len -= 2;
           if (io->GetWord() == 0) {
             if (m_pCameraInfo == NULL)
               m_pCameraInfo = new(m_pEnviron) class EXIFMarker(m_pEnviron);
             m_pCameraInfo->ParseMarker(io,len + 4 + 2);
             break;
           }
         }
       }
       if (len < 2)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       //
       // Just skip the contents. For now. More later on.
       io->SkipBytes(len - 2);
     }
     break;
   case 0xffeb: // APP11: Maybe the box marker.
     { // APPx markers are not checksummed.
       io->GetWord();
       LONG len = io->GetWord();
       if (len >= 2 + 2 + 2 + 4 + 4 + 4) { // At least the box header must be present.
         LONG ci = io->PeekWord();
         if (ci == 0x4a50) { 
           class Box *box;
           // Is the correct CI, assume it is a box.
           io->GetWord();
           // 
           // The rest of the logic is part of the boxing.
           if (m_pParent)
             JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                       "Found a box in the residual codestream.");
           box = Box::ParseBoxMarker(this,m_pBoxList,io,len);
           //
           // If the box is complete, check whether we can short-cut the box-search
           // by storing the data here as soon as the box is ready.
           if (box) {
             switch(box->BoxTypeOf()) {
             case MergingSpecBox::SpecType:
               if (m_pResidualSpecs)
                 JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                           "Found a duplicate Merging Specification Box, there must be at most one.");
               m_pResidualSpecs  = (class MergingSpecBox *)box;
               break;
             case MergingSpecBox::AlphaType:
               if (m_pAlphaSpecs)
                 JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                           "Found a duplicate Alpha Merging Specification Box, there must be at most one.");
               m_pAlphaSpecs = (class MergingSpecBox *)box;
               break;
             case ChecksumBox::Type:
               if (m_pChecksumBox)
                 JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                           "Found a duplicate Checksum Box, there must be at most one.");
               m_pChecksumBox = (class ChecksumBox *)box;
               break;
             case DataBox::AlphaType:
               {
                 class Tables *alpha = CreateAlphaTables();
                 if (alpha->m_pAlphaData)
                   JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                             "Found a duplicate Alpha Data Box, there must be at most one.");
                 alpha->m_pAlphaData = (class DataBox *)box;
               }
               break;
             case DataBox::ResidualType:
               if (m_pResidualData)
                 JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                           "Found a duplicate Residual Data Box, there must be at most one.");
               m_pResidualData   = (class DataBox *)box;
               break;
             case DataBox::AlphaResidualType:
               {
                 class Tables *alpha = CreateAlphaTables();
                 if (alpha->m_pResidualData)
                   JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                             "Found a duplicate Residual Alpha Data Box, there must be at most one.");
                 alpha->m_pResidualData = (class DataBox *)box;
               }
               break;
             case DataBox::RefinementType:
               m_bRefinement = true;
               break;
             case DataBox::ResidualRefinementType:
               CreateResidualTables()->m_bRefinement = true;
               break;
             case DataBox::AlphaRefinementType:
               CreateAlphaTables()->m_bRefinement = true;
               break;
             case DataBox::AlphaResidualRefinementType:
               CreateAlphaTables()->CreateResidualTables()->m_bRefinement = true;
               break;
             case InverseToneMappingBox::Type:
             case FloatToneMappingBox::Type:
               {
                 class ToneMapperBox *tmo = dynamic_cast<ToneMapperBox *>(box);
                 assert(tmo);
                 if (!m_NameSpace.isUniqueNonlinearity(tmo->TableDestinationOf()))
                   JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                             "Malformed JPEG stream - found a doubly used table destination for a nonlinearity box");
               }
               break;
             case LinearTransformationBox::Type:
             case FloatTransformationBox::Type:
               {
                 class MatrixBox *matrix = dynamic_cast<MatrixBox *>(box);
                 assert(matrix);
                 if (!m_NameSpace.isUniqueMatrix(matrix->IdOf()))
                   JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                             "Malformed JPEG stream - found a doubly used table destination for a matrix box");
               }
               break;
             }
           }
           break;
         }
       }
       if (len < 2)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       
       io->SkipBytes(len - 2);
     }
     break;
   case 0xffee: // APP14: Maybe the adobe marker.
     { // APPx markers are not checksummed.
       io->GetWord();
       LONG len = io->GetWord();
       if (len == 2 + 5 + 2 + 2 + 2 + 1) { 
         const char *id = "Adobe";
         while(*id) {
           len--;
           if (io->Get() != *id)
             break;
           id++;
         }
         if (*id == 0) {
           if (m_pColorInfo == NULL)
             m_pColorInfo = new(m_pEnviron) class AdobeMarker(m_pEnviron);
           m_pColorInfo->ParseMarker(io,len + 5);
           break;
         }
       }
       if (len < 2)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       //
       // Just skip the contents. For now. More later on.
       io->SkipBytes(len - 2);
     }
     break;
   case 0xffdf: 
     // EXP marker.    
     if (m_bFoundExp) {
       JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","found a double EXP marker between frames");
     } else {
       LONG len;
       UBYTE ehv,evv;
       //
       // This is so simple, no need to have a separate class.
       io->GetWord(); // remove the marker
       len = io->GetWord();
       if (len != 3)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","EXP marker size is invalid, must be three");
       len = io->Get();
       if (len == ByteStream::EOF)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","unexpected EOF while parsing the EXP marker");
       ehv = len >> 4;
       evv = len & 0x0f;
       if (ehv > 1 || evv > 1)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                   "invalid EXP marker, horizontal and vertical expansion "
                   "may be at most one");  
       if (!allowexp)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
                   "found an EXP marker outside a hierarchical process");
       m_bFoundExp            = true;
       m_bHorizontalExpansion = (ehv)?true:false;
       m_bVerticalExpansion   = (evv)?true:false;
     }
     break;
   case 0xffc8: // The JPEG Extensions marker
     {
       LONG len;
       
       io->GetWord(); // remove the marker
       len = io->GetWord();
       if (len < 2)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       //
       // Just skip the contents. For now. The JPEG Extensions marker is not in use.
       io->SkipBytes(len - 2);
     }
     break;
   case 0xffc0:
   case 0xffc1:
   case 0xffc2:
   case 0xffc3:
   case 0xffc5:
   case 0xffc6:
   case 0xffc7:
   case 0xffc9:
   case 0xffca:
   case 0xffcb:
   case 0xffcd:
   case 0xffce:
   case 0xffcf: // all start of frame markers.
   case 0xffb1: // residual scan
   case 0xffb2: // residual scan, progressive
   case 0xffb3: // residual scan, large dct
   case 0xffb9: // residual scan, ac coded
   case 0xffba: // residual scan, ac coded, progressive
   case 0xffbb: // residual scan, ac coded, large dct
   case 0xffd9: // EOI
   case 0xffda: // Start of scan.
   case 0xffde: // DHP
   case 0xfff7: // JPEG LS SOF55
     return false;
   case 0xffff: // A filler byte followed by a marker. Skip.
     io->Get();
     break;
   case 0xffd0:
   case 0xffd1:
   case 0xffd2:
   case 0xffd3:
   case 0xffd4:
   case 0xffd5:
   case 0xffd6:
   case 0xffd7: // Restart markers. These should not go here.
     io->GetWord();
     JPG_WARN(MALFORMED_STREAM,"Tables::ParseTables","found a stray restart marker segment, ignoring");
     break;
   default: 
     if (marker >= 0xffc0 && (marker < 0xffd0 || marker >= 0xffd8) && marker < 0xfff0) {
       LONG size;
       io->GetWord();
       size = io->GetWord();
       // Application marker.
       if (size == ByteStream::EOF)
         JPG_THROW(UNEXPECTED_EOF,"Tables::ParseTables","marker incomplete, stream truncated");
       //
       if (size <= 0x02)
         JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
       //
       // Just skip the contents. For now. More later on.
       // APPx is not checksummed.
       io->SkipBytes(size - 2);
     } else {
       LONG dt;
       //
       JPG_WARN(MALFORMED_STREAM,"Tables::ParseTables",
                "found invalid marker, probably a marker size is out of range");
       // Advance to the next marker manually.
       io->Get();
       do {
         dt = io->Get();
       } while(dt != 0xff && dt != ByteStream::EOF);
       //
       if (dt == 0xff) {
         io->LastUnDo();
       } else {
         return false;
       }
     }
   }

   return true;
}
///

/// Tables::FindDCHuffmanTable
// Find the DC huffman table of the indicated index.
class HuffmanTemplate *Tables::FindDCHuffmanTable(UBYTE idx,ScanType type,
                                                  UBYTE depth,UBYTE hidden,UBYTE scan) const
{
  class HuffmanTemplate *t;

  if (m_pHuffman == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindDCHuffmanTable","DHT marker missing for Huffman encoded scan");

  t = m_pHuffman->DCTemplateOf(idx,type,depth,hidden,scan);
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindDCHuffmanTable","requested DC huffman coding table not defined");
  return t;
}
///

/// Tables::FindACHuffmanTable
// Find the AC huffman table of the indicated index.
class HuffmanTemplate *Tables::FindACHuffmanTable(UBYTE idx,ScanType type,
                                                  UBYTE depth,UBYTE hidden,UBYTE scan) const
{ 
  class HuffmanTemplate *t;

  if (m_pHuffman == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindACHuffmanTable","DHT marker missing for Huffman encoded scan");

  t = m_pHuffman->ACTemplateOf(idx,type,depth,hidden,scan);
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindACHuffmanTable","requested AC huffman coding table not defined");
  return t;
}
///

/// Tables::FindDCConditioner
class ACTemplate *Tables::FindDCConditioner(UBYTE idx,ScanType type,
                                            UBYTE depth,UBYTE hidden,UBYTE scan) const
{
  if (m_pConditioner) {
    return m_pConditioner->DCTemplateOf(idx,type,depth,hidden,scan);
  }

  return NULL;
}
///

/// Tables::FindACConditioner
class ACTemplate *Tables::FindACConditioner(UBYTE idx,ScanType type,
                                            UBYTE depth,UBYTE hidden,UBYTE scan) const
{
  if (m_pConditioner) {
    return m_pConditioner->ACTemplateOf(idx,type,depth,hidden,scan);
  }

  return NULL;
}
///

/// Tables::FindQuantizationTable
// Find the quantization table of the given index.
class QuantizationTable *Tables::FindQuantizationTable(UBYTE idx) const
{
  class QuantizationTable *t;
  
  if (m_pQuant == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindQuantizationTable","DQT marker missing, no quantization table defined");

  t = m_pQuant->QuantizationTable(idx);
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindQuantizationTable","requested quantization matrix not defined");
  return t;
}
///

/// Tables::ColorTrafoOf
// Return the color transformer.
class ColorTrafo *Tables::ColorTrafoOf(class Frame *frame,class Frame *residualframe,UBYTE type,
                                       bool encoding,bool disabletorgb)
{
  if (m_pColorTrafo == NULL) {
    UBYTE dctbits,spatialbits;
    UBYTE bpp = frame->PrecisionOf();
    class MergingSpecBox *specs = ResidualSpecsOf();

    assert(m_pParent == NULL);
    
    if (m_pColorFactory == NULL)
      m_pColorFactory = new(m_pEnviron) class ColorTransformerFactory(this);
    
    if (specs) {
      UBYTE hiddenbits   = specs->HiddenBitsOf();
      UBYTE residualbits = specs->ResidualBitsOf();
      // 
      dctbits     = bpp + hiddenbits;
      spatialbits = bpp + residualbits; 
      //
      // Bits per pixel must then be eight by the specs.
      if (m_pRefinementData || m_pResidualData) {
        if (bpp != 8)
          JPG_THROW(MALFORMED_STREAM,"Tables::ColorTrafoOf",
                    "Residual or refinement coding requires a coding precision of 8 bits per sample");
      }
    } else {
      dctbits     = bpp;
      spatialbits = bpp;
    }
    
    assert(!m_bTruncateColor);
    m_pColorTrafo = m_pColorFactory->BuildColorTransformer(frame,residualframe,
                                                           specs,dctbits,spatialbits,
                                                           type,encoding,disabletorgb);
  }
  
  return m_pColorTrafo;
}
///

/// Tables::HiddenDCTBitsOf
// Check how many bits are hidden in invisible refinement scans.
UBYTE Tables::HiddenDCTBitsOf(void) const
{
  class MergingSpecBox *specs = ResidualSpecsOf();

  if (specs) {
    if (m_pParent) {
      return specs->HiddenResidualBitsOf();
    } else {
      return specs->HiddenBitsOf();
    }
  }

  return 0;
}
///

/// Tables::UseResiduals
// Check whether residual data in the APP11 marker shall be written.
bool Tables::UseResiduals(void) const
{
  if (m_pResidualData || m_pParent)
    return true;
  return false;
}
///

/// Tables::UseRefinements
// Check whether refinement data shall be written.
bool Tables::UseRefinements(void) const
{
  return m_bRefinement;
}
///

/// Tables::FractionalLBitsOf
// Return the number of fractional bits in the L-path.
UBYTE Tables::FractionalLBitsOf(UBYTE count,bool dct) const
{
  if (m_pParent)
    return m_pParent->FractionalColorBitsOf(count,dct);
  else
    return FractionalColorBitsOf(count,dct);
}
///

/// Tables::FractionalRBitsOf
// Return the number of fractional bits in the R-path.
UBYTE Tables::FractionalRBitsOf(UBYTE count,bool dct) const
{
  if (m_pResidualTables)
    return m_pResidualTables->FractionalColorBitsOf(count,dct);
  else
    return FractionalColorBitsOf(count,dct);
}
///

/// Tables::FractionalColorBitsOf
// Check how many fractional bits the color transformation will use.
// The DCT flag indicates whether a DCT is in the path. If so, more bits might be allocated
// to accommodate fractional output bits of the DCT. Note that this
// is an implementation detail.
UBYTE Tables::FractionalColorBitsOf(UBYTE count,bool) const
{  
  MergingSpecBox::DecorrelationType dm;

  if (m_pParent) {
    dm = RTrafoTypeOf(count);
  } else {
    dm = LTrafoTypeOf(count);
  }

  switch (dm) {
  case MergingSpecBox::Identity:
    // This is a strange beast. For residual, there are no fractional bits.
    // For legacy, there are for the full DCT and also for the color transformer,
    // so the bits must be there, regardless of the dct.
    // NOTE: The standard specifies for the lifting DCT a preshift of zero bits.
    // What happens here is that the DCT in the legacy pass also preshifts by four bits,
    // but this preshift is removed in the color transformer which operates with a consistent
    // preshift of four for the legacy coding pass.
    // This trick works because the 1-1 DCT generates all integer output.
    if (m_pParent == NULL)
      return ColorTrafo::COLOR_BITS;
    if (isLossless())
      return 0; // No fractional bits needed.
    // Lossy also uses fractional bits in the residual domain to limit the loss.
    return ColorTrafo::COLOR_BITS;
  case MergingSpecBox::Zero:
  case MergingSpecBox::JPEG_LS:
    return 0; // No fractional bits needed.
  case MergingSpecBox::YCbCr:
    return ColorTrafo::COLOR_BITS;
  case MergingSpecBox::RCT:
    return 1; // One additional bit required here.
  default:
    return ColorTrafo::COLOR_BITS; // This is the freeform transformation.
  }
  //
  // Huh? How did the code go here.
  return 0;
}
///

/// Tables::UseLosslessDCT
// Check whether to use the Lossless DCT transformation.
bool Tables::UseLosslessDCT(void) const
{
  class MergingSpecBox *specs = ResidualSpecsOf();
  
  if (specs) {
    if (m_pParent) {
      return (specs->RDCTProcessOf() == DCTBox::IDCT);
    } else {
      return (specs->LDCTProcessOf() == DCTBox::IDCT);
    }
  }
    
  return false;
}
///

/// Tables::isLossless
// Check whether the lossless flag is set.
bool Tables::isLossless(void) const
{
  class MergingSpecBox *specs = ResidualSpecsOf();

  if (specs && specs->isLossless())
    return true;
  
  return false;
}
///

/// Tables::isChromaCentered
// Return a flag that indicates whether chroma samples are
// centered or cosited. Returns true if they are cosited.
bool Tables::isChromaCentered(void) const
{
  // Currently, upsampling is centered.
  return true;
}
///

/// Tables::isDownsamplingInterpolated
// Return a flag indicating whether the downsampler at encoder
// side should enable interpolation (then true) or if a simple
// box filter is sufficient (then false).
// Currently, residual coding requires the box filter.
bool Tables::isDownsamplingInterpolated(void) const
{
  /* This is currently disabled. */
  return false;
  /*
  if (UseResiduals())
    return false;
  return true;
  */
}
///

/// Tables::hasSeparateChroma
// Test whether this setup has designated chroma components. For the
// legacy codestream, this tests whether there is an L transformation in
// the path. For the residual codestream, this tests for an R-transformation.
bool Tables::hasSeparateChroma(UBYTE depth) const
{  
  if (isResidualTable()) {
    if (RTrafoTypeOf(depth) != MergingSpecBox::Identity)
      return true;
  } else {
    if (LTrafoTypeOf(depth) != MergingSpecBox::Identity)
      return true;
  }
  return false;
}
///

/// Tables::BuildDCT
// Build the proper DCT transformation for the specification
// recorded in this class. This is the DCT for the L-branch.
class DCT *Tables::BuildDCT(class Component *comp,UBYTE count,UBYTE precision) const
{
  UBYTE fractional = FractionalColorBitsOf(count,true); // This is always with the DCT, of course
  bool  lossless   = UseLosslessDCT();
  class DCT *dct   = NULL;
  class QuantizationTable *quant;

  // Ok, so get the quantization matrix there, i.e. in our table because
  // this is the residual table.
  quant    = FindQuantizationTable(comp->QuantizerOf());
  
  if (lossless) { 
    // NOTE: The standard specifies for the lifting DCT a preshift of zero bits.
    // What happens here is that the DCT in the legacy pass also preshifts by four bits,
    // but this preshift is removed in the color transformer which operates with a consistent
    // preshift of four for the legacy coding pass.
    if (m_pParent) {
      // In the residual path, this hopefully means that lossless coding is required
      // and hence the preshift should not be removed.
      assert(fractional == 0 || fractional == 1);
      if (fractional + precision + 12 + 3 > 31) {
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,QUAD,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,QUAD,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,QUAD,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,QUAD,false,false>(m_pEnviron);
          }
        }
      } else {
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,false,false>(m_pEnviron);
          }
        }
      }
    } else {
      switch(fractional) {
      case 0:
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<0,LONG,false,false>(m_pEnviron);
          }
        }
        break;
      case 1:
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<1,LONG,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<1,LONG,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<1,LONG,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<1,LONG,false,false>(m_pEnviron);
          }
        }
        break;
      case ColorTrafo::COLOR_BITS:
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<ColorTrafo::COLOR_BITS,LONG,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<ColorTrafo::COLOR_BITS,LONG,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSLESSDCT<ColorTrafo::COLOR_BITS,LONG,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSLESSDCT<ColorTrafo::COLOR_BITS,LONG,false,false>(m_pEnviron);
          }
        }
        break;
      default:
        JPG_THROW(INVALID_PARAMETER,"Tables::BuildDCT",
                  "invalid combination of color transformation and DCT");
        dct = NULL;
        break;
      }
    }
  } else {
    switch(fractional) {
    case 0:
      if (m_bDeadZone) {
        if (m_bOptimize) {
          dct = new(m_pEnviron) class LOSSYDCT<0,LONG,true,true>(m_pEnviron);
        } else {
          dct = new(m_pEnviron) class LOSSYDCT<0,LONG,true,false>(m_pEnviron);
        }
      } else {
        if (m_bOptimize) {
          dct = new(m_pEnviron) class LOSSYDCT<0,LONG,false,true>(m_pEnviron);
        } else {
          dct = new(m_pEnviron) class LOSSYDCT<0,LONG,false,false>(m_pEnviron);
        }
      }
      break;
    case 1: // This is for the RCT which is range-extending.
      if (m_bDeadZone) {
        if (m_bOptimize) {
          dct = new(m_pEnviron) class LOSSYDCT<1,LONG,true,true>(m_pEnviron);
        } else {
          dct = new(m_pEnviron) class LOSSYDCT<1,LONG,true,false>(m_pEnviron);
        }
      } else {
        if (m_bOptimize) {
          dct = new(m_pEnviron) class LOSSYDCT<1,LONG,false,true>(m_pEnviron);
        } else {
          dct = new(m_pEnviron) class LOSSYDCT<1,LONG,false,false>(m_pEnviron);
        }
      }
      break;
    case ColorTrafo::COLOR_BITS:
      if (precision > 12) {
        // This might be 20 bits large, so be careful.
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,QUAD,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,QUAD,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,QUAD,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,QUAD,false,false>(m_pEnviron);
          }
        }
      } else {
        if (m_bDeadZone) {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,LONG,true,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,LONG,true,false>(m_pEnviron);
          }
        } else {
          if (m_bOptimize) {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,LONG,false,true>(m_pEnviron);
          } else {
            dct = new(m_pEnviron) class LOSSYDCT<ColorTrafo::COLOR_BITS,LONG,false,false>(m_pEnviron);
          }
        }
      }
      break;
    }
  }
  
  if (dct == NULL) {
    JPG_THROW(INVALID_PARAMETER,"Tables::BuildDCT","unsupported DCT requested");
  } else {
    dct->DefineQuant(quant); // does not throw
  }

  return dct;
}
///


/// Tables::RestartIntervalOf
// Return the currently active restart interval in MCUs or zero
// in case restart markers are disabled.
ULONG Tables::RestartIntervalOf(void) const
{
  if (m_pRestart)
    return m_pRestart->RestartIntervalOf();
  return 0;
}
///

/// Tables::BuildToneMapping
// Build a tone mapping for the type (base-tag) and the given tag list
// The base tag is the tag-id for the type of the box. All others are offsets.
class ToneMapperBox *Tables::BuildToneMapping(const struct JPG_TagItem *tags,
                                              JPG_Tag basetag,UBYTE inbits,UBYTE outbits)
{
  UBYTE idx = 0;
  class Box *box = m_pBoxList;
  class InverseToneMappingBox *itm;
  class FloatToneMappingBox   *ftm;
  const UWORD *lut  = NULL; // shut down the compiler.
  const FLOAT *flut = NULL;
  
  // The lookup table, if there is one.
  lut  = (const UWORD *)(tags->GetTagPtr(basetag + 8));
  flut = (const FLOAT *)(tags->GetTagPtr(basetag + 9));
  //
  // We can no longer create parametric boxes here. These should better all go into
  // the merging spec box.
  //
  // No data? Ok, then no box.
  if (lut == NULL && flut == NULL)
    return NULL;

  
  // Check whether there is already a table here we could use.
  while(box) {
    itm = dynamic_cast<InverseToneMappingBox *>(box);
    ftm = dynamic_cast<FloatToneMappingBox *>(box);
    if (itm != NULL && lut != NULL) {
      if (itm->CompareTable(lut,1UL << inbits,outbits - 8))
        return itm;
    } else if (ftm != NULL && flut != NULL) {
      if (ftm->CompareTable(flut,1UL << inbits,outbits - 8))
        return ftm;
    }
    box = box->NextOf();
  }

  //
  // Ok, no box. Create one. 
  // First get the next available ID for NLTs.
  idx  = m_NameSpace.AllocateNonlinearityID();
  //
  // Boxes go already in the boxlist when created, thus there is no leakage here.
  if (lut) {
    itm = new(m_pEnviron) class InverseToneMappingBox(m_pEnviron,m_pBoxList);
    itm->DefineTable(idx,lut,1UL << inbits,outbits - 8);
    return itm;
  } else if (flut) {
     ftm = new(m_pEnviron) class FloatToneMappingBox(m_pEnviron,m_pBoxList);
     ftm->DefineTable(idx,flut,1UL << inbits,outbits - 8);
    return ftm;
  }

  return NULL;
}
///

/// Tables::LTrafoTypeOf
// Return the effective color transformation for the L-transformation.
MergingSpecBox::DecorrelationType Tables::LTrafoTypeOf(UBYTE components) const
{
  class MergingSpecBox *specs = ResidualSpecsOf();

  if (specs) {
    MergingSpecBox::DecorrelationType ltrafo = specs->LTransformationOf();
    //
    if (components == 1 && ltrafo != MAX_UBYTE) // this should simply not exist in this case!
      JPG_THROW(MALFORMED_STREAM,"Tables::LTrafoTypeOf",
                "Base transformation box exists even though the number of components is one");
    //
    switch(ltrafo) {
    case MergingSpecBox::Zero:
    case MergingSpecBox::JPEG_LS:
    case MergingSpecBox::RCT: // These choices are invalid.
      JPG_THROW(MALFORMED_STREAM,"Tables::LTrafoTypeOf",
                "Found an invalid base transformation, must be YCbCr, identity or free-form");
      break;
    case MergingSpecBox::YCbCr:
    case MergingSpecBox::Identity:
      return ltrafo;
    case MergingSpecBox::Undefined:
      break; // runs into the code below.
    default: // 5..15
      return ltrafo;
    }
  }
  //
  // No specs, use the default mechanism of JPEG.
  if (components < 3 || components > 3 ||
      (m_pColorInfo && m_pColorInfo->EnumeratedColorSpaceOf() == AdobeMarker::None)) {
    return MergingSpecBox::Identity; // Do not transform.
  } else if (m_pLSColorTrafo) {
    return MergingSpecBox::JPEG_LS;  // special rule for JPEG-LS here.
  } else {
    return MergingSpecBox::YCbCr;
  }  
}
///

/// Tables::RTrafoTypeOf
// Return the effective color transformation for the R-transformation.
MergingSpecBox::DecorrelationType Tables::RTrafoTypeOf(UBYTE components) const
{ 
  class MergingSpecBox *specs = ResidualSpecsOf();

  if (specs) {
    MergingSpecBox::DecorrelationType rtrafo = specs->RTransformationOf();
    //
    switch(rtrafo) {
    case MergingSpecBox::Zero:
    case MergingSpecBox::JPEG_LS:
      JPG_THROW(MALFORMED_STREAM,"Tables::LTrafoTypeOf",
                "Found an invalid residual transformation");
    case MergingSpecBox::YCbCr:
    case MergingSpecBox::Identity:
    case MergingSpecBox::RCT:
      return rtrafo;
    case MergingSpecBox::Undefined:
      if (m_pParent || m_pResidualData) {
        // Ok, there is a residual. Either because we are the parent, or because we have one.
        // For one component, the box simply does not exist.
        if (components == 1) {
          return MergingSpecBox::Identity;
        } else {
          return MergingSpecBox::YCbCr;
        }
      } else {
        return MergingSpecBox::Zero;
      }
    default:
      // Free form.
      return rtrafo;
    }
  }
  //
  // No residual specs, no residual data.
  return MergingSpecBox::Zero;
}
///


/// Tables::CTrafoTypeOf
// Return the effective color transformation for the C-transformation.
MergingSpecBox::DecorrelationType Tables::CTrafoTypeOf(UBYTE components) const
{
  class MergingSpecBox *specs = ResidualSpecsOf();
  MergingSpecBox::DecorrelationType ctrafo = MergingSpecBox::Identity; // Nothing happening here if no residual
    
  if (specs) {
    ctrafo = specs->CTransformationOf();
    //
    if (components == 1 && ctrafo != MergingSpecBox::Undefined) // this should simply not exist in this case!
      JPG_THROW(MALFORMED_STREAM,"Tables::CTrafoTypeOf",
                "Color transformation box exists even though the number of components is one");
    
    if (components == 1 || ctrafo == MergingSpecBox::Undefined) {
      ctrafo = MergingSpecBox::Identity;
    } else if (ctrafo != MergingSpecBox::Identity && ctrafo < MergingSpecBox::FreeForm) {
      if (ctrafo < MergingSpecBox::FreeForm)
        JPG_THROW(MALFORMED_STREAM,"Tables::CTrafoTypeOf","Found an invalid color space conversion");
    }
  }
    
  return ctrafo;
}
///

/// Tables::ImageNamespace
// Return the regular name space, namely that of the image.
class NameSpace *Tables::ImageNamespace(void)
{
  if (m_pMaster)
    return m_pMaster->ImageNamespace();
  if (m_pParent)
    return m_pParent->ImageNamespace();

  return &m_NameSpace;
}
///

/// Tables::AlphaNamespace
// Return the common name space for the alpha channel.
class NameSpace *Tables::AlphaNamespace(void)
{
  if (m_pMaster)
    return m_pMaster->AlphaNamespace();
  if (m_pParent)
    return m_pParent->AlphaNamespace();

  return &m_AlphaNameSpace;
}
///
