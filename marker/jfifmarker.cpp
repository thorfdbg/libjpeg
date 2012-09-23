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
** This class represents the JFIF, placed in APP0. 
** This is only used to indicate a JFIF file and is otherwise unused.
**
** $Id: jfifmarker.cpp,v 1.2 2012-09-11 14:32:14 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/jfifmarker.hpp"
///

/// JFIFMarker::JFIFMarker
JFIFMarker::JFIFMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// JFIFMarker::~JFIFMarker
JFIFMarker::~JFIFMarker(void)
{
}
///

/// JFIFMarker::WriteMarker
// Write the marker to the stream.
void JFIFMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1;
  const char *id = "JFIF";

  io->PutWord(len);

  // Identifier code: ASCII "JFIF"
  while(*id) {
    io->Put(*id);
    id++;
  }
  io->Put(0); // This comes with a terminating zero.

  // Version is 1.02.
  io->Put(1);
  io->Put(2);
  //
  io->Put(m_Unit);
  io->PutWord(m_usXRes);
  io->PutWord(m_usYRes);

  // Thumbnail size: No thumbnail
  io->Put(0);
  io->Put(0);
}
///

/// JFIFMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void JFIFMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UBYTE unit;
  
  if (len < 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1)
    JPG_THROW(MALFORMED_STREAM,"JFIFMarker::ParseMarker","misformed JFIF marker");

  io->Get(); // version
  io->Get(); // revision
  //
  // Currently ignore.

  unit     = io->Get();
  if (unit > Centimeter)
    JPG_THROW(MALFORMED_STREAM,"JFIFMarker::ParseMarker","JFIF specified unit is invalid");

  m_Unit   = (ResolutionUnit)unit;
  //
  // Read the dimensions.
  m_usXRes = io->GetWord();
  m_usYRes = io->GetWord();

  len     -= 2 + 5 + 2 + 1 + 2 + 2;
  // Skip the rest of the marker.
  if (len > 0)
    io->SkipBytes(len);
}
///
