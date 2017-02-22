/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** $Id: component.cpp,v 1.14 2014/09/30 08:33:17 thor Exp $
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
