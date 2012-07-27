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
** This class parses the markers and holds the decoder together.
**
** $Id: decoder.cpp,v 1.15 2012-07-17 21:33:33 thor Exp $
**
*/

/// Include
#include "codestream/decoder.hpp"
#include "io/bytestream.hpp"
#include "std/assert.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "codestream/image.hpp"

///

/// Decoder::Decoder
// Construct the decoder
Decoder::Decoder(class Environ *env)
  : JKeeper(env), m_pImage(NULL), m_pTables(NULL)
{
}
///

/// Decoder::~Decoder
// Delete the decoder
Decoder::~Decoder(void)
{
  delete m_pImage;
  delete m_pTables;
}
///

/// Decoder::ParseHeader
// Parse off the header
class Image *Decoder::ParseHeader(class ByteStream *io)
{
  LONG marker;

  if (m_pImage || m_pTables)
    JPG_THROW(OBJECT_EXISTS,"Decoder::ParseHeader","stream parsing has already been started");
  
  marker = io->GetWord();
  if (marker != 0xffd8) // SOI
    JPG_THROW(MALFORMED_STREAM,"Decoder::ParseHeader","stream does not contain a JPEG file, SOI marker missing");

  m_pTables = new(m_pEnviron) class Tables(m_pEnviron);
  m_pTables->ParseTables(io);
  m_pImage  = new(m_pEnviron) class Image(m_pTables);
  //
  // The rest is done by the image.
  return m_pImage;
}
///

/// Decoder::ParseTags
// Accept decoder options.
void Decoder::ParseTags(const struct JPG_TagItem *tags)
{
  if (tags->GetTagData(JPGTAG_DECODER_FORCEINTEGERDCT)) {
    if (m_pTables)
      m_pTables->ForceIntegerCodec();
  }
  if (tags->GetTagData(JPGTAG_DECODER_FORCEFIXPOINTDCT)) {
    m_pTables->ForceFixpointCodec();
  }
  if (tags->GetTagData(JPGTAG_IMAGE_COLORTRANSFORMATION,true) == false) {
    m_pTables->ForceColorTrafoOff();
  }
}
///
