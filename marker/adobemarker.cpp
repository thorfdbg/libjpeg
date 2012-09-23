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
** This class represents the adobe color specification marker, placed
** in APP14. Used here to indicate the color space and to avoid a color
** transformation.
**
** $Id: adobemarker.cpp,v 1.4 2012-09-11 14:32:14 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/adobemarker.hpp"
///

/// AdobeMarker::AdobeMarker
AdobeMarker::AdobeMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// AdobeMarker::~AdobeMarker
AdobeMarker::~AdobeMarker(void)
{
}
///

/// AdobeMarker::WriteMarker
// Write the marker to the stream.
void AdobeMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 5 + 2 + 2 + 2 + 1;
  const char *id = "Adobe";

  io->PutWord(len);

  // Identifier code: ASCII "Adobe"
  while(*id) {
    io->Put(*id);
    id++;
  }

  io->PutWord(100); // version
  io->PutWord(0);   // flags 0
  io->PutWord(0);   // flags 1
  io->Put(m_ucColorSpace);
}
///

/// AdobeMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void AdobeMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UWORD version;
  LONG color;

  if (len != 2 + 5 + 2 + 2 + 2 + 1)
    JPG_THROW(MALFORMED_STREAM,"AdobeMarker::ParseMarker","misformed Adobe marker");

  version = io->GetWord();
  if (version != 100) // Includes EOF
    JPG_THROW(MALFORMED_STREAM,"AdobeMarker::ParseMarker","Adobe marker version unrecognized");

  io->GetWord();
  io->GetWord(); // ignored.

  color  = io->Get();

  if (color < 0 || color > Last)
    JPG_THROW(MALFORMED_STREAM,"AdobeMarker::ParseMarker","Adobe color information unrecognized");

  m_ucColorSpace = color;
}
///
