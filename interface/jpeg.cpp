
/*
** Definition of the library interface
**
** This file defines the main entry points and user accessible data
** for the 10918 (JPEG) codec. Except for the tagitem and hook methods,
** no other headers should be publically accessible.
** 
** $Id: jpeg.cpp,v 1.24 2015/03/16 08:55:34 thor Exp $
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
	m_pScan = m_pFrame->StartParseScan(m_pImage->InputStreamOf(m_pIOStream),m_pImage->ChecksumOf());
	if (m_pScan && (stopflags & JPGFLAG_DECODER_STOP_SCAN))
	  return;
	if (m_pScan == NULL) {
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
  m_bOptimizeHuffman = RequiresTwoPassEncoding(tags);

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

    m_bOptimizeHuffman = RequiresTwoPassEncoding(tags);
    m_pImage           = m_pEncoder->CreateImage(tags);
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
