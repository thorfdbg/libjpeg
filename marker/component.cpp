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
** This class represents a single component.
**
** $Id: component.cpp,v 1.15 2020/08/31 07:50:44 thor Exp $
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

  if (m_ucMCUWidth == 0 || m_ucMCUHeight == 0)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","frame marker corrupt, MCU size cannot be 0");
  
  data = io->Get();
  if (data < 0 || data > 3)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","quantization table identifier corrupt, must be >= 0 and <= 3");

  m_ucQuantTable = data;
}
///
