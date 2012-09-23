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
** This class represents the APP4 marker carrying residual data
** information to make JPEG lossless or to support high-bitrange
** without loosing compatibility.
**
** $Id: residualmarker.cpp,v 1.12 2012-09-23 12:58:39 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/residualmarker.hpp"
#include "io/memorystream.hpp"
///

/// ResidualMarker::ResidualMarker
ResidualMarker::ResidualMarker(class Environ *env,MarkerType type)
  : JKeeper(env), m_pBuffer(NULL), m_pReadBack(NULL),
    m_Type(type)
{
}
///

/// ResidualMarker::~ResidualMarker
ResidualMarker::~ResidualMarker(void)
{
  delete m_pBuffer;
  delete m_pReadBack;
}
///

/// ResidualMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void ResidualMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  if (len < 2 + 4 + 2) // Marker length, identifier, plus error
    JPG_THROW(MALFORMED_STREAM,"ResidualMarker::ParseMarker",
	      "APP9 residual data marker size too short");

  if (m_pBuffer == NULL)
    m_pBuffer = new(m_pEnviron) class MemoryStream(m_pEnviron);

  len -= 2 + 4 + 2;
  m_pBuffer->Append(io,len);
}
///

/// ResidualMarker::StreamOf
class ByteStream *ResidualMarker::StreamOf(void)
{
  if (m_pBuffer) {
    if (m_pReadBack == NULL)
      m_pReadBack = new(m_pEnviron) class MemoryStream(m_pEnviron,m_pBuffer,JPGFLAG_OFFSET_BEGINNING);
    return m_pReadBack;
  }

  return NULL;
}
///

/// ResidualMarker::WriteMarker
// Write the marker as hidden refinement scan marker.
void ResidualMarker::WriteMarker(class ByteStream *target,class MemoryStream *src)
{ 
  class MemoryStream readback(m_pEnviron,src,JPGFLAG_OFFSET_BEGINNING);

  ULONG bytes = readback.BufferedBytes();

  while(bytes) {
    ULONG write     = bytes;
    UWORD extradata = 2 + 4 + 2;

    if (write > ULONG(MAX_UWORD - extradata)) {
      write = MAX_UWORD - extradata;
    }
    target->PutWord(0xffe9); // Write an APP9.
    target->PutWord(write + extradata);
    // Write the ID
    target->Put('J');
    target->Put('P');
    switch(m_Type) {
    case Refinement:
      target->Put('F');
      target->Put('I');
      target->Put('N');
      target->Put('E');
      break;
    case Residual:
      target->Put('R');
      target->Put('E');
      target->Put('S');
      target->Put('I');
      break;
    }

    readback.Push(target,write);
    bytes -= write;
  }
}
///
