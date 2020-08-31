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
