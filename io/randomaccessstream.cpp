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
** A random access IO stream that allows forwards and backwards
** seeking. This is an abstraction of the known "IOHook".
**
** $Id: randomaccessstream.cpp,v 1.3 2012-09-09 15:53:51 thor Exp $
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
