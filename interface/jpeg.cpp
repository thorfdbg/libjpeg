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
** Definition of the library interface
**
** This file defines the main entry points and user accessible data
** for the 10918 (JPEG) codec. Except for the tagitem and hook methods,
** no other headers should be publically accessible.
** 
** $Id: jpeg.cpp,v 1.29 2021/12/01 11:14:42 thor Exp $
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
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "boxes/mergingspecbox.hpp"
#include "boxes/checksumbox.hpp"
#include "tools/checksum.hpp"
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
  m_pIOStream          = NULL;
  m_pImage             = NULL;
  m_pFrame             = NULL;
  m_pScan              = NULL;
  m_bRow               = false;
  m_bDecoding          = false;
  m_bEncoding          = false;
  m_bHeaderWritten     = false;
  m_bOptimized         = false;
  m_bOptimizeHuffman   = false;
  m_bOptimizeQuantizer = false;
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

/// JPEG::StopDecoding
// Complete the decoding, test for the checksum,
// then exit.
void JPEG::StopDecoding(void)
{ 
  if (m_pImage) {
    class ChecksumBox *box;
    class Checksum    *sum;
    // Make sure we don't get the residual, but the legacy image.
    m_pImage->ResetToFirstFrame();
    box = m_pImage->TablesOf()->ChecksumOf();
    sum = m_pImage->ChecksumOf();
    if (box && sum) {
      if (sum->ValueOf() != box->ValueOf()) {
        JPG_WARN(PHASE_ERROR,"Frame::StopDecoding",
                 "Found a mismatching checksum of the legacy stream, HDR reconstructed image may be wrong");
      }
    }
  }
  m_bDecoding = false;
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
  
  while (m_pImage == NULL) {
    // Several iterations may be necessary to parse
    // off the header, each taking one marker of the
    // header.
    m_pImage = m_pDecoder->ParseHeaderIncremental(m_pIOStream);
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
      while (m_pScan == NULL) {
        m_pScan = m_pFrame->StartParseScan(m_pImage->InputStreamOf(m_pIOStream),m_pImage->ChecksumOf());
        //
        if (m_pScan == NULL) {
          // This is not yet the start of the scan, but might
          // either be a frame trailer, or part of the frame header.
          if (m_pFrame->isEndOfFrame()) {
            if (!m_pFrame->ParseTrailer(m_pImage->InputStreamOf(m_pIOStream))) {
              // Frame done, advance to the next frame.
              m_pFrame = NULL;
              if (!m_pImage->ParseTrailer(m_pIOStream)) {
                // Image done, stop decoding, image is now loaded.
                StopDecoding();
                return;
              }
            }
          } else {
            if (stopflags & JPGFLAG_DECODER_STOP_FRAME)
              return;
          }
          // else continue looking for the start of scan.
        } else {
          if (stopflags & JPGFLAG_DECODER_STOP_SCAN)
            return;
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
            m_pFrame->EndParseScan();
            m_pScan = NULL;
            if (!m_pFrame->ParseTrailer(m_pImage->InputStreamOf(m_pIOStream))) {
              // Frame done, advance to the next frame.
              m_pFrame = NULL;
              if (!m_pImage->ParseTrailer(m_pIOStream)) {
                // Image done, stop decoding, image is now loaded.
                StopDecoding();
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
    m_bEncoding      = true; 
    m_pFrame         = NULL;
    m_pScan          = NULL;
    m_bRow           = false;
    m_bDecoding      = false;
    m_bHeaderWritten = false;
    m_bOptimized     = false;
  }

  //
  // Actually, we do not need the encoder class here.
  m_bOptimizeHuffman   = RequiresTwoPassEncoding(tags);

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
    // Run the R/D optimization over the DC part if we have not yet
    // done that. This is a joint optimization that requires full
    // access to all data and cannot be run on the fly.
    if (m_bOptimizeQuantizer) {
      do {
        class Frame *frame = m_pImage->StartOptimizeFrame();
        do {
          class Scan *scan = frame->StartOptimizeScan();
          scan->OptimizeDC();
        } while(frame->NextScan());
      } while(m_pImage->NextFrame());
    }
    // Now try find a better Huffman coding.
    if (m_bOptimizeHuffman) {
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
      m_pScan = m_pFrame->StartWriteScan(m_pImage->OutputStreamOf(m_pIOStream),m_pImage->ChecksumOf());
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
        m_pFrame->EndWriteScan();
        //m_pScan->Flush(); included in the above.
        // This will write the DNL marker.
        m_pFrame->CompleteRefimentScan(m_pIOStream);
        m_pFrame->WriteTrailer(m_pImage->OutputStreamOf(m_pIOStream));
        m_pScan = NULL;
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

/// JPEG::PeekMarker
// In case reading was interrupted by a JPGTAG_DECODER_STOP mask
// at some point in the codestream, this call returns the next
// 16 bits at the current stop position without removing them
// from the stream. You may want to use these bits to decide
// whether you (instead of the library) want to parse the following
// data off yourself with the calls below. This call returns -1 on
// an EOF or any other error.
// Currently, the tags argument is not used, just pass NULL.
JPG_LONG JPEG::PeekMarker(struct JPG_TagItem *tags)
{
  volatile JPG_LONG ret = 0;

  JPG_TRY {
    ret = InternalPeekMarker(tags);
  } JPG_CATCH {
    ret = -1; // Deliver an error.
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalPeekMarker
// Return the marker at the current stream position or -1 for an error.
// tags is currently unused.
JPG_LONG JPEG::InternalPeekMarker(struct JPG_TagItem *) const
{
  LONG marker;
  
  if (m_pEncoder)
     JPG_THROW(OBJECT_EXISTS,"JPEG::PeekMarker","encoding in process, cannot read data");

  if (m_pDecoder == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::PeekMarker","decoding not in progress");

  if (m_pIOStream == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::PeekMarker","I/O stream does not exist, decoding did not start yet");

  marker = m_pIOStream->PeekWord();
  
  switch(marker) {
  case 0xffc0:
  case 0xffc1:
  case 0xffc2:
  case 0xffc3:
  case 0xffc5:
  case 0xffc6:
  case 0xffc7:
  case 0xffc8:
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
    // These are all markers that cannot be handled externally in any case.
    return 0;
  }
  
  return marker;
}
///

/// JPEG::ReadMarker
// In case reading was interrupted by a JPGTAG_DECODER_STOP mask,
// remove parts of the data from the stream outside of the library.
// This call reads the given number of bytes into the supplied
// buffer and returns the number of bytes it was able to read, or -1
// for an error. The tags argument is currently unused and should be
// set to NULL.
JPG_LONG JPEG::ReadMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *tags)
{
  volatile JPG_LONG ret = 0;

  JPG_TRY {
    ret = InternalReadMarker(buffer,bufsize,tags);
  } JPG_CATCH {
    ret = -1; // Deliver an error.
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalReadMarker
// Read data from the current stream position, return -1 for an error.
JPG_LONG JPEG::InternalReadMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *) const
{
  if (m_pEncoder)
     JPG_THROW(OBJECT_EXISTS,"JPEG::ReadMarker","encoding in process, cannot read data");

  if (m_pDecoder == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::ReadMarker","decoding not in progress");

  if (m_pIOStream == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::ReadMarker","I/O stream does not exist, decoding did not start yet");

  return m_pIOStream->Read((UBYTE *)buffer,bufsize);
}
///

/// JPEG::SkipMarker
// Skip over the given number of bytes. Returns -1 for failure, anything
// else for success. The tags argument is currently unused and should
// be set to NULL.
JPG_LONG JPEG::SkipMarker(JPG_LONG bytes,struct JPG_TagItem *tags)
{
  volatile JPG_LONG ret = 0;

  JPG_TRY {
    ret = InternalSkipMarker(bytes,tags);
  } JPG_CATCH {
    ret = -1; // Deliver an error.
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalSkipMarker
// Skip over the given number of bytes.
JPG_LONG JPEG::InternalSkipMarker(JPG_LONG bytes,struct JPG_TagItem *) const
{
  if (m_pEncoder)
     JPG_THROW(OBJECT_EXISTS,"JPEG::SkipMarker","encoding in process, cannot read data");

  if (m_pDecoder == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::SkipMarker","decoding not in progress");

  if (m_pIOStream == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::SkipMarker","I/O stream does not exist, decoding did not start yet");

  m_pIOStream->SkipBytes(bytes);

  return 0;
}
///

/// JPEG::WriteMarker
// In case writing was interrupted by a JPGTAG_ENCODER_STOP mask,
// this call can be used to inject additional data into the codestream.
// The typical application of this call is to inject custom markers.
// It writes the bytes in the buffer of the given size to the stream,
// and returns the number of bytes it could write, or -1 for an error.
// The tags argument is currently unused and should be set to NULL.
JPG_LONG JPEG::WriteMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *tags)
{
  volatile JPG_LONG ret = 0;

  JPG_TRY {
    ret = InternalWriteMarker(buffer,bufsize,tags);
  } JPG_CATCH {
    ret = -1; // Deliver an error.
  } JPG_ENDTRY;

  return ret;
}
///

/// JPEG::InternalWriteMarker
// Write data to the current stream position.
JPG_LONG JPEG::InternalWriteMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *) const
{
  if (m_pDecoder)
     JPG_THROW(OBJECT_EXISTS,"JPEG::WriteMarker","decoding in process, cannot write data");

  if (m_pEncoder == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::WriteMarker","encoding not in progress");

  if (m_pIOStream == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::WriteMarker","I/O stream does not exist, decoding did not start yet");

  return m_pIOStream->Write((UBYTE *)buffer,bufsize);
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

/// JPEG::RequiresTwoPassEncoding
// Check whether any of the scans is optimized Huffman and thus requires a two-pass
// go over the data.
bool JPEG::RequiresTwoPassEncoding(const struct JPG_TagItem *tags) const
{
  if (m_bOptimizeHuffman)
    return true;

  if (tags) {
    const struct JPG_TagItem *alphatags = (const struct JPG_TagItem *)tags->GetTagPtr(JPGTAG_ALPHA_TAGLIST);
    
    if (tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE) & JPGFLAG_OPTIMIZE_HUFFMAN)
      return true;

    if (tags->GetTagData(JPGTAG_RESIDUAL_FRAMETYPE) & JPGFLAG_OPTIMIZE_HUFFMAN)
      return true;

    if (alphatags) {
      if (alphatags->GetTagData(JPGTAG_IMAGE_FRAMETYPE) & JPGFLAG_OPTIMIZE_HUFFMAN)
        return true;

      if (alphatags->GetTagData(JPGTAG_RESIDUAL_FRAMETYPE) & JPGFLAG_OPTIMIZE_HUFFMAN)
        return true;
    }
  }

  return false;
}
///

/// JPEG::InternalProvideImage
// Forward transform an image, push it into the encoder - this is the internal
// version that generates exceptions.
void JPEG::InternalProvideImage(struct JPG_TagItem *tags)
{
  class BitMapHook bmh(tags);
  struct RectangleRequest rr;
  bool loop = tags->GetTagData(JPGTAG_ENCODER_LOOP_ON_INCOMPLETE)?true:false;

  if (m_bDecoding)
    JPG_THROW(OBJECT_EXISTS,"JPEG::InternalProvideImage","Decoding is active, cannot provide image data");

  if (m_pDecoder) {
    delete m_pDecoder;m_pDecoder = NULL;

    delete m_pImage;m_pImage = NULL;

    delete m_pIOStream;m_pIOStream = NULL;

    m_pFrame             = NULL;
    m_pScan              = NULL;
    m_bRow               = false;
    m_bDecoding          = false;
    m_bEncoding          = false;
    m_bHeaderWritten     = false;
    m_bOptimized         = false;
    m_bOptimizeHuffman   = false;
    m_bOptimizeQuantizer = false;
  }

  if (m_pImage == NULL) {
    if (m_pEncoder == NULL) {
      m_pEncoder  = new(m_pEnviron) class Encoder(m_pEnviron);
      m_bEncoding = true;
    }
    m_bOptimizeHuffman   = RequiresTwoPassEncoding(tags);
    m_bOptimizeQuantizer = tags->GetTagData(JPGTAG_OPTIMIZE_QUANTIZER,false)?true:false;
    m_pImage             = m_pEncoder->CreateImage(tags);
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

/// JPEG::GetOutputInformation
// Return layout information about floating point and conversion from the specs
// and insert it into the given tag list.
void JPEG::GetOutputInformation(class MergingSpecBox *specs,struct JPG_TagItem *tags) const
{
  bool isfloat = false;
  bool usesoc  = false;
  
  if (specs) {
    if (specs->usesOutputConversion()) {
      isfloat = true;
      usesoc  = true;
    } else if (specs->usesClipping()) {
      isfloat = false;
      usesoc  = false;
    } else if (specs->isLossless()) {
      isfloat = false;
      usesoc  = false;
    } else {
      isfloat = true;
      usesoc  = false;
    }
  }

  tags->SetTagData(JPGTAG_IMAGE_IS_FLOAT,isfloat);
  tags->SetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION,usesoc);
}
///

/// JPEG::InternalGetInformation
// Request information from the JPEG object - the internal version that creates exceptions.
void JPEG::InternalGetInformation(struct JPG_TagItem *tags)
{ 
  class Tables *tables;
  struct JPG_TagItem *alphatag  = tags->FindTagItem(JPGTAG_ALPHA_MODE);
  struct JPG_TagItem *alphalist = tags->FindTagItem(JPGTAG_ALPHA_TAGLIST);

  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::InternalGetInformation","no image loaded to request information from");

  assert(m_pImage);

  //
  // Currently, that's all. More to come later.
  tags->SetTagData(JPGTAG_IMAGE_WIDTH ,m_pImage->WidthOf());
  tags->SetTagData(JPGTAG_IMAGE_HEIGHT,m_pImage->HeightOf());
  tags->SetTagData(JPGTAG_IMAGE_DEPTH ,m_pImage->DepthOf());
  tags->SetTagData(JPGTAG_IMAGE_PRECISION,m_pImage->PrecisionOf());
  tables = m_pImage->TablesOf();
  if (tables) {
    class MergingSpecBox *specs = tables->ResidualSpecsOf();
    class MergingSpecBox *alpha = tables->AlphaSpecsOf();
    class Image *alphachannel   = m_pImage->AlphaChannelOf();
    ULONG tablesz               = tags->GetTagData(JPGTAG_IMAGE_SUBLENGTH);
    UBYTE *subxtable            = NULL;
    UBYTE *subytable            = NULL;

    if (tablesz) {
      UWORD c,depth = m_pImage->DepthOf();
      subxtable = (UBYTE *)tags->GetTagPtr(JPGTAG_IMAGE_SUBX);
      subytable = (UBYTE *)tags->GetTagPtr(JPGTAG_IMAGE_SUBY);
      if (subxtable)
        memset(subxtable,0,tablesz);
      if (subytable)
        memset(subytable,0,tablesz);
      //
      // Request now the subsampling parameters of the components in the image.
      if (depth > tablesz)
        depth = tablesz;
      for(c = 0;c < depth;c++) {
        class Frame *frame    = m_pImage->FirstFrameOf();
        if (frame) {
          class Component *comp = frame->ComponentOf(c);
          if (comp) {
            if (subxtable)
              subxtable[c] = comp->SubXOf();
            if (subytable)
              subytable[c] = comp->SubYOf();
          }
        }
      }
    }

    GetOutputInformation(specs,tags);

    
    
    if (alpha && alphachannel) {
      ULONG r,g,b;
      BYTE mode = alpha->AlphaModeOf(r,g,b);
      
      if (mode >= 0) {
        if (alphatag)
          alphatag->ti_Data.ti_lData = mode;
        tags->SetTagData(JPGTAG_ALPHA_MATTE(0),r);
        tags->SetTagData(JPGTAG_ALPHA_MATTE(1),g);
        tags->SetTagData(JPGTAG_ALPHA_MATTE(2),b);
        
        if (alphalist) {
          alphalist = (struct JPG_TagItem *)(alphalist->ti_Data.ti_pPtr);
          alphalist->SetTagData(JPGTAG_IMAGE_PRECISION,alphachannel->PrecisionOf());
          GetOutputInformation(alpha,alphalist);
        }
      } else {
        if (alphatag) 
          alphatag->ti_Tag  = JPGTAG_TAG_IGNORE;
        if (alphalist)
          alphalist->ti_Tag = JPGTAG_TAG_IGNORE;
      }
    } else {
      if (alphatag)
        alphatag->ti_Tag  = JPGTAG_TAG_IGNORE;
      if (alphalist)
        alphalist->ti_Tag = JPGTAG_TAG_IGNORE;
    }
  } else {
    JPG_THROW(OBJECT_DOESNT_EXIST,"JPEG::InternalGetInformation","no image created or loaded");
  }
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
