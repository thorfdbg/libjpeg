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
** This class parses the markers and holds the decoder together.
**
** $Id: encoder.cpp,v 1.55 2021/09/08 10:30:06 thor Exp $
**
*/

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "codestream/encoder.hpp"
#include "io/bytestream.hpp"
#include "codestream/tables.hpp"
#include "codestream/image.hpp"
#include "marker/scantypes.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "boxes/mergingspecbox.hpp"
#include "std/assert.hpp"
///

/// Encoder::Encoder
// Create the default encoder
Encoder::Encoder(class Environ *env)
  : JKeeper(env), m_pImage(NULL)
{
}
///

/// Encoder::~Encoder
// Delete the encoder
Encoder::~Encoder(void)
{
  delete m_pImage;
}
///

/// Encoder::FindScanTypes
// Create from a set of parameters the proper scan type.
// This fills in the scan type of the base image and the residual image,
// the number of refinement scans in the LDR and HDR domain, the
// frame precision (excluding hidden/refinement bits) in the base and extension layer
// and the number of additional precision bits R_b in the spatial domain.
void Encoder::FindScanTypes(const struct JPG_TagItem *tags,LONG defaultscan,UBYTE defaultdepth,
                            ScanType &scantype,ScanType &restype,
                            UBYTE &hiddenbits,UBYTE &riddenbits,
                            UBYTE &ldrprecision,UBYTE &hdrprecision,
                            UBYTE &rangebits) const
{  
  LONG frametype   = tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE);
  LONG resflags    = tags->GetTagData(JPGTAG_RESIDUAL_FRAMETYPE,defaultscan);
  ULONG depth      = tags->GetTagData(JPGTAG_IMAGE_DEPTH,defaultdepth);
  bool accoding    = (frametype & JPGFLAG_ARITHMETIC)?true:false; 
  bool raccoding   = (resflags  & JPGFLAG_ARITHMETIC)?true:false;
  bool residual    = (frametype & JPGFLAG_RESIDUAL_CODING)?true:false;

  hiddenbits       = tags->GetTagData(JPGTAG_IMAGE_HIDDEN_DCTBITS,0);
  riddenbits       = tags->GetTagData(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,0); 
  ldrprecision     = tags->GetTagData(JPGTAG_IMAGE_PRECISION,8);
  hdrprecision     = 0;
  rangebits        = 0;
  
  switch(frametype & 0x07) {
  case JPGFLAG_BASELINE:
    scantype = Baseline;
    if (accoding)
      JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","baseline coding does not allow arithmetic coding");
    break;
  case JPGFLAG_SEQUENTIAL:
    scantype = Sequential;
    if (accoding)
      scantype = ACSequential;
    break;
  case JPGFLAG_PROGRESSIVE:
    scantype = Progressive;
    if (accoding)
      scantype = ACProgressive;
    break;
  case JPGFLAG_LOSSLESS:
    scantype = Lossless;
    if (accoding)
      scantype = ACLossless;
    break;
  case JPGFLAG_JPEG_LS:
    scantype = JPEG_LS;
    break; // Could enable AC coding, maybe? In future revisions.
  case JPGFLAG_RESIDUAL:
    JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","Residual scan type not available for legacy codestream");
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","specified invalid frame type");
    break;
  }
  
  if (resflags & JPGFLAG_PYRAMIDAL)
    JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","Residual image cannot be hierarchical");
  
  if (resflags & JPGFLAG_RESIDUAL_CODING)
    JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","Residual image cannot contain another residual");
   
  switch(resflags & 0x07) {
  case JPGFLAG_RESIDUAL:
    restype = Residual;
    if (accoding || raccoding) 
      // since the frame type is never indicated, the ac-coding flag comes from the main scan here.
      restype = ACResidual; // actually, this is not official.
    break;
  case JPGFLAG_RESIDUALDCT:
    restype = ResidualDCT;
    if (accoding || raccoding)
      restype = ACResidualDCT; // inofficial.
    break;
  case JPGFLAG_RESIDUALPROGRESSIVE:
    restype = ResidualProgressive;
    if (accoding || raccoding)  
      // since the frame type is never indicated, the ac-coding flag comes from the main scan here.
      restype = ACResidualProgressive; // actually, this is not official.
    break;
  case JPGFLAG_BASELINE:
    restype = Baseline;
    if (accoding || raccoding)
      JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","baseline coding does not allow arithmetic coding");
    break; 
  case JPGFLAG_SEQUENTIAL:
    restype = Sequential;
    if (accoding || raccoding)
      restype = ACSequential;
    break;
  case JPGFLAG_PROGRESSIVE:
    restype = Progressive;
    if (accoding || raccoding)
      restype = ACProgressive; // actually, this is not official
    break;
  case JPGFLAG_LOSSLESS:
    restype = Lossless;  // actually, this is not official
    if (accoding || raccoding)
      restype = ACLossless;
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","specified invalid frame type for residual image");
    break;
  } 
  
  if (hiddenbits) {
    if (int(hiddenbits) > ldrprecision - 8)
      JPG_THROW(OVERFLOW_PARAMETER,"Encoder::FindScanTypes",
                "can only hide at most the number of extra bits between "
                "the native bit depth of the image and eight bits per pixel");
    if (hiddenbits + 8 > 12)
      JPG_THROW(OVERFLOW_PARAMETER,"Encoder::FindScanTypes",
                "the maximum number of hidden DCT bits can be at most four");
  }

  if (residual || hiddenbits || riddenbits) {
    switch(frametype & 0x07) {
    case JPGFLAG_BASELINE:
    case JPGFLAG_SEQUENTIAL:
    case JPGFLAG_PROGRESSIVE:
      if (ldrprecision > 8) {
        rangebits    = ldrprecision - 8;
        ldrprecision = 8;
      }
      break;
    }
  }

  switch(scantype) {
  case Baseline:
    if (ldrprecision != 8)
      JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","baseline Huffman coding only supports 8bpp scans");
    break;
  case Sequential:
  case Progressive:
  case ACSequential:
  case ACProgressive:
    if (ldrprecision != 8 && ldrprecision != 12)
      JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes","JPEG supports only 8 or 12 bit sample precision");
    break;
  default: 
    // lossless and LS support arbitrary precision.
    break;
  }
  
  if (residual) {
    //
    switch(scantype) {
    case Lossless:
    case ACLossless:
    case JPEG_LS:
      JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes",
                "the lossless scans do not create residuals, no need to code them");
      break;
    case DifferentialSequential:
    case DifferentialProgressive:
    case DifferentialLossless:
    case ACDifferentialSequential:
    case ACDifferentialProgressive:
    case ACDifferentialLossless:
      // Hmm. At this time, simply disallow. There is probably a way how to fit this into
      // the highest hierarchical level, but not now.
      JPG_THROW(NOT_IMPLEMENTED,"Encoder::FindScanTypes",
                "the hierarchical mode does not yet allow residual coding");
      break;
    default:
      // Ok, try that.
      break;
    } 
    // The residual scans are not JPEG images, and can store their content
    // in full range.
    switch(restype) {
      // All the fancy scantypes for part-8.
    case Residual:
    case ResidualProgressive:
    case ACResidual:
    case ACResidualProgressive:
    case ResidualDCT:
    case ACResidualDCT:
      hdrprecision = ldrprecision + rangebits;
      // If we have bits left, and the color trafo in the residual domain is the RCT,
      // take care of its range expansion.
      {
        ULONG rtrafo     = JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE;
        ULONG colortrafo = tags->GetTagData(JPGTAG_MATRIX_LTRAFO,
                                            (depth > 1)?JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR:
                                            JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE);
        // The internal logic will rewire the default of YCVBCR to RCT for lossless, thus do the
        // same here.
        if (colortrafo != JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE && depth == 3) {
          rtrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_RCT;
        }
        rtrafo     = tags->GetTagData(JPGTAG_MATRIX_RTRAFO,rtrafo);
        if (rtrafo == JPGFLAG_MATRIX_COLORTRANSFORMATION_RCT) {
          // We have the rct in the residual path, one bit more precision please.
          hdrprecision++;
        }
        
        if (riddenbits > 8 || riddenbits >= hdrprecision)
          JPG_THROW(OVERFLOW_PARAMETER,"Encoder::FindScanTypes",
                    "too many refinement scans in the residual domain, can have at most eight with the DCT disabled");
      }
      break;
    case Sequential:
    case Progressive:
    case ACSequential:
    case ACProgressive:
    case Baseline:
      hdrprecision  = tags->GetTagData(JPGTAG_RESIDUAL_PRECISION,8);
      if (hdrprecision != 8 && (hdrprecision != 12 || restype == Baseline))
        JPG_THROW(INVALID_PARAMETER,"Encoder::FindScanTypes",
                  "The residual image precision must be either 8 or 12 bits per component");
      // fall through
    default:
      if (riddenbits > 4)
        JPG_THROW(OVERFLOW_PARAMETER,"Encoder::FindScanTypes",
                  "too many refinement scans in the residual domain, can have at most four with the DCT enabled");
      hdrprecision += riddenbits;
      break;
    }
    //
    if (accoding || raccoding)
      JPG_WARN(NOT_IN_PROFILE,"Encoder::FindScanTypes",
               "arithmetic coding is not covered by the JPEG XT standard and should not be "
               "combined with JPEG XT coding features such as residual coding");
    //
    if (riddenbits >= hdrprecision)
      JPG_THROW(OVERFLOW_PARAMETER,"Encoder::FindScanTypes",
                "too many refinement scans in the residual domain");
  
  }
}
///

/// Encoder::CreateImage
// Create an image from the layout specified in the tags. See interface/parameters
// for the available tags.
class Image *Encoder::CreateImage(const struct JPG_TagItem *tags)
{
  ScanType scantype,restype;
  const struct JPG_TagItem *alphatags = (const struct JPG_TagItem *)tags->GetTagPtr(JPGTAG_ALPHA_TAGLIST);
  LONG frametype  = tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE);
  ULONG width     = tags->GetTagData(JPGTAG_IMAGE_WIDTH);
  ULONG height    = tags->GetTagData(JPGTAG_IMAGE_HEIGHT);
  ULONG depth     = tags->GetTagData(JPGTAG_IMAGE_DEPTH,3);
  ULONG precision = tags->GetTagData(JPGTAG_IMAGE_PRECISION,8);
  ULONG maxerror  = tags->GetTagData(JPGTAG_IMAGE_ERRORBOUND);
  UBYTE hiddenbits;
  UBYTE riddenbits;
  UBYTE ldrprecision;
  UBYTE hdrprecision;
  ULONG hdrquality= tags->GetTagData(JPGTAG_RESIDUAL_QUALITY,MAX_ULONG);
  UBYTE rangebits = 0;
  bool writednl   = tags->GetTagData(JPGTAG_IMAGE_WRITE_DNL)?true:false;
  ULONG restart   = tags->GetTagData(JPGTAG_IMAGE_RESTART_INTERVAL);
  ULONG levels    = tags->GetTagData(JPGTAG_IMAGE_RESOLUTIONLEVELS);
  bool residual   = (frametype & JPGFLAG_RESIDUAL_CODING)?true:false;
  bool scale      = (frametype & JPGFLAG_PYRAMIDAL)?true:false;
  const UBYTE *subx = (UBYTE *)tags->GetTagPtr(JPGTAG_IMAGE_SUBX);
  const UBYTE *suby = (UBYTE *)tags->GetTagPtr(JPGTAG_IMAGE_SUBY);
  const UBYTE *rubx = (UBYTE *)tags->GetTagPtr(JPGTAG_RESIDUAL_SUBX);
  const UBYTE *ruby = (UBYTE *)tags->GetTagPtr(JPGTAG_RESIDUAL_SUBY);

  if (m_pImage)
    JPG_THROW(OBJECT_EXISTS,"Encoder::CreateImage","the image is already initialized");
  
  if (depth > 256)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","image depth can be at most 256");
  
  if (precision < 1 || precision > 16)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","image precision must be between 1 and 16");

  if (levels > 32)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","number of resolution levels must be between 0 and 32");

  if ((frametype & 0x07) != JPGFLAG_JPEG_LS && restart > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","restart interval must be between 0 and 65535");
  
  if (maxerror > 255)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::WriteHeader","the maximum error must be between 0 and 255");

  FindScanTypes(tags,JPGFLAG_SEQUENTIAL,depth,
                scantype,restype,hiddenbits,riddenbits,ldrprecision,hdrprecision,rangebits);
  
  // Do not indicate baseline here, though it may...
  m_pImage  = new(m_pEnviron) class Image(m_pEnviron);
  m_pImage->TablesOf()->InstallDefaultTables(ldrprecision,rangebits,tags);

  m_pImage->InstallDefaultParameters(width,height,depth,
                                     ldrprecision,scantype,levels,scale,
                                     writednl,subx,suby,
                                     0,tags);

  if (residual && hdrquality > 0) { 
    class Image *residualimage;
    //
    // Create tables for the residual image.
    residualimage = m_pImage->CreateResidualImage();
    residualimage->TablesOf()->InstallDefaultTables(hdrprecision,0,tags);
 
    residualimage->InstallDefaultParameters(width,height,depth,
                                            hdrprecision - riddenbits,restype,levels,scale,
                                            writednl,rubx,ruby,
                                            JPGTAG_RESIDUAL_TAGOFFSET,tags);
  }
  
  if (alphatags) {
    ScanType ascantype,arestype;
    class Image *alphachannel = NULL;
    ULONG awidth     = alphatags->GetTagData(JPGTAG_IMAGE_WIDTH,width);
    ULONG aheight    = alphatags->GetTagData(JPGTAG_IMAGE_HEIGHT,height);
    ULONG adepth     = alphatags->GetTagData(JPGTAG_IMAGE_DEPTH,1);
    LONG aframetype  = alphatags->GetTagData(JPGTAG_IMAGE_FRAMETYPE,frametype & (~JPGFLAG_RESIDUAL_CODING));  
    LONG resflags    = tags->GetTagData(JPGTAG_RESIDUAL_FRAMETYPE,JPGFLAG_RESIDUAL);
    ULONG alevels    = alphatags->GetTagData(JPGTAG_IMAGE_RESOLUTIONLEVELS);
    bool awritednl   = alphatags->GetTagData(JPGTAG_IMAGE_WRITE_DNL,writednl)?true:false;
    bool aresidual   = (aframetype & JPGFLAG_RESIDUAL_CODING)?true:false;
    bool ascale      = (aframetype & JPGFLAG_PYRAMIDAL)?true:false; 
    bool accoding    = (frametype & JPGFLAG_ARITHMETIC)?true:false; 
    bool raccoding   = (resflags  & JPGFLAG_ARITHMETIC)?true:false;
    ULONG arestart   = alphatags->GetTagData(JPGTAG_IMAGE_RESTART_INTERVAL,restart); 
    ULONG ahdrquality= alphatags->GetTagData(JPGTAG_RESIDUAL_QUALITY,MAX_ULONG); 
    ULONG amaxerror  = alphatags->GetTagData(JPGTAG_IMAGE_ERRORBOUND);
    UBYTE ahiddenbits;
    UBYTE ariddenbits;
    UBYTE aldrprecision;
    UBYTE ahdrprecision;
    UBYTE arangebits = 0;

    //
    // First check whether the alpha channel has the same dimensions as the regular image.
    if (awidth != width || aheight != height)
      JPG_THROW(INVALID_PARAMETER,"Encoder::CreateImage",
                "the dimensions of the alpha channel must match the dimensions of the image");

    if (adepth != 1)
      JPG_THROW(INVALID_PARAMETER,"Encoder::CreateImage",
                "the alpha channel may only have a single component");

    FindScanTypes(alphatags,JPGFLAG_SEQUENTIAL,adepth,
                  ascantype,arestype,ahiddenbits,ariddenbits,aldrprecision,ahdrprecision,arangebits);
    
    if (aldrprecision < 8)
      JPG_WARN(NOT_IN_PROFILE,"Encoder::CreateImage",
               "alpha channel precisions below 8bpp are not covered by the standard");

    switch(ascantype) {
    case JPGFLAG_LOSSLESS:
    case JPGFLAG_JPEG_LS:
      JPG_WARN(NOT_IN_PROFILE,"Encoder::CreateImage",
               "JPEG LS and JPEG lossless scan types for alpha channels are not covered by the standard");
      break;
    default: // Everything else is kind of ok.
      break;
    }

    //
    // The alpha channel must be 18477-6/-7 compliant, i.e. there is no hierarchical coding
    // allowed. For experiments, let's check nevertheless.
    if (alevels > 32)
      JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","number of resolution levels must be between 0 and 32");
    
    if (ascale)
      JPG_WARN(NOT_IN_PROFILE,"Encoder::CreateImage",
               "hierarchical coding of the alpha channel is not covered by the standard");
    if ((frametype & 0x07) != JPGFLAG_JPEG_LS && arestart > MAX_UWORD)
      JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","restart interval must be between 0 and 65535");

    if (amaxerror > 255)
      JPG_THROW(OVERFLOW_PARAMETER,"Encoder::WriteHeader","the maximum error must be between 0 and 255");

    if (accoding || raccoding)
      JPG_THROW(NOT_IN_PROFILE,"Encoder::CreateImage",
                "arithmetic coding of the alpha channel is not covered by the standard");

    
    alphachannel = m_pImage->CreateAlphaChannel();
    alphachannel->TablesOf()->InstallDefaultTables(aldrprecision,arangebits,alphatags);
    alphachannel->InstallDefaultParameters(width,height,1,
                                           aldrprecision,ascantype,alevels,ascale,
                                           awritednl,NULL,NULL,0,alphatags);

    if (aresidual && ahdrquality > 0) { 
      class Image *aresidualimage;
      //
      // Create tables for the residual image.
      aresidualimage = alphachannel->CreateResidualImage();
      aresidualimage->TablesOf()->InstallDefaultTables(ahdrprecision,0,alphatags);
 
      aresidualimage->InstallDefaultParameters(width,height,1,
                                               ahdrprecision - ariddenbits,arestype,alevels,ascale,
                                               awritednl,NULL,NULL,
                                               JPGTAG_RESIDUAL_TAGOFFSET,alphatags);
    }
  }

  return m_pImage;
}
///
