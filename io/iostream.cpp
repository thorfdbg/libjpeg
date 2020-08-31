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
** An implementation of the random access stream:
** Use user call-back hooks to perform the file IO. This is
** the back-end class to perform the real job getting bytes in and
** out of the library.
**
** $Id: iostream.cpp,v 1.8 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "iostream.hpp"
#include "interface/parameters.hpp"
#include "interface/tagitem.hpp"
#include "tools/debug.hpp"
///

/// IOStream::IOStream
// Constructor: build an IOhook structure from an ordinary hook,
// keep track of the buffer size, but do not yet allocate the buffer
// (due to the standard problem of exceptions in constructors).
IOStream::IOStream(class Environ *env,struct JPG_Hook *in,APTR stream,ULONG bufsize,ULONG userdata,UBYTE *buffer)
  : RandomAccessStream(env, bufsize), m_Hook(*in), m_pHandle(stream),
    m_ulCachedSeek(0), m_lUserData(userdata), m_pSystemBuffer(NULL), m_pUserBuffer(buffer), m_bSeekable(true)
{ 
}
///

/// IOStream::IOStream
// The taglist constructor.
IOStream::IOStream(class Environ *env,const struct JPG_TagItem *tags)
  : RandomAccessStream(env), m_Hook(&DefaultEntry,this), m_pHandle(NULL),
    m_ulCachedSeek(0), m_lUserData(0), m_pSystemBuffer(NULL), m_pUserBuffer(NULL), m_bSeekable(true)
{
  
  while(tags) {
    switch(tags->ti_Tag) {
    case JPGTAG_HOOK_IOHOOK:
      if (tags->ti_Data.ti_pPtr) 
        m_Hook = *(struct JPG_Hook *)(tags->ti_Data.ti_pPtr);
      break;
    case JPGTAG_HOOK_IOSTREAM:
      m_pHandle     = tags->ti_Data.ti_pPtr;
      break;
    case JPGTAG_HOOK_BUFFERSIZE:
      m_ulBufSize   = tags->ti_Data.ti_lData;
      break;
    case JPGTAG_FIO_USERDATA:
      m_lUserData   = tags->ti_Data.ti_lData;
      break;
    case JPGTAG_HOOK_BUFFER:
      m_pUserBuffer = tags->ti_Data.ti_pPtr;
      break;
    }
    tags = tags->NextTagItem();
  }
}
///

/// IOStream::~IOStream
// Destructor: This just gets rid of the buffer, but does not flush
// the buffer itself. If it had to, I'd to keep track whether this 
// stream was used for reading or writing, and had to flush it
// for writing, only. Ok, I could do so, but I'd to keep one additional
// flag I currently don't want to invest. 
// Just flush it before you close it, a'right?
IOStream::~IOStream(void)
{
  if (m_pSystemBuffer)
    m_pEnviron->FreeMem(m_pSystemBuffer,m_ulBufSize + 1);
}
///

/// IOStream::DefaultEntry
// A dummy hook entry that is used when the user provided no hook.
LONG IOStream::DefaultEntry(struct JPG_Hook *hk,struct JPG_TagItem *)
{
  class IOStream *io        = (class IOStream *)hk->hk_pData;
  class Environ *m_pEnviron = io->EnvironOf();
  //
  // This just throws an error.
  JPG_THROW(MISSING_PARAMETER, "IOStream::DefaultEntry","IO Hook argument missing");
  //
  // will never go here...
  return -1;
}
///

/// IOStream::Fill
// Re-fill the internal buffer on reading
LONG IOStream::Fill(void)
{
  LONG bytes;

  // Check whether this buffer is seekable.
  // In case it is and there are outstanding
  // seeks pending, seek now.
  if (m_bSeekable) {
    if (m_ulCachedSeek) {
      if (AdvanceFilePointer(m_ulCachedSeek)) {
        // Did work, reset the pointer.
        m_ulCachedSeek = 0;
      } else {
        // Do not try to seek again here... Fix
        // the missing seek otherwise by reading
        // data from the buffer.
        m_bSeekable    = false;
      }
    }
  }
  
  //
  // get the buffer
  if (m_pucBuffer == NULL) {
    // get a new buffer
    if (m_pUserBuffer) {
      m_pucBuffer   = (UBYTE *) m_pUserBuffer;
    } else {
      // to be able to undo two reads instead of one, we
      // must allocate a buffer that is a tad larger to hold the additional
      // character we might want to un-put
      m_pSystemBuffer = m_pEnviron->AllocMem(m_ulBufSize + 1);
      m_pucBuffer     = (UBYTE *) m_pSystemBuffer;
    }
  } else {
    // Otherwise increment the file counter by the offset of the
    // bytes read so far
    m_uqCounter += m_pucBufPtr - m_pucBuffer;
  }
  //
  // Construct the tags now.
  {
    JPG_TagItem tags[] = {
      JPG_PointerTag(JPGTAG_FIO_BUFFER,m_pucBuffer),
      JPG_ValueTag(JPGTAG_FIO_SIZE,m_ulBufSize),
      JPG_PointerTag(JPGTAG_FIO_HANDLE,m_pHandle),
      JPG_ValueTag(JPGTAG_FIO_ACTION,JPGFLAG_ACTION_READ),
      JPG_ValueTag(JPGTAG_FIO_USERDATA,m_lUserData),
      JPG_EndTag
    };
    //
    do {
      // Now initiate a new read
      if ((bytes = m_Hook.CallLong(tags)) < 0) {
        JPG_THROW_INT(Query(), "IOStream::Fill", 
                      "Client signalled an error on reading from the file hook");
      }
      m_pucBuffer  = (UBYTE *)tags[0].ti_Data.ti_pPtr;  // re-fetch the buffer
      m_pucBufPtr  = m_pucBuffer;              // re-initiate the buffer pointer
      m_pucBufEnd  = m_pucBuffer + bytes;      // re-initiate the buffer end
      m_lUserData  = tags[4].ti_Data.ti_lData;
      //
      // If we have an EOF or no cached seeks, abort here there's nothing
      // more we could do.
      if (bytes == 0 || m_ulCachedSeek == 0)
        return bytes;
      //
      // Check whether we have some pending seeks to be taken care of. The
      // virtual file pointer has been incremented already, so we just must
      // read data back.
      // Try to advance the buffer pointer by the largest possible amount.
      if (ULONG(bytes) > m_ulCachedSeek) {
        // Ok, there's not that much overload here,
        // remove the bytes from the buffer and
        // let it go. Furthermore ensure that the invariance
        // file position = position of the start of the buffer
        // remains valid.
        bytes         -= m_ulCachedSeek;
        m_pucBufPtr   += m_ulCachedSeek;
        m_uqCounter   -= m_ulCachedSeek;
        m_ulCachedSeek = 0;
        return bytes;
      }
      // No, just too many bytes to seek over. Remove
      // all of them from the buffer and read more.
      m_ulCachedSeek  -= bytes;
    } while(true);
  }
  //
  // We should never go here...
  return 0;
}
///

/// IOStream::Flush
// Write out the contents of the internal buffer
void IOStream::Flush(void)
{
  LONG bytes;
  UBYTE *bufstart;
  ULONG bufbytes = m_ulBufSize;

  //
  // In case seeks are pending here, generate an
  // error. We cannot forward-seek on writing
  // files (at least not legally with JPG)
  assert(m_ulCachedSeek == 0);
  //
  bufstart     = m_pucBuffer;
  if (bufstart) {    // do nothing if we don't come with a buffer...
    ULONG bytestowrite;
    //
    JPG_TagItem tags[] = {
      JPG_PointerTag(JPGTAG_FIO_BUFFER,m_pucBuffer),
      JPG_ValueTag(JPGTAG_FIO_SIZE,bufbytes), // will be overwritten by the proper size
      JPG_PointerTag(JPGTAG_FIO_HANDLE,m_pHandle),
      JPG_ValueTag(JPGTAG_FIO_ACTION,JPGFLAG_ACTION_WRITE),      
      JPG_ValueTag(JPGTAG_FIO_USERDATA,m_lUserData),
      JPG_EndTag
    };
    //
    // number of valid bytes in the buffer.
    bytestowrite = m_pucBufPtr - m_pucBuffer;
    
    while(bytestowrite) {
      tags[0].ti_Data.ti_pPtr  = bufstart;
      tags[1].ti_Data.ti_lData = bytestowrite;
      if ((bytes = m_Hook.CallLong(tags)) < 0) {
        JPG_THROW_INT(Query(), "IOStream::Flush", 
                  "Client signalled error on flushing the IO buffer");
      }
      bytestowrite -= bytes;
      bufstart     += bytes;
      m_uqCounter  += bytes;
    }
    //
    // Re-fetch the buffer and user data.
    m_pucBuffer = (UBYTE *)tags[0].ti_Data.ti_pPtr;
    if (m_pucBuffer == m_pSystemBuffer) {
      bufbytes  = m_ulBufSize;
    } else{
      bufbytes  = tags[1].ti_Data.ti_lData;
    }
    m_lUserData = tags[4].ti_Data.ti_lData;
  }
  
  if (m_pucBuffer == NULL) {
    // get a new buffer
    if (m_pUserBuffer) {
      m_pucBuffer     = (UBYTE *) m_pUserBuffer;
    } else {
      m_pSystemBuffer = m_pEnviron->AllocMem(m_ulBufSize + 1);
      m_pucBuffer     = (UBYTE *) m_pSystemBuffer;
    }
    bufbytes          = m_ulBufSize;
  }
  
  m_pucBufPtr  = m_pucBuffer;
  m_pucBufEnd  = m_pucBuffer + bufbytes;
}
///

/// IOStream::AdvanceFilePointer
// Skip bytes by first trying to seek over and then by trying to continuously
// read over the bytes. Returns false in case seeking did not
// work and we must advance the file pointer manually by reading
// data.
bool IOStream::AdvanceFilePointer(ULONG skip)
{
  LONG newpos;
  //
  JPG_TagItem tags[] = {
    JPG_ValueTag(JPGTAG_FIO_OFFSET,skip),
    JPG_PointerTag(JPGTAG_FIO_HANDLE,m_pHandle),
    JPG_ValueTag(JPGTAG_FIO_SEEKMODE,JPGFLAG_OFFSET_CURRENT),
    JPG_ValueTag(JPGTAG_FIO_ACTION,JPGFLAG_ACTION_SEEK),      
    JPG_ValueTag(JPGTAG_FIO_USERDATA,m_lUserData),
    JPG_EndTag
  };
  // Move the buffer, return the new file pointer
  newpos = m_Hook.CallLong(tags);
  if (newpos != -1) { 
    // worked fine
    m_lUserData = tags[4].ti_Data.ti_lData;   
    return true;
  }
  //
  return false;
}
///

/// IOStream::SkipBytes
// Skip bytes by first trying to seek over and then by trying to continuously
// read over the bytes
void IOStream::SkipBytes(ULONG skip)
{
  ULONG remains;
  //
  remains = skip;
  
  while(remains) {
    ULONG skipbytes;
    ULONG avail = m_pucBufEnd - m_pucBufPtr;
    //
    // If we have no buffer or the buffer is empty, try to seek over
    // or refill the buffer.
    if (avail == 0) {
      if (m_bSeekable) {
        ULONG nextseek = m_ulCachedSeek + remains;
        // Try to cache the seek. First check
        // whether the caching would overflow.
        // If so, seek completely.
        if (nextseek < m_ulCachedSeek || nextseek >= MAX_LONG) {
          if (m_ulCachedSeek == 0) {
            m_uqCounter    += remains;
            m_ulCachedSeek += remains;
            remains         = 0;
          }
          if (AdvanceFilePointer(m_ulCachedSeek)) {
            m_ulCachedSeek = 0;
          } else {
            // Could not seek, bummer. Retry from loop.
            m_bSeekable = false;
            continue;
          }
        }
        // Now advance the virtual file pointer and
        // buffer the seek.
        //AdvanceFilePointer(remains);
        m_uqCounter    += remains;
        m_ulCachedSeek += remains;
        //
        // That's all for now.
        return;
      }
      //
      // We cannot seek, fetch data instead. Don't
      // try to seek in there. Note that Fill() also handles
      // the case of still outstanding cached seeks we might
      // have here in case the stream wasn't seekable.
      if (Fill() == 0) {
        JPG_THROW(UNEXPECTED_EOF, "IOStream::SkipBytes", 
              "unexpected EOF while skipping bytes");
      }
    }
    //
    // Check how many bytes we can use up by buffering
    skipbytes     = avail;
    if (skipbytes > remains)
      skipbytes   = remains;
    remains      -= skipbytes;
    //
    // Increment buffer position and decrement valid # of bytes within buffer
    m_pucBufPtr  += skipbytes;
  }
}
///

/// IOStream::Query
// Get the status of the user interface
LONG IOStream::Query(void)
{
  LONG result;
  //
  //
  JPG_TagItem tags[] = {
    JPG_PointerTag(JPGTAG_FIO_HANDLE,m_pHandle),
    JPG_ValueTag(JPGTAG_FIO_ACTION,JPGFLAG_ACTION_QUERY),
    JPG_ValueTag(JPGTAG_FIO_USERDATA,m_lUserData),
    JPG_EndTag
  };

  result = m_Hook.CallLong(tags);
  //
  // Update user data
  m_lUserData = tags[2].ti_Data.ti_lData;
  //
  return result;
}
///

/// IOStream::SetFilePointer
// Set the file pointer to the indicated position (read only). This may
// seek within the stream. Note that this implements an absolute 
// seek relative to the start of the file.
void IOStream::SetFilePointer(UQUAD newpos)
{
  //
  do { 
    UQUAD current = FilePosition();
    //
    // Check whether the indicated file pointer is still within
    // the original buffer. If so, just set the buffer pointer
    // and let it go.
    if (newpos > current) {
      UQUAD skip;
      // The new position is "right" to the current. Check by how much
      // if small enough, we may get away just by using the buffer.
      // Skip forwards. This is more effective anyhow if it applies.
      //
      // To avoid potential problems, do not advance more than MAX_LONG
      // bytes at once, and skip in these unities until we reach the
      // target.
      skip = newpos - current;
      if (skip > MAX_LONG)
        skip = MAX_LONG;
      //
      // Now do the skip here.
      SkipBytes(ULONG(skip));
    } else if (newpos == current) {
      // We are where we want to be.
      return;
    } else {
      UQUAD target;
      // Otherwise, the buffer is invalid and we must seek manually
      // to the desired position.
      //
      // Do not advance more than MAX_LONG here, otherwise client
      // code might overrun somewhere, and I don't want to risk that.
      // We will loop until the desired position is reached.
      target = newpos;
      if (target > MAX_LONG)
        target = MAX_LONG;
      //
      // NOTE: We do not support writing here, dirty buffers aren't
      // pushed intentionally.
      //
      {
        LONG result;
        //
        JPG_TagItem tags[] = {
          JPG_ValueTag(JPGTAG_FIO_OFFSET,target),
          JPG_PointerTag(JPGTAG_FIO_HANDLE,m_pHandle),
          JPG_ValueTag(JPGTAG_FIO_SEEKMODE,JPGFLAG_OFFSET_BEGINNING),
          JPG_ValueTag(JPGTAG_FIO_ACTION,JPGFLAG_ACTION_SEEK),      
          JPG_ValueTag(JPGTAG_FIO_USERDATA,m_lUserData),
          JPG_EndTag
        };
        // Move the buffer, return the new file pointer
        result = m_Hook.CallLong(tags);
        if (result != -1) { // worked fine
          m_lUserData    = tags[4].ti_Data.ti_lData;   
          // Set the file counter to the target position
          m_uqCounter    = target; 
          m_pucBufPtr    = m_pucBuffer; // ensure that FilePosition works
          m_pucBufEnd    = m_pucBuffer; // declare buffer as empty.
          m_ulCachedSeek = 0; // clearly, as we are right where we want to be.
        } else {
          JPG_THROW_INT(Query(), "IOStream::SetFilePointer", 
                        "Server signalled an error on seeking in the file hook");
        }
      }
    }
  } while(true);
}
///

/// IOStream::Seek
// An unbuffered file seeking method that works on reading and writing,
// but requires proper buffer flushing to make it work.
void IOStream::Seek(QUAD newpos,LONG mode)
{
  switch(mode) {
  case JPGFLAG_OFFSET_CURRENT:
    m_uqCounter += newpos;
    break;
  case JPGFLAG_OFFSET_BEGINNING:
    m_uqCounter  = newpos;
    break;
  }
  while(newpos) {
    QUAD skip;
    //
    // Skip as much in the indicated direction as possible.
    skip = newpos;
    if (skip < MIN_LONG)
      skip = MIN_LONG;
    if (skip > MAX_LONG)
      skip = MAX_LONG;
    //
    JPG_TagItem tags[] = {
      JPG_ValueTag(JPGTAG_FIO_OFFSET,skip),
      JPG_PointerTag(JPGTAG_FIO_HANDLE,m_pHandle),
      JPG_ValueTag(JPGTAG_FIO_SEEKMODE,mode),
      JPG_ValueTag(JPGTAG_FIO_ACTION,JPGFLAG_ACTION_SEEK),      
      JPG_ValueTag(JPGTAG_FIO_USERDATA,m_lUserData),
      JPG_EndTag
    };
    // Move the buffer, return the new file pointer
    if (m_Hook.CallLong(tags) == -1) {
      // An error! Yikes!
      JPG_THROW_INT(Query(), "IOStream::Seek", "Client signalled error on seeking");
    }
    //
    m_lUserData = tags[4].ti_Data.ti_lData;   
    newpos     -= skip;
    mode        = JPGFLAG_OFFSET_CURRENT;
  }
}
///

/// IOStream::PeekWord
// Peek the next marker in the stream, deliver the marker without
// advancing the file pointer. Deliver EOF in case we run into
// the end of the stream. This stream requires a specific implementation
// of the primitive.
LONG IOStream::PeekWord(void)
{
  LONG byte1,byte2;
  //
  // First, read the first and the second byte
  byte1 = Get();
  if (byte1 != EOF) {
    byte2 = Get();
    if (byte2 != EOF) {
      // Here we are in a mess... We must un-get two characters whereas
      // the base class only guarantees one.
      // We can always un-do one single get. So do it now.
      LastUnDo();
      // Ok, now check whether we can undo the first get as well.
      if (m_pucBufPtr>m_pucBuffer) {
        // Yes, we can.
        LastUnDo();
      } else {
        // Otherwise, we're in a mess. If the buffer is the system buffer,
        // then we can move the bytes around because there is one additional
        // byte in the system buffer for just that. 
        if (m_pucBuffer == (UBYTE *)m_pSystemBuffer) {
          // *Yuck*
          memmove(m_pucBuffer + 1,m_pucBuffer,m_pucBufEnd - m_pucBuffer);
          m_pucBuffer[0] = (UBYTE) byte1;
          m_pucBufEnd++;
        } else {
          ULONG bytes = m_pucBufEnd - m_pucBuffer;
          UBYTE *buf;
          // Check for the amount of data in the current buffer. If there
          // is not enough room, allocate or enlarge the system buffer.
          if (m_pSystemBuffer && bytes > m_ulBufSize) {
            m_pEnviron->FreeMem(m_pSystemBuffer,m_ulBufSize + 1);
            m_pSystemBuffer = NULL;
          }
          // If no buffer is available, create one now.
          if (m_pSystemBuffer == NULL) {
            m_ulBufSize     = bytes;
            m_pSystemBuffer = m_pEnviron->AllocMem(m_ulBufSize + 1);
          }
          //
          // Otherwise (or now), we know that there is enough room in the
          // system buffer, so copy the user data over.
          buf    = (UBYTE *)m_pSystemBuffer;
          buf[0] = (UBYTE) byte1;
          memcpy(buf + 1,m_pucBuffer,bytes);
          m_pucBuffer = buf;
          m_pucBufPtr = buf;
          m_pucBufEnd = buf + bytes + 1;
        }
        // Fixup the position in the stream.
        m_uqCounter--;
      }
      // Deliver the result.
      return ((byte1<<8) | byte2);
    }
    //
    // Avoid to modify the user buffer.
    if (m_pucBuffer != (UBYTE *)m_pSystemBuffer) {
      // We really need a system buffer here. *Sigh*.
      if (m_pSystemBuffer == NULL) {
        // This is already at least one byte large,
        // and thus large enough to contain the
        // single character we need to keep.
        m_pSystemBuffer = m_pEnviron->AllocMem(m_ulBufSize + 1);
      }
      m_pucBuffer = (UBYTE *)m_pSystemBuffer;
    }
    // Unput the first character at least
    // and hack it into the buffer.
    assert(m_pucBuffer);
    m_pucBufPtr    = m_pucBuffer;
    m_pucBuffer[0] = byte1;
    m_pucBufEnd    = m_pucBuffer + 1;
    m_uqCounter--;
  }
  // We are in the EOF case either.
  return EOF;
}
///
