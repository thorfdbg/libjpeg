/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** $Id: image.cpp,v 1.27 2012-09-09 19:57:02 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
#include "codestream/image.hpp"
#include "marker/frame.hpp"
#include "io/bytestream.hpp"
#include "codestream/tables.hpp"
#include "control/bitmapctrl.hpp"
#include "control/hierarchicalbitmaprequester.hpp"
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
Image::Image(class Tables *t)
  : JKeeper(t->EnvironOf()), m_pTables(t), 
    m_pDimensions(NULL), m_pSmallest(NULL), m_pLast(NULL), m_pCurrent(NULL), m_pImage(NULL)
{
}
///

/// Image::~Image
Image::~Image(void)
{
  class Frame *frame;
  delete m_pImage;

  while((frame = m_pSmallest)) {
    m_pSmallest = frame->NextOf();
    delete frame;
  }

  delete m_pDimensions;
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
				     const struct JPG_TagItem *tags)
{
  ScanType followup; // follow-up frame type.
  //
  if (m_pDimensions || m_pImage)
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
  case ACLossless:
    followup = ACDifferentialLossless;
    break;
  case JPEG_LS:
    followup = DifferentialLossless; // Actually, not really.
    if (scale)
      JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
		"JPEG-LS does not support hierarchical coding");
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
	      "initial frame type must be non-differential");
  }
  //
  // Build the frame for the DHP marker segment - or for the only frame here.
  m_pDimensions = new(m_pEnviron) class Frame(m_pTables,(levels > 0)?(Dimensions):(type));
  m_pDimensions->InstallDefaultParameters(width,height,depth,precision,
					  writednl,subx,suby,tags);
  // Build the image the user data goes into.
  m_pImage      = m_pDimensions->BuildImage();
  //
  // Now check whether there are any smaller levels that need to be installed.
  // This is only the case for the hierarchical mode.
  if (levels) {
    class HierarchicalBitmapRequester *hr = (HierarchicalBitmapRequester *)m_pImage;
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
		      "image dimensions become too small for resonable hierarchical coding "
		      "reduce the number of levels");
	  }
	  w = (w + 1) >> 1; // always scaled in both dimensions in this program.
	  h = (h + 1) >> 1;
	  t--;
	}
	frame = new(m_pEnviron) class Frame(m_pTables,(down == levels - 1)?type:followup);
	if (m_pSmallest == NULL) {
	  assert(m_pLast == NULL);
	  m_pSmallest = frame;
	} else {
	  assert(m_pLast);
	  m_pLast->TagOn(frame);
	}
	m_pLast = frame;
	frame->InstallDefaultParameters(w,h,depth,precision,writednl,subx,suby,tags);
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
      m_pSmallest = new(m_pEnviron) class Frame(m_pTables,type);
      m_pLast     = m_pSmallest;
      if (levels == 1) {
	m_pSmallest->InstallDefaultParameters(width,height,depth,precision,
					      writednl,subx,suby,tags);
      } else {
	m_pSmallest->InstallDefaultParameters((width + 1) >> 1,(height + 1) >> 1,depth,
					      precision,writednl,subx,suby,tags);
      }
      hr->AddImageScale(m_pSmallest,false,false);
      //
      // Now create the second frame.
      switch(type) {
      case Baseline:
      case Sequential:
      case Progressive:
      case JPEG_LS:
	residual = new(m_pEnviron) class Frame(m_pTables,DifferentialLossless);
	break;
      case ACSequential:
      case ACProgressive:
	residual = new(m_pEnviron) class Frame(m_pTables,ACDifferentialLossless);
	break;
      default:
	JPG_THROW(INVALID_PARAMETER,"Image::InstallDefaultParameters",
		  "invalid initial frame type, must be a non-differential type");
      }
      assert(m_pLast);
      m_pLast->TagOn(residual);
      residual->InstallDefaultParameters(width,height,depth,precision,writednl,subx,suby,tags);
      if (levels == 1) {
	hr->AddImageScale(residual,false,false);
      } else {
	hr->AddImageScale(residual,true,true);
      }
    }
  } else {
    m_pDimensions->SetImage(m_pImage);
  }
  m_pImage->PrepareForEncoding();
}
///
/// Image::StartParseFrame
// Start parsing a single frame. Returns NULL if there are no more frames in this
// image.
class Frame *Image::StartParseFrame(class ByteStream *io)
{
  ScanType type = Dimensions;
  LONG len,marker;
  bool eh = false;
  bool ev = false;

  do {
    marker = io->GetWord();
    switch(marker) {
    case ByteStream::EOF:
      JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","unexpected EOF while parsing the image");
      break;
    case 0xffd9: // EOI
      return NULL;
    case 0xffde: // DHP, this is a hierachical process.
      if (m_pDimensions) {
	if (m_pSmallest) 
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","found a double DHP marker");
	else
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","found a DHP marker within a non-hierarchical image");
	return NULL; // code never goes here.
      }
      m_pDimensions = new(m_pEnviron) class Frame(m_pTables,Dimensions);
      m_pDimensions->ParseMarker(io);
      m_pImage      = m_pDimensions->BuildImage();
      m_pTables->ParseTables(io);
      // The next following must be a frame, a non-differential, though.
      marker = io->GetWord();
      switch(marker) {
      case ByteStream::EOF:
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","unexpected EOF while parsing the image");
	break; 
      case 0xffd9: // EOI
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","unexpected EOI - expected a non-differential frame header");
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
      default:
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		  "unexpected marker while parsing the image, decoder out of sync");
	break; 
      }
      assert(m_pSmallest == NULL);
      assert(m_pImage);
      m_pSmallest = new(m_pEnviron) class Frame(m_pTables,type);
      m_pLast     = m_pSmallest;
      m_pCurrent  = m_pSmallest;
      m_pSmallest->ParseMarker(io);
      if (m_pSmallest->DepthOf()     != m_pDimensions->DepthOf() ||
	  m_pSmallest->PrecisionOf() != m_pDimensions->PrecisionOf()) {
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		  "image properties indicated in the DHP marker are incompatible with the "
		  "frame properties, stream is damaged");
      }
      ((class HierarchicalBitmapRequester *)m_pImage)->AddImageScale(m_pSmallest,false,false);
      m_pImage->PrepareForDecoding();
      return m_pSmallest;
    case 0xffc0:
    case 0xffc1:
    case 0xffc2:
    case 0xffc3:
    case 0xffc9:
    case 0xffca:
    case 0xffcb:
    case 0xfff7:
      // All non-differential types. 
      // If this start of an image, all is well - this should then be the only frame.
      // Otherwise, it is illegal.
      if (m_pDimensions) {
	if (m_pSmallest)
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		    "found a non-differential frame as second frame in a hierarchical image");
	else
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		    "found a second frame in a non-hierarchical image");
	return NULL; // Code never goes here.
      }
      switch(marker) {
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
      }
      assert(m_pDimensions == NULL);
      m_pDimensions = new(m_pEnviron) class Frame(m_pTables,type);
      m_pDimensions->ParseMarker(io);
      m_pImage      = m_pDimensions->BuildImage();
      m_pDimensions->SetImage(m_pImage);
      m_pImage->PrepareForDecoding();
      return m_pDimensions;
    case 0xffdf: 
      // An EXP marker segment. Only for differential frames.
      if (m_pSmallest == NULL)
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","found an EXP marker outside a hierarchical image process");
      len = io->GetWord();
      if (len != 3)
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","EXP marker size is invalid, must be three");
      len = io->Get();
      if (len == ByteStream::EOF)
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame","unexpected EOF while parsing the EXP marker");
      {
	UBYTE ehv = len >> 4;
	UBYTE evv = len & 0x0f;
	if (ehv > 1 || evv > 1)
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		    "invalid EXP marker, horizontal and vertical expansion "
		    "may be at most one");
	eh = (ehv)?true:false;
	ev = (evv)?true:false;
      }
      break;
    case 0xffc5:
    case 0xffc6:
    case 0xffc7:
    case 0xffcd:
    case 0xffce:
    case 0xffcf:
      // All differential types. This only works if a non-differential first frame is available.
      if (m_pSmallest == NULL)
	JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		  "found a differential frame outside a hierarchical image process");
      assert(m_pDimensions);
      switch(marker) {
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
      }
      {
	assert(m_pLast);
	class Frame *frame = new(m_pEnviron) class Frame(m_pTables,type);
	class Frame *prev  = m_pLast; // The frame before us.
	m_pLast->TagOn(frame);
	m_pLast    = frame;
	m_pCurrent = frame;
	m_pCurrent->ParseMarker(io);     
	if (m_pCurrent->DepthOf()     != m_pDimensions->DepthOf() ||
	    m_pCurrent->PrecisionOf() != m_pDimensions->PrecisionOf()) {
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		    "image properties indicated in the DHP marker are incompatible with the "
		    "frame properties, stream is damaged");
	}
	// Check whether the frame dimensions work all right.
	if ((!eh && prev->WidthOf()  != frame->WidthOf())             ||
	    ( eh && prev->WidthOf()  != (frame->WidthOf() + 1) >> 1)  ||
	    (( frame->HeightOf()       != 0) &&
	     ((!ev && prev->HeightOf() != frame->HeightOf())            ||
	      ( ev && prev->HeightOf() != (frame->HeightOf() + 1) >> 1))
	     )) {
	  JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		    "frame dimensions are not compatible with the the expansion factors");
	}
	((class HierarchicalBitmapRequester *)m_pImage)->AddImageScale(frame,eh,ev); 
	m_pImage->PrepareForDecoding();
	return frame;
      }
    default:
      JPG_THROW(MALFORMED_STREAM,"Image::StartParseFrame",
		"found unexpected or invalid marker, decoder is likely out of sync");
      return NULL;
    }
  } while(true);

  // The code never goes here.
  return NULL;
}
///

/// Image::StartWriteFrame
// Start writing a single scan. Scan parameters must have been installed before.
class Frame *Image::StartWriteFrame(class ByteStream *io)
{
  if (m_pCurrent == NULL) {
    assert(m_pDimensions);
    // If this is a hierarchical scan, first write the dimensions into the DHP marker.
    if (m_pSmallest) {
      io->PutWord(0xffde);
      m_pDimensions->WriteMarker(io);
      //
      // Now write the first reference frame and operate on it solely.
      m_pSmallest->ResetToFirstScan();
      m_pSmallest->WriteFrameType(io);
      m_pSmallest->WriteMarker(io);
      m_pCurrent = m_pSmallest;
      return m_pSmallest;
    } else {
      // This is a flat image (the usual case).
      m_pDimensions->ResetToFirstScan();
      m_pDimensions->WriteFrameType(io);
      m_pDimensions->WriteMarker(io);
      m_pCurrent = m_pDimensions;
      return m_pCurrent;
    }
  } else {
    // Found a current frame. If this is a flat image, this is the last frame.
    if (m_pSmallest == NULL) {
      return NULL;
    } else {
      class HierarchicalBitmapRequester *hr = (class HierarchicalBitmapRequester *)m_pImage;
      bool hexp,vexp;
      UBYTE v = 0;
      //
      // Get the exp marker and transfer the differential data into the current frame.
      hr->GenerateDifferentialImage(m_pCurrent,hexp,vexp);
      //
      // Now write the EXP marker.
      io->PutWord(0xffdf);
      io->PutWord(0x0003);
      if (hexp) v |= 0x10;
      if (vexp) v |= 0x01;
      io->Put(v);
      //
      m_pCurrent->WriteFrameType(io);
      m_pCurrent->WriteMarker(io);
      return m_pCurrent;
    }
  }
}
///

/// Image::StartMeasureFrame
// Instead of writing, just collect statistics for the huffman coder.
class Frame *Image::StartMeasureFrame(void)
{  
  if (m_pCurrent == NULL) {
    assert(m_pDimensions);
    // If this is a hierarchical scan, first write the dimensions into the DHP marker.
    if (m_pSmallest) {
      m_pSmallest->ResetToFirstScan();
      m_pCurrent = m_pSmallest;
      return m_pSmallest;
    } else {
      // This is a flat image (the usual case).
      m_pCurrent = m_pDimensions;
      return m_pCurrent;
    }
  } else {
    // Found a current frame. If this is a flat image, this is the last frame.
    if (m_pSmallest == NULL) {
      return NULL;
    } else {
      class HierarchicalBitmapRequester *hr = (class HierarchicalBitmapRequester *)m_pImage;
      bool hexp,vexp;
      //
      // Get the exp marker and transfer the differential data into the current frame.
      // exp information is not required here...
      hr->GenerateDifferentialImage(m_pCurrent,hexp,vexp);
      //
      return m_pCurrent;
    }
  }
}
///

/// Image::NextFrame
// Advance to the next frame, deliver it or NULL if there is no next frame.
class Frame *Image::NextFrame(void)
{
  if (m_pCurrent == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Image::NextFrame","no frame iteration has been started yet");

  if (m_pSmallest == NULL) {
    // Flat image, there is no next frame.
    return NULL;
  } else {
    return m_pCurrent = m_pCurrent->NextOf();
  }
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
  } else {
    m_pDimensions->ResetToFirstScan();
  }
}
///

/// Image::ReconstructRegion
// Control interface - direct forwarding to the bitmap control
// Reconstruct a rectangle of coefficients.
void Image::ReconstructRegion(class BitMapHook *bmh,const struct RectangleRequest *rr)
{
  if (m_pDimensions == NULL || m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Image::ReconstructRegion","no image loaded that could be reconstructed");

  m_pImage->ReconstructRegion(bmh,rr);
}
///

/// Image::EncodeRegion
// Encode the next region in the scan from the user bitmap. The requested region
// is indicated in the tags going to the user bitmap hook.
void Image::EncodeRegion(class BitMapHook *bmh,const struct RectangleRequest *rr)
{
  if (m_pDimensions == NULL || m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Image::EncodeRegion","no image constructed into which data could be loaded");

  m_pImage->EncodeRegion(bmh,rr);
}
///

/// Image::BufferedLines
// Return the number of lines available for reconstruction from this scan.
ULONG Image::BufferedLines(const struct RectangleRequest *rr) const
{  
  if (m_pDimensions == NULL)
    return 0; // No image, no lines.

  if (m_pImage == NULL)
    return 0;

  return m_pImage->BufferedLines(rr);
}
///

/// Image::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder.
bool Image::isNextMCULineReady(void) const
{  
  if (m_pDimensions == NULL)
    return false; // no image, nothing ready.

  if (m_pImage == NULL)
    return false;

  return m_pImage->isNextMCULineReady();
}
///

/// Image::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool Image::isImageComplete(void) const
{ 
  if (m_pDimensions == NULL)
    return false; // No image, nothing complete.

  if (m_pImage == NULL)
    return false;

  return m_pImage->isImageComplete();
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
void Image::WriteTrailer(class ByteStream *io) const
{
  io->PutWord(0xffd9);
}
///

/// Image::ParseTrailer
// Parse off the EOI marker at the end of the image. Return false
// if there are no more scans in the file, true otherwise.
bool Image::ParseTrailer(class ByteStream *io) const
{
  do {
    LONG marker = io->PeekWord();

    if (marker == 0xffd9) { //EOI
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
