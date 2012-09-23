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
** Definition of the library interface
**
** This file defines the main entry points and user accessible data
** for the 10918 (JPEG) codec. Except for the tagitem and hook methods,
** no other headers should be publically accessible.
** 
** $Id: jpeg.cpp,v 1.6 2012-09-16 17:28:40 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "interface/tagitem.hpp"
#include "interface/hooks.hpp"
#include "interface/jpeg.hpp"
#include "interface/bitmaphook.hpp"
#include "codestream/rectanglerequest.hpp"
#include "codestream/encoder.hpp"
#include "codestream/decoder.hpp"
#include "codestream/image.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "io/iostream.hpp"
#include "std/assert.hpp"
///

/// Opaque helper structure for both the environment and the main class.
struct JPEG_Helper : public JPEG {
  class Environ m_Env;
};
///

/// JPEG::JPEG
// Does really nothing sensible right now. Main stuff is done in
// the doConstruct call.
JPEG::JPEG(void)
{
}
///

/// JPEG::~JPEG
JPEG::~JPEG(void)
{
  assert(m_pEnviron == NULL);
  assert(m_pEncoder == NULL);
  assert(m_pDecoder == NULL);
}
///

/// JPEG::doConstruct
// The real constructor. We must use this, since we're not using
// NEW to allocate objects, but MALLOC.
void JPEG::doConstruct(class Environ *env)
{
  m_pEnviron = env;
  m_pEncoder = NULL;
  m_pDecoder = NULL;

  //
  // State variables.
  m_pIOStream        = NULL;
  m_pImage           = NULL;
  m_pFrame           = NULL;
  m_pScan            = NULL;
  m_bRow             = false;
  m_bDecoding        = false;
  m_bEncoding        = false;
  m_bHeaderWritten   = false;
  m_bOptimized       = false;
  m_bOptimizeHuffman = false;
}
///

/// JPEG::doDestruct
// The real destructor. We must use this, since we're not using
// NEW to allocate objects, but MALLOC.
void JPEG::doDestruct(void)
{
  delete m_pEncoder;
  m_pEncoder = NULL;

  delete m_pDecoder;
  m_pDecoder = NULL;

  delete m_pIOStream;
  m_pIOStream = NULL;

  m_pEnviron = NULL; // Deleted elsewhere
}
///

/// JPEG::Construct
// Create an instance of this class.
class JPEG *JPEG::Construct(struct JPG_TagItem *tags)
{
  class Environ ev(tags);           // build up a temporary environment.
  class Environ *m_pEnviron = &ev;  // needed for J2K infrastructure
  struct JPEG_Helper *volatile h_jpeg = NULL;

  JPG_TRY {
    class Environ *env;

    h_jpeg = (struct JPEG_Helper *)JPG_MALLOC(sizeof(struct JPEG_Helper));
    env    = &(h_jpeg->m_Env);
    *env   = ev; // Copy the temporary environment over.
    h_jpeg->doConstruct(env);

  } JPG_CATCH {
    if (h_jpeg) {
      h_jpeg->m_Env.PostLastError();
      h_jpeg->m_Env.PrintException();
      h_jpeg->doDestruct();
      // This looks silly, but is required for orderly shutdown.
      ev = h_jpeg->m_Env;
      JPG_FREE(h_jpeg);
    } else {
      ev.PostLastError();
      ev.PrintException();
    }
    h_jpeg = NULL;
    // Return immediately. Do not try to clean up!
    return NULL;
  } JPG_ENDTRY;

#if CHECK_LEVEL > 0
  h_jpeg->m_Env.TestExceptionStack();
#endif
  return h_jpeg;
}
///

/// JPEG::Destruct
// The real destructor. We must use this, since we're not using
// NEW to allocate objects, but MALLOC.
void JPEG::Destruct(class JPEG *o)
{
  if (o) {
    struct JPEG_Helper *h_jpeg = (struct JPEG_Helper *)o;
    o->doDestruct();
#if CHECK_LEVEL > 0
    h_jpeg->m_Env.TestExceptionStack();
#endif
    {
      // Build a temporary environment for releasing
      // this thingy.
      class Environ ev(h_jpeg->m_Env);
      class Environ *m_pEnviron = &ev;
      JPG_FREE(o);
    }
  }
}
///

/// JPEG::Read
// This is a slim wrapper around the reader which handles
// errors.
JPG_LONG JPEG::Read(struct JPG_TagItem *tags)
{ 
  volatile JPG_LONG ret = TRUE;

  JPG_TRY {
    ReadInternal(tags);
  } JPG_CATCH {
    ret = JPG_FALSE;
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::ReadInternal
// Read a file. This takes all of the tags, class Decode takes.
void JPEG::ReadInternal(struct JPG_TagItem *tags)
{   
  LONG stopflags = tags->GetTagData(JPGTAG_DECODER_STOP);

  if (m_pEncoder)
    JPG_THROW(OBJECT_EXISTS,"JPEG::ReadInternal","encoding in process, cannot start decoding");

  if (m_pDecoder == NULL) {
    m_pDecoder       = new(m_pEnviron) class Decoder(m_pEnviron);
    m_bDecoding      = true; 
    m_pFrame         = NULL;
    m_pScan          = NULL;
    m_bRow           = false;
    m_bEncoding      = false;
  }

  if (!m_bDecoding)
    return;

  if (m_pIOStream == NULL) {
    struct JPG_Hook *iohook = (struct JPG_Hook *)(tags->GetTagPtr(JPGTAG_HOOK_IOHOOK));
    if (iohook == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::ReadInternal","no IOHook defined to read the data from");

    m_pIOStream = new(m_pEnviron) class IOStream(m_pEnviron,tags);
  }

  assert(m_pIOStream);
  
  if (m_pImage == NULL) {
    m_pImage = m_pDecoder->ParseHeader(m_pIOStream);
    if (stopflags & JPGFLAG_DECODER_STOP_IMAGE)
      return;
  }

  assert(m_pImage);

  while(m_bDecoding) {
    if (m_pFrame == NULL) {
      m_pFrame = m_pImage->StartParseFrame(m_pIOStream);
      if (m_pFrame) {
	m_pDecoder->ParseTags(tags);
	if (stopflags & JPGFLAG_DECODER_STOP_FRAME)
	  return;
      }
    }

    if (m_pFrame) {
      if (m_pScan == NULL) {
	m_pScan = m_pFrame->StartParseScan(m_pIOStream);
	if (m_pScan && (stopflags & JPGFLAG_DECODER_STOP_SCAN))
	  return;
	if (m_pScan == NULL) {
	  if (!m_pFrame->ParseTrailer(m_pIOStream)) {
	    // Frame done, advance to the next frame.
	    m_pFrame = NULL;
	    if (!m_pImage->ParseTrailer(m_pIOStream)) {
	      // Image done, stop decoding, image is now loaded.
	      m_bDecoding = false;
	      return;
	    }
	  }
	}
      }

      if (m_pScan) {
	if (m_bRow == false) {
	  m_bRow = m_pScan->StartMCURow();
	  if (m_bRow) {
	    if (stopflags & JPGFLAG_DECODER_STOP_ROW)
	      return;
	  } else {
	    // Scan done, advance to the next scan.
	    m_pScan = NULL;
	    if (!m_pFrame->ParseTrailer(m_pIOStream)) {
	      // Frame done, advance to the next frame.
	      m_pFrame = NULL;
	      if (!m_pImage->ParseTrailer(m_pIOStream)) {
		// Image done, stop decoding, image is now loaded.
		m_bDecoding = false;
		return;
	      }
	    }
	  }
	}
	
	if (m_bRow) {
	  while (m_pScan->ParseMCU()) {
	    if (stopflags & JPGFLAG_DECODER_STOP_MCU)
	      return;
	  } 
	  m_bRow = false;
	}
      }
    }
  }
}
///

/// JPEG::Write
// Write a file. This takes all of the tags, class Encode takes.
JPG_LONG JPEG::Write(struct JPG_TagItem *tags)
{ 
  volatile JPG_LONG ret = JPG_TRUE;

  JPG_TRY {
    WriteInternal(tags);
  } JPG_CATCH {
    ret = JPG_FALSE;
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::WriteInternal
// Write a file. Exceptions are thrown here and captured outside.
void JPEG::WriteInternal(struct JPG_TagItem *tags)
{ 
  LONG stopflags = tags->GetTagData(JPGTAG_ENCODER_STOP);

  if (m_pDecoder)
    JPG_THROW(OBJECT_EXISTS,"JPEG::WriteInternal","decoding in process, cannot start encoding");

  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::WriteInternal","no image data loaded, use ProvideImage first");
  
  if (m_pEncoder == NULL) {
    m_pEncoder       = new(m_pEnviron) class Encoder(m_pEnviron);
    m_bEncoding      = true; 
    m_pFrame         = NULL;
    m_pScan          = NULL;
    m_bRow           = false;
    m_bDecoding      = false;
    m_bHeaderWritten = false;
    m_bOptimized     = false;
  }

  if (!m_bEncoding)
    return;

  if (m_pIOStream == NULL) {
    struct JPG_Hook *iohook = (struct JPG_Hook *)(tags->GetTagPtr(JPGTAG_HOOK_IOHOOK));
    if (iohook == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::WriteInternal","no IOHook defined to write the data to");

    m_pIOStream = new(m_pEnviron) class IOStream(m_pEnviron,tags);
  }

  assert(m_pIOStream);
  
  if (m_bHeaderWritten == false) {
    m_pImage->WriteHeader(m_pIOStream);
    m_bHeaderWritten = true;
    if (stopflags & JPGFLAG_ENCODER_STOP_IMAGE)
      return;
  }

  assert(m_pImage);


  if (!m_bOptimized) {
    if (m_bOptimizeHuffman || (tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE) & JPGFLAG_OPTIMIZE_HUFFMAN)) {
      do {
	class Frame *frame = m_pImage->StartMeasureFrame();
	do {
	  class Scan *scan = frame->StartMeasureScan();
	  while(scan->StartMCURow()) { 
	    while(scan->WriteMCU()) {
	      ;
	    }
	  }
	  scan->Flush();
	} while(frame->NextScan());
      } while(m_pImage->NextFrame());
    }
    m_bOptimized = true;
    m_pImage->ResetToFirstFrame();
  }

  while(m_bEncoding) {
    if (m_pFrame == NULL) {
      m_pFrame = m_pImage->StartWriteFrame(m_pIOStream);
      if (stopflags & JPGFLAG_ENCODER_STOP_FRAME)
	return;
    }
    assert(m_pFrame);

    if (m_pScan == NULL) {
      m_pScan = m_pFrame->StartWriteScan(m_pIOStream);
      if (stopflags & JPGFLAG_ENCODER_STOP_SCAN)
	return;
    }
    assert(m_pScan);

    if (!m_bRow) {
      if (m_pScan->StartMCURow()) {
	m_bRow = true;
	if (stopflags & JPGFLAG_ENCODER_STOP_ROW)
	  return;
      } else {
	// Scan done, flush it out.
	m_pScan->Flush();
	m_pScan = NULL;
	m_pFrame->WriteTrailer(m_pIOStream);
	if (!m_pFrame->NextScan()) {
	  m_pFrame = NULL;
	  if (!m_pImage->NextFrame()) {
	    m_pImage->WriteTrailer(m_pIOStream);
	    m_pIOStream->Flush();
	    m_bEncoding = false;
	    return;
	  }
	}
      }
    }

    if (m_bRow) {
      while (m_pScan->WriteMCU()) {
	if (stopflags & JPGFLAG_ENCODER_STOP_MCU)
	  return;
      }
      m_bRow = false;
    }
  }
}
///

/// JPEG::DisplayRectangle
// Reverse transform a given rectangle
JPG_LONG JPEG::DisplayRectangle(struct JPG_TagItem *tags)
{  
  volatile JPG_LONG ret = JPG_TRUE;

  JPG_TRY {
    InternalDisplayRectangle(tags);
  } JPG_CATCH {
    ret = JPG_FALSE;
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalDisplayRectangle
// Reverse transform a given rectangle, internal version that
// throws exceptions.
void JPEG::InternalDisplayRectangle(struct JPG_TagItem *tags)
{
  class BitMapHook bmh(tags);
  struct RectangleRequest rr;
  
  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::InternalDisplayRectangle","no image loaded that could be displayed");


  rr.ParseTags(tags,m_pImage);
  m_pImage->ReconstructRegion(&bmh,&rr);
}
///

/// JPEG::ProvideImage
// Forward transform an image, push it into the encoder.
JPG_LONG JPEG::ProvideImage(struct JPG_TagItem *tags)
{
  volatile JPG_LONG ret = JPG_TRUE;

  JPG_TRY {
    InternalProvideImage(tags);
  } JPG_CATCH {
    ret = JPG_FALSE;
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalProvideImage
// Forward transform an image, push it into the encoder - this is the internal
// version that generates exceptions.
void JPEG::InternalProvideImage(struct JPG_TagItem *tags)
{
  class BitMapHook bmh(tags);
  struct RectangleRequest rr;
  bool loop = tags->GetTagData(JPGTAG_ENCODER_LOOP_ON_INCOMPLETE);

  if (m_bDecoding)
    JPG_THROW(OBJECT_EXISTS,"JPEG::InternalProvideImage","Decoding is active, cannot provide image data");

  if (m_pDecoder) {
    delete m_pDecoder;m_pDecoder = NULL;

    delete m_pImage;m_pImage = NULL;

    delete m_pIOStream;m_pIOStream = NULL;

    m_pFrame           = NULL;
    m_pScan            = NULL;
    m_bRow             = false;
    m_bDecoding        = false;
    m_bEncoding        = false;
    m_bHeaderWritten   = false;
    m_bOptimized       = false;
    m_bOptimizeHuffman = false;
  }

  if (m_pImage == NULL) {
    if (m_pEncoder == NULL) {
      m_pEncoder  = new(m_pEnviron) class Encoder(m_pEnviron);
      m_bEncoding = true;
    }
    if (tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE) & JPGFLAG_OPTIMIZE_HUFFMAN)
      m_bOptimizeHuffman = true;

    m_pImage = m_pEncoder->CreateImage(tags);
  }

  do {
    rr.ParseTags(tags,m_pImage);
    m_pImage->EncodeRegion(&bmh,&rr);
  } while(!m_pImage->isImageComplete() && loop);

  tags->SetTagData(JPGTAG_ENCODER_IMAGE_COMPLETE,m_pImage->isImageComplete());
}
///

/// JPEG::GetInformation
// Request information from the JPEG object.
JPG_LONG JPEG::GetInformation(struct JPG_TagItem *tags)
{ 
  volatile JPG_LONG ret = JPG_TRUE;
 
  JPG_TRY {
    InternalGetInformation(tags);
  } JPG_CATCH {
    ret = JPG_FALSE;
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalGetInformation
// Request information from the JPEG object - the internal version that creates exceptions.
void JPEG::InternalGetInformation(struct JPG_TagItem *tags)
{ 
  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::InternalGetInformation","no image loaded to request information from");

  assert(m_pImage);

  //
  // Currently, that's all. More to come later.
  tags->SetTagData(JPGTAG_IMAGE_WIDTH ,m_pImage->WidthOf());
  tags->SetTagData(JPGTAG_IMAGE_HEIGHT,m_pImage->HeightOf());
  tags->SetTagData(JPGTAG_IMAGE_DEPTH ,m_pImage->DepthOf());
  tags->SetTagData(JPGTAG_IMAGE_PRECISION,m_pImage->PrecisionOf());
}
///

/// JPEG::LastError
// Return the last exception - the error code, if present - in
// the primary result code, a pointer to the error string in the
// argument. If no error happened, return 0. For finer error handling,
// define an exception hook upon creation of the library.
JPG_LONG JPEG::LastError(const char *&error)
{
  return m_pEnviron->LastException(error);
}
///

/// JPEG::LastWarning
// Return the last warning - the error code, if present - in the
// primary result code and the warning message in the argument. Returns
// 0 if no warning happened. Define a warning hook for finer warning
// control.
JPG_LONG JPEG::LastWarning(const char *&warning)
{
  return m_pEnviron->LastWarning(warning);
}
///
