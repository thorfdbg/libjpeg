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
** An implementation of the ByteStream that reads from or writes to
** a static buffer allocated outside of this class.
** $Id: staticstream.hpp,v 1.3 2012-09-09 15:53:51 thor Exp $
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
