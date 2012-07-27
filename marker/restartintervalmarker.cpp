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
** This class keeps the restart interval size in MCUs.
**
** $Id: restartintervalmarker.cpp,v 1.2 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/restartintervalmarker.hpp"
///

/// RestartIntervalMarker::RestartIntervalMarker
RestartIntervalMarker::RestartIntervalMarker(class Environ *env)
  : JKeeper(env), m_usRestartInterval(0)
{
}
///

/// RestartIntervalMarker::WriteMarker
// Write the marker (without the marker id) to the stream.
void RestartIntervalMarker::WriteMarker(class ByteStream *io) const
{
  io->PutWord(0x04); // size of the marker.
  io->PutWord(m_usRestartInterval);
}
///

/// RestartIntervalMarker::ParseMarker
// Parse the marker from the stream.
void RestartIntervalMarker::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len != 4)
    JPG_THROW(MALFORMED_STREAM,"RestartIntervalMarker::ParseMarker",
	      "DRI restart interval definition marker size is invalid");

  len = io->GetWord();
  if (len == ByteStream::EOF)
    JPG_THROW(UNEXPECTED_EOF,"RestartIntervalMarker::ParseMarker",
	      "DRI restart interval definition marker run out of data");

  m_usRestartInterval = UWORD(len);
}
///

