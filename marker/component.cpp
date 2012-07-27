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
** This class represents a single component.
**
** $Id: component.cpp,v 1.9 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "marker/component.hpp"
#include "io/bytestream.hpp"
///

/// Component::Component
Component::Component(class Environ *env,UBYTE idx,UBYTE prec,UBYTE subx,UBYTE suby)
  : JKeeper(env), m_ucIndex(idx), m_ucID(idx), m_ucSubX(subx), m_ucSubY(suby), m_ucPrecision(prec)
{
}
///

/// Component::~Component
Component::~Component(void)
{
}
///

/// Component::WriteMarker
// Write the component information to the bytestream.
void Component::WriteMarker(class ByteStream *io)
{
  // Write the component identifier.
  io->Put(m_ucID);

  // Write the dimensions of the MCU
  assert(m_ucMCUWidth  < 16);
  assert(m_ucMCUHeight < 16);

  io->Put((m_ucMCUWidth << 4) | m_ucMCUHeight);

  // Write the quantization table index.
  io->Put(m_ucQuantTable);
}
///

/// Component::ParseMarker
void Component::ParseMarker(class ByteStream *io)
{
  LONG data;

  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","frame marker incomplete, no component identifier found");

  m_ucID = data;

  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","frame marker incomplete, subsamling information missing");

  m_ucMCUWidth  = data >> 4;
  m_ucMCUHeight = data & 0x0f;

  data = io->Get();
  if (data < 0 || data > 3)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","quantization table identifier corrupt, must be >= 0 and <= 3");

  m_ucQuantTable = data;
}
///
