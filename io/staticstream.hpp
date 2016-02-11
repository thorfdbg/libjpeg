/*
** An implementation of the ByteStream that reads from or writes to
** a static buffer allocated outside of this class.
** $Id: staticstream.hpp,v 1.8 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef STATICSTREAM_HPP
#define STATICSTREAM_HPP

/// Includes
#include "bytestream.hpp"
///

/// class StaticStream
// This implementation of a ByteStream simply buffers
// the data it gets, but unlike the memory stream the
// data goes into a buffer that is administrated outside
// of this class, and has a finite length. If the
// stream tries to write beyond this buffer, an error
// is generated. If the caller tries to read beyond the
// end of the buffer, an EOF is generated.
class StaticStream : public ByteStream {
  //
public:
  //
  // Constructor: build a Static given a buffer and a buffer size.
  StaticStream(class Environ *env, UBYTE *buffer,ULONG bufsize)
    : ByteStream(env, bufsize)
  { 
    m_pucBuffer = buffer;
    m_pucBufPtr = buffer;
    m_pucBufEnd = buffer + bufsize;
  }
  //
  // Destructor: Get rid of the buffered bytes
  virtual ~StaticStream(void)
  {
  }
  //
  // Implementation of the abstract functions:
  virtual LONG Fill(void)
  {
    return 0; // always an EOF.
  }
  //
  virtual void Flush(void)
  {
    JPG_THROW(OVERFLOW_PARAMETER,"StaticStream::Flush","static memory buffer run over");
  }
  virtual LONG Query(void)
  {
    return 0;  // always success
  }
  //
  // Peek the next word in the stream, deliver the marker without
  // advancing the file pointer. Deliver EOF in case we run into
  // the end of the stream.
  virtual LONG PeekWord(void)
  {
    if (m_pucBufPtr + 1 < m_pucBufEnd) {
      return (m_pucBufPtr[0] << 8) | m_pucBufPtr[1];
    }
    return ByteStream::EOF;
  }
  //
};
///

///
#endif
