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
 * $Id: bytestream.hpp,v 1.11 2015/03/30 09:39:09 thor Exp $
 *
 */

#ifndef BYTESTREAM_HPP
#define BYTESTREAM_HPP

/// Includes
#include "tools/environment.hpp"
#include "interface/parameters.hpp"
#include "interface/hooks.hpp"
#include "std/string.hpp"
#ifdef EOF
#undef EOF
#endif
///

/// Design
/** Design
******************************************************************
** class ByteStream                                             **
** Super Class: none                                            **
** Sub Classes: IOHook, MemoryStream, DecoderStream, NULStream  **
** Friends:     none                                            **
******************************************************************

The ByteStream is an abstract class that implements byte oriented
input/output functions. This includes reading/writing a block
of data, reading and writing of single bytes and words and some 
status and special operations.

The ByteStream class already implements buffering, and contains
also a file/byte counter. What the corresponding sub classes have
to do is just to implement methods for reading and writing complete
buffers, and to deliver status information.

One special method, PeekWord(), allows to read the next marker
(two bytes) without advancing the file pointer. This is required 
for some of the higher magic of the error resiliance features.
* */
///

/// ByteStream
// The ByteStream class is an abstract data class to read or write groups
// or blocks of bytes from an abstract stream, additionally providing some
// kind of buffering mechanism. It implements non-virtual member functions
// that operate on the buffer only, and some virtual member functions to
// read or write complete buffers.
class ByteStream : public JKeeper {
  // As sad as it sounds, but the segmented stream
  // must grab into the following to steal buffers.
  friend class ChecksumAdapter;
  //
protected:
  ULONG       m_ulBufSize;   // Size of our (internal) IO buffer
  UBYTE      *m_pucBuffer;   // an IO buffer if we have it
  UBYTE      *m_pucBufPtr;   // a pointer to the first valid buffer byte
  UBYTE      *m_pucBufEnd;   // pointer beyond the last valid byte of the buffer.
  UQUAD       m_uqCounter;   // counts output bytes, if possible
  //
  // Note: The counter and the buffers must be maintained by instances of
  // this abstract class.
  //
  // constructors: This just fills in the buffer size and resets the pointers
  //
  ByteStream(class Environ *env, ULONG bufsize = 2048)
    : JKeeper(env),
      m_ulBufSize(bufsize), m_pucBuffer(NULL), m_pucBufPtr(NULL), m_pucBufEnd(NULL), m_uqCounter(0)
  { }
  //
  //
  //
  // fill up the buffer and flush it.
  // these two have to be replaced by the corresponding
  // member functions of the inherited classses
  virtual LONG Fill(void) = 0; 
  // Flush the IO buffer. This must be defined by the instances of
  // this class.
  virtual void Flush(void) = 0;
  //
#if CHECK_LEVEL > 2
  static UBYTE lastbyte;
#endif
  //
public: 
  //
  // A couple of definitions
  enum {
    EOF   = -1 // indication of an EOF condition
  };
  //
  // the destructor is virtual and has to be
  // overloaded.
  virtual ~ByteStream(void)
  { }
  //
  // Some rather standard IO functions, you know what they do.
  // These are not for overloading and non-virtual. All they
  // need is the buffer structure above. 
  LONG Read(UBYTE *buffer,ULONG size);          // read from buffer
  LONG Write(const UBYTE *buffer,ULONG size);   // write to buffer
  // Push the (partial) contents of another bytestream out.
  LONG Push(class ByteStream *out,ULONG size);
  // 
  // read stream buffer status. Also to be overloaded.
  virtual LONG Query(void) = 0;
  //
  // Reset the byte counter. This *MUST* be matched by a flush or a refill
  // or otherwise the result is undesirable.
  void ResetCounter(void)
  {
    m_uqCounter = 0;
  }
  //
  // Peek the next word in the stream, deliver the marker without
  // advancing the file pointer. Deliver EOF in case we run into
  // the end of the stream.
  virtual LONG PeekWord(void) = 0;  
  //
  // Skip over bytes, ignore their contribution. The offset must
  // be positive (or zero).
  virtual void SkipBytes(ULONG offset);
  //
  //
#if CHECK_LEVEL > 0
  LONG Get(void);
#else
  // The following two methods are single byte IO functions,
  // inlined for maximal performance.
  LONG Get(void)                          // read a single byte (inlined)
  {
    if (unlikely(m_pucBufPtr >= m_pucBufEnd)) {
      if (Fill() == 0)                    // Found EOF
        return EOF;
    }
    return *m_pucBufPtr++;
  }
#endif
  //
  // Read a word from the stream. Return EOF if not available.
  LONG GetWord(void)
  {
    LONG in1,in2;
    //
    in1 = Get();
    if (in1 == EOF)
      return EOF;
    in2 = Get();
    if (in2 == EOF)
      return EOF;
    //
    // The FDIS enforces BIG ENDIAN, so do we.
    return (in1 << 8) | in2;
  }
  //
  // Just the same for writing data.
  void Put(UBYTE byte)           // write a single byte (inlined)
  {
    if (unlikely(m_pucBufPtr >= m_pucBufEnd)) {
      Flush();                   // note that this will also allocate a buffer
    }

    *m_pucBufPtr++ = byte;
  }
  //
  // Put a WORD over the stream.
  void PutWord(UWORD word)
  {
    // Use big-endianness, as in the FDIS.
    Put(word >> 8);
    Put(word & 0xff);
  }
  //
  // Return the last byte that has been read from or put into the buffer.
  // If the last byte is not available, return EOF.
  // Note that the ArthDeco/MQ coder requires this behaivour as in this
  // case it can know that the last byte was at least not a 0xff which
  // we removed then already.
  LONG LastByte(void) const
  {
    if (m_pucBufPtr <= m_pucBuffer)
      return EOF;

    return m_pucBufPtr[-1];
  }
  //
  // Return the last byte written/read and un-put/get it.
  UBYTE LastUnDo(void)
  {
    // Actually, this may be valid in case we want to un-do an EOF
    // read. Hence, this is only an error if there was a buffer before.
    assert(m_pucBufPtr == NULL || m_pucBufPtr > m_pucBuffer);
    
    if (m_pucBufPtr > m_pucBuffer) {
      m_pucBufPtr--;
      return *m_pucBufPtr;
    }
    
    return 0; // shut-up g++
  }
  //
  // Return the byte counter = #of bytes read or written
  UQUAD FilePosition(void) const
  {
    if (m_pucBuffer) {
      return m_uqCounter + (m_pucBufPtr - m_pucBuffer);
    } else {
      return m_uqCounter;
    }
  }
  //
  // Seek forwards to one of the supplied marker segments, but
  // do not pull the marker segment itself.
  // This method is required for error-resilliance features, namely
  // to resynchronize.
  // Returns the detected marker, or EOF.
  LONG SkipToMarker(UWORD marker1,UWORD marker2 = 0,
                    UWORD marker3 = 0,UWORD marker4 = 0,
                    UWORD marker5 = 0);
};
///

///
#endif
