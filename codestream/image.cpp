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
**
** This class represents the image as a whole, consisting either of a single
** or multiple frames.
**
** $Id: image.cpp,v 1.77 2024/06/20 13:13:36 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/checksum.hpp"
#include "marker/scantypes.hpp"
#include "codestream/image.hpp"
#include "marker/frame.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "io/checksumadapter.hpp"
#include "codestream/tables.hpp"
#include "codestream/rectanglerequest.hpp"
#include "control/bitmapctrl.hpp"
#include "control/blockctrl.hpp"
#include "control/bufferctrl.hpp"
#include "control/residualbuffer.hpp"
#include "control/hierarchicalbitmaprequester.hpp"
#include "boxes/checksumbox.hpp"
///

/// Forwards
class ByteStream;
class Component;
class Tables;
class Scan;
class BitmapCtrl;
class Frame;
///

/// Image::Image
// Create an image
Image::Image(class Environ *env)
  : JKeeper(env), m_pResidual(NULL), m_pAlphaChannel(NULL), m_pParent(NULL), 
    m_pMaster(NULL), m_pTables(NULL), m_pTableOwner(NULL), m_pDimensions(NULL), m_pSmallest(NULL), 
    m_pLast(NULL), m_pCurrent(NULL), m_pImageBuffer(NULL), 
    m_pResidualImage(NULL), m_pChecksum(NULL), 
    m_pLegacyStream(NULL), m_pAdapter(NULL), m_pBoxList(NULL),
    m_bReceivedFrameHeader(false)
{
}
///

/// Image::~Image
Image::~Image(void)
{
  class Frame *frame;

  delete m_pAlphaChannel;

  delete m_pResidual;
  delete m_pTableOwner;
  delete m_pResidualImage;
  delete m_pImageBuffer;
  delete m_pAdapter;
  delete m_pChecksum;
  delete m_pLegacyStream;
  delete m_pBoxList;

  while((frame = m_pSmallest)) {
    m_pSmallest = frame->NextOf();
    delete frame;
  }

  delete m_pDimensions;
}
///

/// Image::CreateResidualImage
// Create a residual image and install it here.
class Image *Image::CreateResidualImage(void)
{
  assert(m_pResidual == NULL && m_pParent == NULL);

  m_pResidual = new(m_pEnviron) class Image(m_pEnviron);
  m_pResidual->m_pParent = this;
  m_pResidual->m_pMaster = m_pMaster; // carry over the master for alpha channels.

  return m_pResidual;
}
///

/// Image::CreateAlphaChannel
// Create an alpha channel and install it here.
class Image *Image::CreateAlphaChannel(void)
{
  assert(m_pAlphaChannel == NULL && m_pParent == NULL && m_pMaster == NULL);

  m_pAlphaChannel = new(m_pEnviron) class Image(m_pEnviron);
  m_pAlphaChannel->m_pMaster = this;

  return m_pAlphaChannel;
}
///

/// Image::CreateResidualBuffer
// Create the buffer providing an access path to the residuals, if available.
// This works only for block based modes, line based modes do not create 
// residuals.
class BufferCtrl *Image::CreateResidualBuffer(class BufferCtrl *img)
{
  assert(img);
  
  if (m_pResidualImage == NULL) {
    class BlockBitmapRequester *req = dynamic_cast<class BlockBitmapRequester *>(img);
    
    if (req) {
      m_pResidualImage = new(m_pEnviron) class ResidualBuffer(req);
    } else {
      JPG_THROW(INVALID_PARAMETER,"Image::CreateResidualBuffer",
                "Line based coding modes do not support residual coding");

    }
  }
  return m_pResidualImage;
}
///

/// Image::TablesOf
// Create the side information for this image and return it.
class Tables *Image::TablesOf(void)
{
  if (m_pTables == NULL) {
    if (m_pParent) {
      m_pTables = m_pParent->TablesOf()->CreateResidualTables();
    } else if (m_pMaster) {
      m_pTables = m_pMaster->TablesOf()->CreateAlphaTables();
    } else {
      m_pTables = new(m_pEnviron) class Tables(m_pEnviron);
      m_pTableOwner = m_pTables;
    }
  }

  return m_pTables;
}
///

/// Image::InstallDefaultParameters
// Define default scan parameters. Returns the frame smallest frame or the only frame.
// Levels is the number of decomposition levels for the hierarchical mode. It is zero
// for the regular "flat" mode.
void Image::InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,
                                     UBYTE precision,ScanType type,UBYTE levels,
                                     bool scale,bool writednl,
                                     const UBYTE *subx,const UBYTE *suby,
                                     ULONG tagoffset,
                                     const struct JPG_TagItem *tags)
{
  ScanType followup; // follow-up frame type.
  //
  if (m_pDimensions || m_pImageBuffer)
    JPG_THROW(OBJECT_EXISTS,"Image::InstallDefaultParameters",
              "image parameters have been already established");
  //
  switch(type) {
    // All valid frame types.
  case Baseline:
  case Sequential:
    followup = DifferentialSequential;
    break;
  case Progressive:
    followup = DifferentialProgressive;
    break;
  case Lossless:
    followup = DifferentialLossless;
    break;
  case ACSequential:
    followup = ACDifferentialSequential;
    break;
  case ACProgressive:
    followup = ACDifferentialProgressive;
    break;
  case ACLossless:
    followup = ACDifferentialLossless;
    break;
  case JPEG_LS:
    followup = DifferentialLossless; // Actually, not really.
    if (scale || levels)
      JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
                "JPEG-LS does not support hierarchical coding");
    break;
  case Residual:
  case ACResidual:
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
    followup = type; // Actually, not really.
    if (scale || levels)
      JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
                "Residual coding does not support hierarchical coding");
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
              "initial frame type must be non-differential");
  }
  //
  // Build the frame for the DHP marker segment - or for the only frame here.
  m_pDimensions = new(m_pEnviron) class Frame(this,m_pTables,(levels > 0)?(Dimensions):(type));
  m_pDimensions->InstallDefaultParameters(width,height,depth,precision,
                                          writednl,subx,suby,tagoffset,tags);
  //
  // Build the image the user data goes into if we need one.
  // Note that the residual image does not require one, but the alpha
  // channel does.
  if (m_pParent == NULL)
    m_pImageBuffer = m_pDimensions->BuildImageBuffer();
  //
  // Now check whether there are any smaller levels that need to be installed.
  // This is only the case for the hierarchical mode.
  if (levels) {
#if ACCUSOFT_CODE
    class HierarchicalBitmapRequester *hr = (HierarchicalBitmapRequester *)m_pImageBuffer;
    if (scale) {
      UBYTE down = levels;
      // Several levels, scale one after another, start with the smallest.
      do {
        class Frame *frame;
        UBYTE t = --down;
        ULONG w = width;
        ULONG h = height;
        //
        // Compute the dimension of the downscaled frame.
        while(t) {
          // This makes really little sense if the image becomes degenerated.
          if (w < 2 || h < 2) {
            JPG_THROW(OVERFLOW_PARAMETER,"Image::InstallDefaultParameters",
                      "image dimensions become too small for reasonable hierarchical coding "
                      "reduce the number of levels");
          }
          w = (w + 1) >> 1; // always scaled in both dimensions in this program.
          h = (h + 1) >> 1;
          t--;
        }
        frame = new(m_pEnviron) class Frame(this,m_pTables,(down == levels - 1)?type:followup);
        if (m_pSmallest == NULL) {
          assert(m_pLast == NULL);
          m_pSmallest = frame;
        } else {
          assert(m_pLast);
          m_pLast->TagOn(frame);
        }
        m_pLast = frame;
        frame->InstallDefaultParameters(w,h,depth,precision,writednl,subx,suby,tagoffset,tags);
        if (m_pLast == m_pSmallest) {
          // The first and smallest frame, no expansion.
          hr->AddImageScale(frame,false,false);
        } else {
          hr->AddImageScale(frame,true,true);
        }
        //
        // Until the original image size is reached.
      } while(down);
    } else {
      class Frame *residual;
      // Unscaled. In this case, allow only two frames of which the first must be lossy
      // and the second lossless. This is another way of implementing a lossless process,
      // though not one that is backwards compatible to sequential.
      if (levels > 2)
        JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
                  "image parameters are not sensible, unscaled operation should use only "
                  "two frames");
      //
      // And it only makes sense if the first is not lossless.
      if (type == Lossless || type == ACLossless)
        JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
                  "image parameters are not sensible, unscaled operation should use a "
                  "lossy initial frame type");
      //
      m_pSmallest = new(m_pEnviron) class Frame(this,m_pTables,type);
      m_pLast     = m_pSmallest;
      if (levels == 1) {
        m_pSmallest->InstallDefaultParameters(width,height,depth,precision,
                                              writednl,subx,suby,tagoffset,tags);
      } else {
        m_pSmallest->InstallDefaultParameters((width + 1) >> 1,(height + 1) >> 1,depth,
                                              precision,writednl,subx,suby,tagoffset,tags);
      }
      hr->AddImageScale(m_pSmallest,false,false);
      //
      // Now create the second frame.
      switch(type) {
      case Baseline:
      case Sequential:
      case Progressive:
      case JPEG_LS:
        residual = new(m_pEnviron) class Frame(this,m_pTables,DifferentialLossless);
        break;
      case ACSequential:
      case ACProgressive:
        residual = new(m_pEnviron) class Frame(this,m_pTables,ACDifferentialLossless);
        break;
      default:
        JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
                  "invalid initial frame type, must be a non-differential type");
      }
      assert(m_pLast);
      m_pLast->TagOn(residual);
      residual->InstallDefaultParameters(width,height,depth,precision,writednl,subx,suby,tagoffset,tags);
      if (levels == 1) {
        hr->AddImageScale(residual,false,false);
      } else {
        hr->AddImageScale(residual,true,true);
      }
    }
#else
    NOREF(followup);
    JPG_THROW(NOT_IMPLEMENTED,"Image::InstallDefaultParameters",
              "Hierarchical JPEG not available in your code release, please contact Accusoft for a full version");
#endif
  } else if (m_pParent) {
    m_pDimensions->SetImageBuffer(CreateResidualBuffer(m_pParent->m_pImageBuffer));
    m_pParent->m_pDimensions->ExtendImageBuffer(m_pParent->m_pImageBuffer,m_pDimensions);
  } else {
    m_pDimensions->SetImageBuffer(m_pImageBuffer);
  }

  if (m_pImageBuffer)
    m_pImageBuffer->PrepareForEncoding();
}
///

/// Image::isDifferentialType
// Check whether a scan type is a differential scan
// and hence can only be used in a hierarchical JPEG.
bool Image::isDifferentialType(ScanType type)
{ 
  switch(type) {
  case DifferentialSequential:
  case DifferentialProgressive:
  case DifferentialLossless:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case ACDifferentialLossless:
    return true;
  default:
    return false;
  }
}
///

/// Image::MarkerToScanType
// Convert a frame marker to a scan type, return it.
ScanType Image::FrameMarkerToScanType(LONG marker) const
{  
  ScanType type = Dimensions;
  
  switch(marker) {
  case ByteStream::EOF:
    JPG_THROW(MALFORMED_STREAM,"Image::FrameMarkerToScanType","unexpected EOF while parsing the image");
    break; 
  case 0xffd9: // EOI
    JPG_THROW(MALFORMED_STREAM,"Image::FrameMarkerToScanType","unexpected EOI, expected a frame header");
    break;
  case 0xffb1:
    type = Residual;
    break;
  case 0xffb2:
    type = ResidualProgressive;
    break;
  case 0xffb3:
    type = ResidualDCT;
    break;
  case 0xffb9:
    type = ACResidual;
    break;
  case 0xffba:
    type = ACResidualProgressive;
    break;
  case 0xffbb:
    type = ACResidualDCT;
    break;
  case 0xffc0:
    type = Baseline;
    break;
  case 0xffc1:
    type = Sequential;
    break;
  case 0xffc2:
    type = Progressive;
    break;
  case 0xffc3:
    type = Lossless;
    break;
  case 0xffc9:
    type = ACSequential;
    break;
  case 0xffca:
    type = ACProgressive;
    break;
  case 0xffcb:
    type = ACLossless;
    break;
  case 0xfff7:
    type = JPEG_LS;
    break;
  case 0xffc5:
    type = DifferentialSequential;
    break;
  case 0xffc6:
    type = DifferentialProgressive;
    break;
  case 0xffc7:
    type = DifferentialLossless;
    break;
  case 0xffcd:
    type = ACDifferentialSequential;
    break;
  case 0xffce:
    type = ACDifferentialProgressive;
    break;
  case 0xffcf:
    type = ACDifferentialLossless;
    break;
  case 0xffde:
    type = Dimensions; // This is the DHP marker which delivers the image dimensions.
    break;
  default:
    JPG_THROW(MALFORMED_STREAM,"Image::FrameMarkerToScanType",
              "unexpected marker while parsing the image, decoder out of sync");
    break; 
  }

  return type;
}
///

/// Image::CreateFrameBuffer
// Create the frame, or frame hierarchy, from the given type
// This probably builds the hierarchical buffer if there is one.
// eh and ev are the horizontal and vertical expansion flags that
// indicate the frame expansion for hierarchical JPEG.
class Frame *Image::CreateFrameBuffer(class ByteStream *io,ScanType type)
{
  class Frame *frame = NULL;
  
  //
  // Check whether we expand/extend a previous frame by a differential frame or
  // start a new frame from scratch.
  if (isDifferentialType(type)) {
#if ACCUSOFT_CODE
    class Frame *prev;
    bool eh = false;
    bool ev = false;
    //
    // Get the expansion flags from the tables. The default is false,false
    // if exp is not there.
    TablesOf()->isEXPDetected(eh,ev);
    //
    // All differential types. This only works if a non-differential first frame is available.
    if (m_pSmallest == NULL)
      JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer",
                "found a differential frame outside a hierarchical image process");
    //
    // Tag onto the linked frame hierarchy.
    assert(m_pLast && m_pDimensions);
    frame      = new(m_pEnviron) class Frame(this,m_pTables,type);
    prev       = m_pLast; // The frame before us.
    m_pLast->TagOn(frame);
    m_pLast    = frame;
    frame->ParseMarker(io);
    //
    // Make a couple of consistency checks.
    if (frame->DepthOf()     != m_pDimensions->DepthOf() ||
        frame->PrecisionOf() != m_pDimensions->PrecisionOf()) {
      JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer",
                "image properties indicated in the DHP marker are incompatible with the "
                "frame properties, stream is damaged");
    }
    //
    // Check whether the frame dimensions work all right.
    if ((!eh && prev->WidthOf()  != frame->WidthOf())             ||
        ( eh && prev->WidthOf()  != (frame->WidthOf() + 1) >> 1)  ||
        (( frame->HeightOf()       != 0) &&
         ((!ev && prev->HeightOf() != frame->HeightOf())            ||
          ( ev && prev->HeightOf() != (frame->HeightOf() + 1) >> 1))
         )) {
      JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer",
                "frame dimensions are not compatible with the the expansion factors");
    }
    //
    // This should have been created before. Or rather, we should better be a non-residual frame.
    if (m_pImageBuffer == NULL)
      JPG_THROW(NOT_IMPLEMENTED,"Image::ParseFrameHeader",
                "hierarchical scan types cannot be combined with residual coding");
    //
    // Setup the image buffer to include the new resolution level.
    ((class HierarchicalBitmapRequester *)m_pImageBuffer)->AddImageScale(frame,eh,ev); 
#else
    JPG_THROW(NOT_IMPLEMENTED,"Image::CreateFrameBuffer",
              "Hierarchical JPEG not available in your code release, please contact Accusoft for a full version");
#endif    
  } else {
    // Here create a non-differential frame or start a new frame hierarchy. The DHP header and
    // the non-differential scan headers go here.
    // 
    if (m_pDimensions) {
      JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer","found a double frame header");
      return NULL; // code never goes here.
    }
    //
    // This should only exist in case the dimensions have been created.
    assert(m_pSmallest == NULL);
    //
    m_pDimensions  = new(m_pEnviron) class Frame(this,m_pTables,type);
    m_pDimensions->ParseMarker(io);
    //
    // The alpha channel requires also a separate buffer, the residual image does not.
    if (m_pParent == NULL) {
      m_pImageBuffer = m_pDimensions->BuildImageBuffer();
      m_pDimensions->SetImageBuffer(m_pImageBuffer);
    }
    //
    // If this is a hierarchical scan, create the remaining buffers.
    if (type == Dimensions) {
#if ACCUSOFT_CODE
      LONG marker;
      //
      // This is just the DHP header. Another frame header and more tables are coming.
      m_pTables->ParseTables(io,NULL,false,false);
      //
      // Now again, the next try. This must now be the real frame.
      marker = io->GetWord();
      type   = FrameMarkerToScanType(marker);
      //
      if (isDifferentialType(type))
        JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer",
                  "the first frame of a hierarchical encoded JPEG must be non-differential");
      if (type == Dimensions)
        JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer",
                  "found a double DHP marker in a hierarchical scan");
      //
      m_pSmallest = new(m_pEnviron) class Frame(this,m_pTables,type);
      m_pLast     = m_pSmallest;
      m_pSmallest->ParseMarker(io);
      if (m_pSmallest->DepthOf()     != m_pDimensions->DepthOf() ||
          m_pSmallest->PrecisionOf() != m_pDimensions->PrecisionOf()) {
        JPG_THROW(MALFORMED_STREAM,"Image::CreateFrameBuffer",
                  "image properties indicated in the DHP marker are incompatible with the "
                  "frame properties, stream is damaged");
      }
      if (m_pImageBuffer == NULL) {
        JPG_THROW(NOT_IMPLEMENTED,"Image::CreateFrameBuffer",
                  "hierarchical scan types cannot be combined with residual coding");
      } else {
        ((class HierarchicalBitmapRequester *)m_pImageBuffer)->AddImageScale(m_pSmallest,false,false);
        frame = m_pSmallest;
      }
#else
      JPG_THROW(NOT_IMPLEMENTED,"Image::CreateFrameBuffer",
                "Hierarchical JPEG not available in your code release, please contact Accusoft for a full version");
#endif
    } else {
      frame = m_pDimensions;
    }
  }
  //
  // Finally, if we have a frame buffer (non-residual), prep it for decoding.
  if (m_pImageBuffer)
    m_pImageBuffer->PrepareForDecoding();
  //
  // Finally, done with it.
  return frame;
}
///

/// Image::ParseFrameHeader
// Parse off the frame header and construct the frame, then return it.
class Frame *Image::ParseFrameHeader(class ByteStream *io)
{ 
  ScanType type = Dimensions;
  LONG marker;
  
  do {
    marker = io->PeekWord();
    switch(marker) {
    case ByteStream::EOF:
      JPG_THROW(MALFORMED_STREAM,"Image::ParseFrameHeader","unexpected EOF while parsing the image");
      break;
    case 0xffd9: // EOI
      JPG_THROW(MALFORMED_STREAM,"Image::ParseFrameHeader","unexpected EOI marker while parsing the image");
      break;
    default:
      // Collect the frame type.
      marker = io->GetWord();
      type   = FrameMarkerToScanType(marker);
      //
      // For non-differential-types: Just create the dimension/frame
      if (m_pChecksum && m_pMaster == NULL && m_pParent == NULL && TablesOf()->ChecksumTables()) {
        UBYTE tmp[2];
        class ChecksumAdapter csa(io,m_pChecksum,false);
        // The SOF_x requires checksumming, starting with the marker itself.
        // Fiddle the marker into the checksum.
        tmp[0] = marker >> 8;
        tmp[1] = marker;
        m_pChecksum->Update(tmp,sizeof(tmp));
        return CreateFrameBuffer(&csa,type);
      } else {
        return CreateFrameBuffer(io,type);
      }
      break; // code never goes here.
    }
  } while(true);

  // The code never goes here.
  return NULL;
}
///

/// Image::StartParseFrame
// Start parsing a single frame. Returns NULL if there are no more frames in this
// image.
class Frame *Image::StartParseFrame(class ByteStream *io)
{
  //
  // This should only be called from the main image.
  assert(m_pParent == NULL && m_pMaster == NULL);
  //
  // Check whether we have the frame header. Residual and alpha
  // already parse that off as part of ParseTrailer().
  if (m_bReceivedFrameHeader == false) {
    assert(m_pTables);
    m_pCurrent = ParseFrameHeader(io);
    if (m_pCurrent) {
      // Create the checksum if it is needed.
      CreateChecksumWhenNeeded(m_pChecksum);
      //
      // Is now there.
      m_bReceivedFrameHeader = true;
    }
  }
  //
  // Otherwise, the frame header has already been parsed off and need not to be
  // looked at here again.
  return m_pCurrent;
}
///

/// Image::FindFirstWriteFrame
// Select the first frame to write to, return it.
class Frame *Image::FindFirstWriteFrame(void) const
{
  if (m_pAlphaChannel) {
    // First write alpha (or rather, recursively, alpha residual, followed by alpha)
    return m_pAlphaChannel->FindFirstWriteFrame();
  } else if (m_pResidual) {
    // Then write the residual
    return m_pResidual->FindFirstWriteFrame();
  } else if (m_pSmallest) {
    // If we are hierarchical, start there.
    return m_pSmallest;
  } else {
    // Otherwise, start with the regular image.
    assert(m_pDimensions);
    return m_pDimensions;
  }
}
///

/// Image::WriteImageAndFrameHeader
// Write the image header belonging to the given frame to the given stream.
void Image::WriteImageAndFrameHeader(class Frame *frame,class ByteStream *target) const
{
  // Write the image header for residual alpha.
  // For legacy reasons, the SOI and the tables are written ahead for the main
  // image and are not written here. This allows legacy applications to inject
  // custom data into the frame header.
  if ((m_pParent || m_pMaster) && (m_pSmallest == NULL || m_pSmallest == frame)) {
    target->PutWord(0xffd8); // SOI
    frame->TablesOf()->WriteTables(target);
  }
  //
  frame->ResetToFirstScan();
  //
  // If it is hierarchical, write the dimensions into the DHP marker now.
  if (m_pSmallest) {
    // Depending on which resolution we are working on, write the DHP marker or the exp marker.
    if (frame == m_pSmallest) {
      // We start the smallest frame. Write the DHP marker with the real frame
      // definitions, then write the frame itself.
      target->PutWord(0xffde); // DHP marker.
      m_pDimensions->WriteMarker(target);
    } else {
      class HierarchicalBitmapRequester *hr = (class HierarchicalBitmapRequester *)m_pImageBuffer;
      bool hexp,vexp;
      UBYTE v = 0;
      //
      if (hr) {
        // Otherwise, we need to generate an EXP marker here.
        // Get the exp marker and transfer the differential data into the current frame.
        hr->GenerateDifferentialImage(m_pCurrent,hexp,vexp);
        //
        // Now write the EXP marker.
        target->PutWord(0xffdf);
        target->PutWord(0x0003);
        if (hexp) v |= 0x10;
        if (vexp) v |= 0x01;
        target->Put(v);
      } else {
        JPG_THROW(NOT_IMPLEMENTED,"Image::WriteImageAndFrameHeader",
                  "cannot use hierarchical encoding in the residual domain");
      }
    }
  }
  //
  // Done with all the preparations for hierarchical.
  // Now write the frame header.
  frame->WriteFrameType(target);
  frame->WriteMarker(target);
}
///

/// Image::OutputBufferOf
// Given an image, get the target buffer where the data goes when writing, or the
// regular stream passed in in case data can be written directly.
class DataBox *Image::OutputBufferOf(void)
{
  if (m_pParent) {
    // This is a residual stream. Get the residual stream buffer from the tables.
    // this is also valid for alpha.
    return TablesOf()->ResidualDataOf();
  } else if (m_pMaster) {
    // This is an alpha stream. This goes into the alpha buffer of the master table.
    return m_pMaster->TablesOf()->AlphaDataOf();
  } else {
    // This is the base image. It does not have a buffer for its data, but writes
    // directly to the target.
    return NULL;
  }
}
///

/// Image::FlushSideChannel
// Complete the side channel, i.e. complete it and flush it out.
void Image::FlushSideChannel(class ByteStream *target)
{
  class DataBox *output = OutputBufferOf();
  class ByteStream *io;
  //
  assert(output);
  //
  // Get the buffer where the information should go to.
  io  = output->EncoderBufferOf();
  // First, write the trailer.
  assert(io != target); // the trailer should hopefully go into the buffer.
  WriteTrailer(io);
  //
  // Then flush the stuff.
  // The data boxes are unique, hence set the enumerator to one.
  output->Flush(target,1);
}
///

/// Image::StartWriteFrame
// Start writing a single scan. Scan parameters must have been installed before.
class Frame *Image::StartWriteFrame(class ByteStream *io)
{
  //
  // This should not be called from the residual or alpha.
  assert(m_pParent == NULL && m_pMaster == NULL);
  //
  if (m_pCurrent == NULL) {
    // First, find the current frame to write to, then cover all the work
    // required to be done to open a write stream for this frame.
    m_pCurrent = FindFirstWriteFrame();
  }
  //
  // Write the frame header into the encoder output buffer if we have one. Only the 
  // base image has this set to NULL.
  {
    class DataBox *container = m_pCurrent->ImageOf()->OutputBufferOf();
    //
    // Are we in a side channel?
    if (container) {
      class ByteStream *target = container->EncoderBufferOf();
      m_pCurrent->ImageOf()->WriteImageAndFrameHeader(m_pCurrent,target);
    } else {
      assert(m_pDimensions);
      //
      // We are in the regular image stream. Check whether we need the checksum information.
      // This happens whenever we create JPEG XT files.
      if (m_pTables->ResidualSpecsOf() || m_pTables->AlphaSpecsOf()) {
        if (m_pChecksum == NULL) {
          assert(m_pLegacyStream == NULL); 
          m_pChecksum     = new(m_pEnviron) class Checksum();
          m_pLegacyStream = new(m_pEnviron) class MemoryStream(m_pEnviron,MAX_UWORD);
        }
      }
      //
      // Write now either into the memory buffer (for checksumming) or into the real IO
      {
        // Do we checksum the tables as well?
        if (m_pLegacyStream && TablesOf()->ChecksumTables()) {
          class ChecksumAdapter adapter(io,m_pChecksum,true);
          // Also create the adaptor for the main stream.
          m_pAdapter = new(m_pEnviron) class ChecksumAdapter(m_pLegacyStream,m_pChecksum,true);
          // Also generate the image and frame header now.
          // Its data is included in the checksum.
          WriteImageAndFrameHeader(m_pCurrent,&adapter);
          adapter.Close();
        } else {
          // Also generate the image and frame header now. They are not checksum'd here
          // and data goes directly to disk.
          WriteImageAndFrameHeader(m_pCurrent,io);
        }
        //
        // Finally, write out all residuals/side channels
        // If there are still side information channels pending pending, complete it now.
        // This is only required if there is really something to flush, i.e. when we
        // write the smallest dimension of a hierarchical, or a flat image.
        if (m_pSmallest == NULL || m_pCurrent == m_pSmallest) {
          if (m_pAlphaChannel && m_pAlphaChannel->m_pResidual)
            m_pAlphaChannel->m_pResidual->FlushSideChannel(io);
          if (m_pAlphaChannel)
            m_pAlphaChannel->FlushSideChannel(io);
          if (m_pResidual)
            m_pResidual->FlushSideChannel(io);
        }
      }
    }
  }
  
  return m_pCurrent;
}
///

/// Image::StartMeasureFrame
// Instead of writing, just collect statistics for the huffman coder.
class Frame *Image::StartMeasureFrame(void)
{ 
  class Image *current;
  //
  if (m_pCurrent == NULL) {
    // First, find the current frame to write to, then cover all the work
    // required to be done to open a write stream for this frame.
    m_pCurrent = FindFirstWriteFrame();
  }
  //
  // Start at the first scan of this frame.
  m_pCurrent->ResetToFirstScan();
  //
  // Generate frames for differential images.
  current = m_pCurrent->ImageOf();
  assert(current->m_pDimensions);
  //
  // Check whether this is a hierarchical scan. If so, we must first
  // generate the next higher resolution level if we are not at the lowest
  // level.
  if (current->m_pSmallest) {
    if (m_pCurrent != current->m_pSmallest) { 
      class HierarchicalBitmapRequester *hr = (class HierarchicalBitmapRequester *)m_pImageBuffer;
      if (hr) {
        bool hexp,vexp;
        //
        // Get the exp marker and transfer the differential data into the current frame.
        hr->GenerateDifferentialImage(m_pCurrent,hexp,vexp);
      } else {
        JPG_THROW(NOT_IMPLEMENTED,"Image::StartMeasureFrame",
                  "cannot combine hierarchical coding and residual coding");
      }
    }
  }
  
  return m_pCurrent;
}
///

/// Image::NextFrame
// Advance to the next frame, deliver it or NULL if there is no next frame.
class Frame *Image::NextFrame(void)
{
  class Image *current;
  
  if (m_pCurrent == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Image::NextFrame","no frame iteration has been started yet");

  //
  // This must be called from the master image.
  assert(m_pMaster == NULL && m_pParent == NULL);

  current = m_pCurrent->ImageOf();
  //
  // Check whether we have hierarchical frames here. If so, continue in this direction first.
  if (current->m_pSmallest) {
    if (m_pCurrent->NextOf()) {
      m_pCurrent = m_pCurrent->NextOf();
      return m_pCurrent;
    }
  }
  //
  // Not hierarchical, or hierarchy done. Continue with the regular images.
  if (current->m_pMaster && current->m_pParent) {
    // Advance to alpha channel, legacy image. Must have a parent, so fetch that.
    // This is the alpha channel.
    current    = current->m_pParent;
  } else if (current->m_pMaster) {
    // Switch from alpha channel to residuals or main.
    if (current->m_pMaster->m_pResidual) {
      current = current->m_pMaster->m_pResidual;
    } else {
      current = current->m_pMaster;
    }
  } else if (current->m_pParent) {
    // Residual. Ok, switch to main.
    current = current->m_pParent;
  } else {
    // We were already main. Give up.
    return NULL;
  }
  
  //
  // Hopefully, received the next base image.
  assert(current);
  //
  // Now start iteration in the hierarchical direction if we have that.
  if (current->m_pSmallest) {
    m_pCurrent = current->m_pSmallest;
  } else {
    m_pCurrent = current->m_pDimensions;
  }
  assert(m_pCurrent);

  return m_pCurrent;
}
///

/// Image::InputStreamOf
// Return the input stream data should come from. This might be the
// residual stream if the current frame is the residual frame.
// It is otherwise the unmodified input.
class ByteStream *Image::InputStreamOf(class ByteStream *legacy) const
{
  if (m_pCurrent) {
    class DataBox *box = m_pCurrent->ImageOf()->OutputBufferOf();
    if (box) {
      // Ok, this image is part of a side channel.
      class ByteStream *in = box->DecoderBufferOf();
      assert(in);
      //
      // In case we reached here the EOF, do not try to continue parsing and
      // searching for an EOI because it might not be there. Instead, just
      // abort and continue with the EOI at the legacy stream.
      if (in->PeekWord() == ByteStream::EOF)
        return legacy;
      return in;
    }
  }
  return legacy;
}
///

/// Image::ChecksumOf
// Return the checksum so far if we need to keep one.
class Checksum *Image::ChecksumOf(void) const
{ 
  //
  // Only to be called from the main image.
  assert(m_pParent == NULL && m_pMaster == NULL);
  //
  // Everything that goes into a residual buffer is not checksummed.
  if (m_pCurrent && m_pCurrent->ImageOf()->OutputBufferOf())
    return NULL;
  return m_pChecksum;
}
///

/// Image::CreateChecksumWhenNeeded
// Create a checksum when this is the main image, and a checksum
// in the tables is needed.
class Checksum *Image::CreateChecksumWhenNeeded(class Checksum *chk)
{
  // This only applies to the main image.
  if (chk == NULL && m_pParent == NULL && m_pMaster == NULL) {
    if (m_pTables->ResidualSpecsOf() || m_pTables->AlphaSpecsOf()) {
      if (m_pChecksum == NULL)
        chk = m_pChecksum = new(m_pEnviron) class Checksum();
    }
  }
  return chk;
}
///

/// Image::OutputStreamOf
// Return the output stream data should go to. This might be the
// residual stream if the current frame is the residual frame.
// It is otherwise the unmodified input.
class ByteStream *Image::OutputStreamOf(class ByteStream *legacy) const
{ 
  class DataBox *box;
  class ByteStream *out;
  //
  // Only to be called from the main image.
  assert(m_pParent == NULL && m_pMaster == NULL);
  assert(m_pCurrent);
  //
  // Does the current stream go into a side channel?
  box = m_pCurrent->ImageOf()->OutputBufferOf();
  if (box) {
    // Yup, does, deliver the side channel.
    out = box->EncoderBufferOf();
    assert(box);
  } else if (m_pAdapter) {
    // Here use the RAM-disk with checksum.
    out = m_pAdapter;
  } else if (m_pLegacyStream) {
    // Here use the RAM-disk without checksum.
    out = m_pLegacyStream;
  } else {
    out = legacy;
  }
  assert(out);
  return out;
}
///

/// Image::ResetToFirstFrame
// Reset the scan to the first in the image
void Image::ResetToFirstFrame(void)
{
  m_pCurrent = NULL;
  
  if (m_pSmallest) {
    for(class Frame *frame = m_pSmallest;frame;frame = frame->NextOf()) {
      frame->ResetToFirstScan();
    }
  }
  m_pDimensions->ResetToFirstScan();

  if (m_pAlphaChannel)
    m_pAlphaChannel->ResetToFirstFrame();
  
  if (m_pResidual)
    m_pResidual->ResetToFirstFrame();
}
///

/// Image::ReconstructRegion
// Control interface - direct forwarding to the bitmap control
// Reconstruct a rectangle of coefficients.
void Image::ReconstructRegion(class BitMapHook *bmh,const struct RectangleRequest *rr)
{
  struct RectangleRequest rralpha = *rr;
  bool doalpha = m_pAlphaChannel && rr->rr_bIncludeAlpha;
  RectAngle<LONG> region;
  
  
  if (m_pDimensions == NULL || m_pImageBuffer == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Image::ReconstructRegion","no image loaded that could be reconstructed");
  if (doalpha) {
    if (m_pAlphaChannel->m_pDimensions == NULL || m_pAlphaChannel->m_pImageBuffer == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Image::ReconstructRegion","alpha channel not loaded, or not yet available");
  }
  
  region = rr->rr_Request;

  // Fiddle a request for the alpha channel if we have that.
  if (doalpha) {
    rralpha.rr_usFirstComponent = 0; // There is only one component for alpha
    rralpha.rr_usLastComponent  = 0;
  }
  
  m_pImageBuffer->CropDecodingRegion(region,rr);
  if (doalpha)
    m_pAlphaChannel->m_pImageBuffer->CropDecodingRegion(region,&rralpha);
  m_pImageBuffer->RequestUserDataForDecoding(bmh,region,rr,false);
  if (doalpha)
    m_pAlphaChannel->m_pImageBuffer->RequestUserDataForDecoding(bmh,region,&rralpha,true);
  if (!region.IsEmpty()) {
    m_pImageBuffer->ReconstructRegion(region,rr);
    if (doalpha)
      m_pAlphaChannel->m_pImageBuffer->ReconstructRegion(region,&rralpha);
  }
  if (doalpha)
    m_pAlphaChannel->m_pImageBuffer->ReleaseUserDataFromDecoding(bmh,&rralpha,true);
  m_pImageBuffer->ReleaseUserDataFromDecoding(bmh,rr,false);
}
///

/// Image::EncodeRegion
// Encode the next region in the scan from the user bitmap. The requested region
// is indicated in the tags going to the user bitmap hook.
void Image::EncodeRegion(class BitMapHook *bmh,const struct RectangleRequest *rr)
{
  struct RectangleRequest rralpha = *rr;
  bool doalpha = m_pAlphaChannel && rr->rr_bIncludeAlpha;
  RectAngle<LONG> region;

  if (m_pImageBuffer == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Image::EncodeRegion","no image constructed into which data could be loaded");
  if (doalpha) {
    if (m_pAlphaChannel->m_pImageBuffer == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Image::ReconstructRegion","alpha channel not loaded, or not yet available");
  }

  region.ra_MinX = 0;
  region.ra_MinY = MAX_LONG;
  region.ra_MaxX = MAX_LONG;
  region.ra_MaxY = MAX_LONG; 

  // Fiddle a request for the alpha channel if we have that.
  if (doalpha) {
    rralpha.rr_usFirstComponent = 0; // There is only one component for alpha
    rralpha.rr_usLastComponent  = 0;
  }

  m_pImageBuffer->CropEncodingRegion(region,rr);
  if (doalpha)
    m_pAlphaChannel->m_pImageBuffer->CropEncodingRegion(region,&rralpha);
  m_pImageBuffer->RequestUserDataForEncoding(bmh,region,false);
  if (doalpha)
    m_pAlphaChannel->m_pImageBuffer->RequestUserDataForEncoding(bmh,region,true);
  if (!region.IsEmpty()) {
    m_pImageBuffer->EncodeRegion(region);
    if (doalpha)
      m_pAlphaChannel->m_pImageBuffer->EncodeRegion(region);
  }
  if (doalpha)
    m_pAlphaChannel->m_pImageBuffer->ReleaseUserDataFromEncoding(bmh,region,true);
  m_pImageBuffer->ReleaseUserDataFromEncoding(bmh,region,false);
}
///

/// Image::BufferedLines
// Return the number of lines available for reconstruction from this scan.
ULONG Image::BufferedLines(const struct RectangleRequest *rr) const
{  
  if (m_pDimensions == NULL)
    return 0; // No image, no lines.

  if (m_pImageBuffer == NULL)
    return 0;

  return m_pImageBuffer->BufferedLines(rr);
}
///

/// Image::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder.
bool Image::isNextMCULineReady(void) const
{  
  if (m_pDimensions == NULL)
    return false; // no image, nothing ready.

  if (m_pImageBuffer == NULL)
    return false;

  return m_pImageBuffer->isNextMCULineReady();
}
///

/// Image::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool Image::isImageComplete(void) const
{ 
  if (m_pDimensions == NULL)
    return false; // No image, nothing complete.

  if (m_pImageBuffer == NULL)
    return false;

  if (m_pAlphaChannel && m_pAlphaChannel->isImageComplete() == false)
    return false;
  
  return m_pImageBuffer->isImageComplete();
}
///

/// Image::WriteHeader
// Write the header and header tables up to the SOS marker.
void Image::WriteHeader(class ByteStream *io) const
{
  io->PutWord(0xffd8); // The SOI
  m_pTables->WriteTables(io);
}
///

/// Image::WriteTrailer
// Write the trailing data of the trailer, namely the EOI
void Image::WriteTrailer(class ByteStream *io)
{
  if (m_pLegacyStream) {
    class ChecksumBox *chkbox;
    class MemoryStream readback(m_pEnviron,m_pLegacyStream,JPGFLAG_OFFSET_BEGINNING);
    // Is the legacy stream still buffered? If so,
    // create now the checksum box (FIXME!) and write the legacy data out.
    if (TablesOf()->ChecksumTables()) {
      assert(m_pAdapter);
      m_pAdapter->Close();
      delete m_pAdapter;m_pAdapter = NULL;
    }
    assert(m_pAdapter == NULL);
    //
    // Create now the checksum box.
    assert(m_pBoxList == NULL);
    chkbox = new(m_pEnviron) class ChecksumBox(m_pEnviron,m_pBoxList);
    assert(m_pBoxList == chkbox);
    //
    // Now set the checksum and define the value of the checksum box.
    chkbox->InstallChecksum(m_pChecksum);
    //
    // And write it out to the file.
    Box::WriteBoxMarkers(m_pBoxList,io);
    //
    // Finally, write everything from the SOF on to the stream.
    readback.Push(io,m_pLegacyStream->BufferedBytes());
  }
  
  io->PutWord(0xffd9);
}
///

/// Image::ParseResidualStream
// Parse off the residual stream. Returns the residual frame if it exists, or NULL
// in case it does not or there are no more scans in the file.
class Frame *Image::ParseResidualStream(class DataBox *box)
{ 
  class ByteStream *sio = box->DecoderBufferOf();
  class Frame *frame    = NULL;
  //
  // Residual image must be there.
  if (m_pDimensions == NULL)
    JPG_THROW(MALFORMED_STREAM,"Image::ParseResidualStream",
              "No image found in legacy codestream, table-definitions only do not qualify a valid JPEG image");
  //
  if (m_pResidual == NULL) {
    // Residual is not yet parsed off.
    m_pResidual = CreateResidualImage();
    //
    if (sio->GetWord() != 0xffd8) {
      JPG_THROW(MALFORMED_STREAM,"Image::ParseResidualStream",
                "Residual codestream is invalid, SOI marker missing.");
    }
    // Start parsing its header.
    // And parse the tables following the SOI.
    // This is the residual stream. It is not checksummed.
    m_pResidual->TablesOf()->ParseTables(sio,NULL,false,false);
    //
    // And start the parsing of the frame header so
    // we can check its dimensions.
    frame = m_pResidual->ParseFrameHeader(sio);
    //
    if (frame) {
      class BufferCtrl *residual;
      //
      // Check for consistency.
      if (WidthOf()  != m_pResidual->WidthOf()  ||
          HeightOf() != m_pResidual->HeightOf()) {
        JPG_THROW(MALFORMED_STREAM,"Image::ParseResidualStream",
                  "Malformed stream - residual image dimensions do not match the dimensions of the legacy image");
      }
      //
      if (DepthOf()  != m_pResidual->DepthOf()) {
        JPG_THROW(MALFORMED_STREAM,"Image::ParseResidualStream",
                  "Malformed stream - number of components differ between residual and legacy image");
      }
      //
      assert(m_pDimensions);
      assert(m_pImageBuffer);
      //
      // Build the block helper that merges the two images.
      m_pDimensions->ExtendImageBuffer(m_pImageBuffer,frame);
      residual = CreateResidualBuffer(m_pImageBuffer);
      frame->SetImageBuffer(residual);
      residual->PrepareForDecoding();
    }
    // 
    // Keep the EOI in the buffer so we come here again.
    return frame;
  } else {
    LONG marker = sio->PeekWord();
    //
    // If this is EOI or also EOF (allowed here!) then do not even
    // check. EOI can be missing if SOI is missing.
    if (marker != 0xffd9 && marker != ByteStream::EOF) {
      // Residual did already exist, ok - parse the residual now
      // and forward the request to the residual. Using a different
      // stream, though.
      if (m_pResidual->ParseTrailer(sio))
        return m_pCurrent;
    }
    // No more scans in the residual.
  }
  
  return NULL;
}
///

/// Image::ParseAlphaChannel
// Parse off the alpha channel. Returns the alpha frame if it is exists, or NULL
// in case it does not or there are no more scans in this frame.
class Frame *Image::ParseAlphaChannel(class DataBox *box)
{
  class ByteStream *sio = box->DecoderBufferOf();
  class Frame *frame    = NULL;
  //
  // Residual image must be there.
  if (m_pDimensions == NULL)
    JPG_THROW(MALFORMED_STREAM,"Image::ParseAlphaChannel",
              "No image found in legacy codestream, table-definitions only do not qualify a valid JPEG image");
  //
  if (m_pAlphaChannel == NULL) {
    //
    // Alpha channel main is not yet parsed off, so do that now.
    m_pAlphaChannel = CreateAlphaChannel();
    //
    if (sio->GetWord() != 0xffd8) {
      JPG_THROW(MALFORMED_STREAM,"Image::ParseAlphaChannel",
                "Alpha channel codestream is invalid, SOI marker missing.");
    }
    // Start parsing its header.
    // And parse the tables following the SOI.
    // This is the alpha stream. It is not checksummed.
    m_pAlphaChannel->TablesOf()->ParseTables(sio,NULL,false,false);
    //
    // And start the parsing of the frame header so
    // we can check its dimensions.
    frame = m_pAlphaChannel->ParseFrameHeader(sio);
    //
    if (frame) {
      // Check for consistency.
      if (WidthOf()  != m_pAlphaChannel->WidthOf()  ||
          HeightOf() != m_pAlphaChannel->HeightOf()) {
        JPG_THROW(MALFORMED_STREAM,"Image::ParseAlphaChannel",
                  "Malformed stream - residual image dimensions do not match the dimensions of the legacy image");
      }
      //
      if (m_pAlphaChannel->DepthOf() != 1) {
        JPG_THROW(MALFORMED_STREAM,"Image::ParseAlphaChannel",
                  "Malformed stream - the alpha channel may only consist of a single component");
      }
      //
      // Build the block helper that merges the two images.
      assert(m_pAlphaChannel);
    }
    // 
    // Keep the EOI in the buffer so we come here again.
    return frame;
  } else {
    LONG marker = sio->PeekWord();
    //
    // If this is EOI or also EOF (allowed here!) then do not even
    // check. EOI can be missing if SOI is missing.
    if (marker != 0xffd9 && marker != ByteStream::EOF) {
      // Residual did already exist, ok - parse the residual now
      // and forward the request to the residual. Using a different
      // stream, though.
      if (m_pAlphaChannel->ParseTrailer(sio))
        return m_pCurrent;
    }
  }
  // No more scans in the alpha... try residual alpha.
  return NULL;
}
///

/// Image::ParseTrailer
// Parse off the EOI marker at the end of the image. Return false
// if there are no more frames in the file, true otherwise.
bool Image::ParseTrailer(class ByteStream *io)
{
  //
  // First, note that the frame header is required again now.
  m_bReceivedFrameHeader = false;
  //
  do {
    LONG marker = io->PeekWord();
    
    // The EOI is not checksummed.
    if (marker == 0xffd9) { //EOI 
      class DataBox *box = m_pTables->ResidualDataOf();
      //
      // Is there a residual scan left that hasn't been
      // parsed off yet?
      if (box) {
        // If there are more scans in the residual, continue there.
        if ((m_pCurrent = ParseResidualStream(box))) {
          // This has been parsed off from the boxed stream, hence, do not
          // require it again.
          m_bReceivedFrameHeader = true;
          return true;
        }
      }
      //
      // Continue the adventure with the alpha channel. This also needs
      // to be parsed off if it exists.
      box = m_pTables->AlphaDataOf();
      if (box) {
        if ((m_pCurrent = ParseAlphaChannel(box))) {
          // This has been parsed off from the boxed stream, hence, do not
          // require it again.
          m_bReceivedFrameHeader = true;
          return true;
        }
        //
        // Now check whether we have residual alpha.
        if (m_pAlphaChannel) {
          assert(m_pAlphaChannel->m_pTables);
          class DataBox *box = m_pAlphaChannel->m_pTables->ResidualDataOf();
          //
          // Is there a residual scan left that hasn't been
          // parsed off yet?
          if (box) {
            if ((m_pCurrent = m_pAlphaChannel->ParseResidualStream(box))) {
              // This has been parsed off from the boxed stream, hence, do not
              // require it again.
              m_bReceivedFrameHeader = true;
              return true;
            }
          }
        }
      }
      // No more scans anywhere. Get rid of the final word.
      //
      io->GetWord();
      return false;
    } else if (marker == 0xffff) { // A filler 0xff byte
      io->Get(); // Skip the filler and try again.
    } else if (marker == ByteStream::EOF) {
      JPG_WARN(MALFORMED_STREAM,"Image::ParseTrailer",
             "expecting an EOI marker at the end of the stream");
      return false;
    } else if (marker < 0xff00) {
      JPG_WARN(MALFORMED_STREAM,"Image::ParseTrailer",
               "expecting a marker or marker segment - stream is out of sync");
      // Advance to the next marker.
      io->Get();
      do {
        marker = io->Get();
      } while(marker != 0xff && marker != ByteStream::EOF);
      //
      if (marker == ByteStream::EOF) {
        JPG_WARN(UNEXPECTED_EOF,"Image::ParseTrailer",
                 "run into an EOF while scanning for the next marker");
        return false;
      }
      io->LastUnDo();
      // Continue parsing, check what the next marker might be.
    } else {
      return true;
    }
  } while(true);
  
  return true;
}
///

/// Image::StartOptimizeFrame
// Start an optimization scan that can be added upfront the measurement to 
// improve the R/D performance.
class Frame *Image::StartOptimizeFrame(void)
{ 
  // Actually, for the time being, this is identical to
  // StartMeasureFrame
  return StartMeasureFrame();
}
///

