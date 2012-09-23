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
** $Id: residualspecsmarker.cpp,v 1.3 2012-09-22 11:16:14 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/residualspecsmarker.hpp"
#include "io/memorystream.hpp"
///

/// ResidualSpecsMarker::ResidualSpecsMarker
ResidualSpecsMarker::ResidualSpecsMarker(class Environ *env)
  : JKeeper(env), m_ucQuantization(0), m_ucPreshift(0), m_ucToneEnable(0), m_ucHiddenBits(0)
{
}
///

/// ResidualSpecsMarker::~ResidualSpecsMarker
ResidualSpecsMarker::~ResidualSpecsMarker(void)
{
}
///

/// ResidualSpecsMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void ResidualSpecsMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UWORD extra = 0;
  if (len < 2 + 4 + 1 + 2 + 1 + 1) // Marker length, identifier, plus error
    JPG_THROW(MALFORMED_STREAM,"ResidualSpecsMarker::ParseMarker","APP9 residual information marker size too short");

  m_ucPreshift     = io->Get();
  m_ucQuantization = io->Get();
  m_ucHiddenBits   = io->Get();

  m_ucToneEnable = m_ucPreshift >> 4;
  m_ucPreshift  &= 0x0f;
  
  if (m_ucToneEnable & 0x01)
    extra++;
  if (m_ucToneEnable & 0x02)
    extra++;
  if (m_ucToneEnable & 0x04)
    extra++;
  if (m_ucToneEnable & 0x08)
    extra++;
  if (len < extra)
    JPG_THROW(MALFORMED_STREAM,"ResidualSpecsMarker::ParseMarker",
	      "APP9 residual specifications marker size too short");

  if (m_ucToneEnable & 0x01)
    m_ucToneMapping[0] = io->Get();
  if (m_ucToneEnable & 0x02)
    m_ucToneMapping[1] = io->Get();
  if (m_ucToneEnable & 0x04)
    m_ucToneMapping[2] = io->Get();
  if (m_ucToneEnable & 0x08)
    m_ucToneMapping[3] = io->Get();

  len -= 2 + 4 + 1 + 2 + 1 + 1 + extra;

  if (len != 0)
    JPG_THROW(MALFORMED_STREAM,"ResidualSpecsMarker::ParseMarker",
	      "APP9 residual specifications marker size invalid");

  if (m_ucHiddenBits > 14)
    JPG_THROW(MALFORMED_STREAM,"ResidualSpecsMarker::ParseMarker",
	      "APP9 residual specifications marker number of hidden bits is > 14");
}
///

/// ResidualSpecsMarker::WriteMarker
// Write the marker, where the raw data comes buffered from the
// indicated memory stream.
void ResidualSpecsMarker::WriteMarker(class ByteStream *target)
{ 
  UWORD extradata = 2 + 4 + 1 + 2 + 1 + 1;

  if (m_ucToneEnable & 0x01)
    extradata++;
  if (m_ucToneEnable & 0x02)
    extradata++;
  if (m_ucToneEnable & 0x04)
    extradata++;
  if (m_ucToneEnable & 0x08)
    extradata++;
  
  target->PutWord(extradata);
  // Write the ID
  target->Put('J');
  target->Put('P');
  target->Put('S');
  target->Put('P');
  target->Put('E');
  target->Put('C');
  target->Put(m_ucPreshift | (m_ucToneEnable << 4));
  target->Put(m_ucQuantization);
  target->Put(m_ucHiddenBits);
  
  if (m_ucToneEnable & 0x01)
    target->Put(m_ucToneMapping[0]);
  if (m_ucToneEnable & 0x02)
    target->Put(m_ucToneMapping[1]);
  if (m_ucToneEnable & 0x04)
    target->Put(m_ucToneMapping[2]);
  if (m_ucToneEnable & 0x08)
    target->Put(m_ucToneMapping[3]);
}
///

/// ResidualSpecsMarker::InstallPreshift
// Install parameters - here only the maximum coding error.
void ResidualSpecsMarker::InstallPreshift(UBYTE preshift)
{
  m_ucPreshift = preshift;
}
///

/// ResidualSpecsMarker::InstallHiddenBits
// Install the number of hidden DCT bits.
void ResidualSpecsMarker::InstallHiddenBits(UBYTE hidden)
{
  m_ucHiddenBits = hidden;
}
///
