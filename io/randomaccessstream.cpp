/*
**
** A random access IO stream that allows forwards and backwards
** seeking. This is an abstraction of the known "IOHook".
**
** $Id: randomaccessstream.cpp,v 1.8 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "randomaccessstream.hpp"
///

/// RandomAccessStream::PeekMarker
// Peek the next word in the stream, deliver the marker without
// advancing the file pointer. Deliver EOF in case we run into
// the end of the stream.
LONG RandomAccessStream::PeekWord(void)
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
	// Otherwise, we're in a mess. We allocated the buffer one
	// byte larger than necessary, so we can move the buffer
	// contents up by one byte and place the first byte at the
	// beginning of the buffer. *Yuck*
	memmove(m_pucBuffer+1,m_pucBuffer,m_pucBufEnd - m_pucBuffer);
	m_pucBuffer[0] = (UBYTE) byte1;
	m_pucBufEnd++;
	m_uqCounter--;
      }
      // Deliver the result.
      return ((byte1<<8) | byte2);
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
