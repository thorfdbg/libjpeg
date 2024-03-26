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
 * Base class for all IO support functions, the abstract ByteStream
 * class.
 *
 * $Id: bytestream.cpp,v 1.9 2024/03/25 18:42:33 thor Exp $
 *
 */

/// Includes
#include "bytestream.hpp"
#include "interface/parameters.hpp"
#include "interface/tagitem.hpp"
#include "tools/debug.hpp"
#include "std/assert.hpp"
///

/// ByteStream::Read
LONG ByteStream::Read(UBYTE *buffer,ULONG size)
{
  ULONG bytesread = 0;
  ULONG avail     = m_pucBufEnd - m_pucBufPtr; // number of available bytes

  while (size >= avail) { // more bytes to read than in the buffer

    assert(m_pucBufPtr <= m_pucBufEnd);
    
    if (avail) {
      assert(m_pucBufPtr >= buffer + avail || buffer >= m_pucBufPtr + avail);
      memcpy(buffer,m_pucBufPtr,avail); // copy all data over
      m_pucBufPtr  += avail;         // required for correct fill
      bytesread    += avail;
      buffer       += avail;
      size         -= avail;
    }

    // Now fill up the buffer again if we have to.
    if (size == 0 || Fill() == 0) { // read zero bytes -> EOF. Don't loop forever
      return bytesread;
    }
    //
    // Update the number of available bytes in the buffer.
    avail = m_pucBufEnd - m_pucBufPtr;
  }

  // only a partial read from the buffer
  // now is size <= avail, guaranteed.
  if (size) {
    assert(m_pucBufPtr >= buffer + avail || buffer >= m_pucBufPtr + avail);
    memcpy(buffer,m_pucBufPtr,size);
    m_pucBufPtr  += size;
    // buffer    += size;  // not needed
    // size      -= size;  // ditto
    bytesread    += size;
  }

  return bytesread;
}
///

/// ByteStream::Write
LONG ByteStream::Write(const UBYTE *buffer,ULONG size)
{
  ULONG byteswritten = 0;
  ULONG avail = m_pucBufEnd - m_pucBufPtr;

  while(size > avail) { // write more bytes than fit into the buffer? 
   
    assert(m_pucBufPtr <= m_pucBufEnd);
    
    // For JPIPs BinDefStream: Must not flush if the data fits, thus ">" and not ">=".
    if (avail) {
      memcpy(m_pucBufPtr,buffer,avail);  // copy the data over
      m_pucBufPtr  += avail;         // required for correct flush
      byteswritten += avail;
      buffer       += avail;
      size         -= avail;
    }
    // Now write the buffer (or allocate one, at least)
    Flush();
    //
    // Update the number of available bytes in the buffer.
    avail        = m_pucBufEnd - m_pucBufPtr;
  }

  // size is now smaller than m_ulBufBytes, guaranteed!
  if (size) {
    memcpy(m_pucBufPtr,buffer,size);
    m_pucBufPtr  += size;
    // buffer    += size;  // not needed
    // size      -= size;  // ditto
    byteswritten += size;
  }

  return byteswritten;
}
///

/// ByteStream::SkipToMarker
// Seek forwards to one of the supplied marker segments, but
// do not pull the marker segment itself.
// This method is required for error-resilliance features, namely
// to resynchronize.
// Returns the detected marker, or EOF.
LONG ByteStream::SkipToMarker(UWORD marker1,UWORD marker2,
                              UWORD marker3,UWORD marker4,
                              UWORD marker5)
{
  LONG byte;

  for(;;) {
    // Read bytes until we detect an 0xff which the marker has to start
    // with. Or an EOF, obviously.
    byte = Get();
    if (byte == EOF)
      return EOF; // We run out of data, *sigh*

    if (byte == 0xff) {
      // A possible marker segment? If so, put the 0xff back and
      // check for the available marker now.
      LastUnDo();
      // And now seek for the marker.
      byte = PeekWord();
      if ((byte == marker1) || (byte == marker2) || 
          (byte == marker3) || (byte == marker4) ||
          (byte == marker5))
        return byte;
      //
      // otherwise, not the marker we seek for. Skip, and don't forget
      // to pull the 0xff we put back.
      Get();
    }
  }
}
///

/// ByteStream::Push
LONG ByteStream::Push(class ByteStream *out,ULONG size)
{
  ULONG bytesread = 0;
  ULONG avail = m_pucBufEnd - m_pucBufPtr;

  while (size >= avail) { // more bytes to read than in the buffer
   
    assert(m_pucBufPtr <= m_pucBufEnd);
    
    if (avail) {
      out->Write(m_pucBufPtr,avail);    // write all data out
      m_pucBufPtr  += avail;           // required for correct fill
      bytesread    += avail;
      size         -= avail;
    }

    // Now fill up the buffer again.
    if (Fill() == 0) { // read zero bytes -> EOF. Don't loop forever
      return bytesread;
    } 
    //
    // Update the number of available bytes in the buffer.
    avail        = m_pucBufEnd - m_pucBufPtr;
  }

  // only a partial read from the buffer
  // now is size <= m_ulBufBytes, guaranteed.
  if (size) {
    out->Write(m_pucBufPtr,size);
    m_pucBufPtr  += size;
    // size      -= size;  // not needed
    bytesread    += size;
  }

  return bytesread;
}
///

/// ByteStream::Get
#if CHECK_LEVEL > 0
LONG ByteStream::Get(void)                          // read a single byte (not inlined)
{
 
  if (m_pucBufPtr >= m_pucBufEnd) {
    if (Fill() == 0)                    // Found EOF
      return EOF;
  }
  assert(m_pucBufPtr < m_pucBufEnd);
  
  return *m_pucBufPtr++;
}
#endif
///

/// ByteStream::SkipBytes
// Skip over bytes, ignore their contribution
void ByteStream::SkipBytes(ULONG offset)
{   
  ULONG avail = m_pucBufEnd - m_pucBufPtr;
  //
  // Cannot seek backwards here, a decoderstream disposes data
  // as soon as it is read (maybe).
  do {
    ULONG bufbytes;
    //
    assert(m_pucBufPtr <= m_pucBufEnd);
    //
    // If we have no buffer or the buffer is empty, try to seek over
    // or refill the buffer.
    if (avail == 0) {
      if (Fill() == 0 && offset) {
        // If this happens, and there's still something to skip,
        // then something's wrong because we should never seek 
        // over all data. If we do, the stream is most likely corrupt.
        JPG_THROW(UNEXPECTED_EOF,"ByteStream::SkipBytes","unexpectedly hit the end of the stream while skipping bytes");
      }
      // Update the number of available bytes.
      avail = m_pucBufEnd - m_pucBufPtr;
    }
    //
    // Abort now. The above condition is still taken since it
    // disposes unnecessary data early.
    if (offset == 0)
      return;
    //
    // Now some bytes should be available.
    assert(m_pucBufPtr < m_pucBufEnd);
    //
    // Check how many bytes we can skip.
    bufbytes = avail;
    if (bufbytes > offset)
      bufbytes = offset;
    //
    // Now skip the indicated number of bytes
    offset       -= bufbytes;
    m_pucBufPtr  += bufbytes;
    avail        -= bufbytes;
    //
    assert(m_pucBufPtr <= m_pucBufEnd);
  } while(true);
}
///
