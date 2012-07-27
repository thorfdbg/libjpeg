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
** This class contains the marker that defines the JPEG LS thresholds.
**
** $Id: thresholds.cpp,v 1.3 2012-07-16 16:40:56 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/thresholds.hpp"
#include "io/bytestream.hpp"
///

/// Thresholds::Thresholds
Thresholds::Thresholds(class Environ *env)
  : JKeeper(env), m_usMaxVal(255), 
    m_usT1(3), m_usT2(7), m_usT3(21), m_usReset(64)
{
}
///

/// Thresholds::~Thresholds
Thresholds::~Thresholds(void)
{
}
///

/// Thresholds::WriteMarker
// Write the marker contents to a LSE marker.
void Thresholds::WriteMarker(class ByteStream *io)
{
  io->PutWord(13); // length including the length byte
  io->Put(1);      // ID of the LSE marker
  io->PutWord(m_usMaxVal);
  io->PutWord(m_usT1);
  io->PutWord(m_usT2);
  io->PutWord(m_usT3);
  io->PutWord(m_usReset);
}
///

/// Thresholds::ParseMarker
// Parse the marker contents of a LSE marker.
void Thresholds::ParseMarker(class ByteStream *io,UWORD len)
{
  if (len != 13)
    JPG_THROW(MALFORMED_STREAM,"Thresholds::ParseMarker","LSE marker length is invalid");
  
  m_usMaxVal = io->GetWord();
  m_usT1     = io->GetWord();
  m_usT2     = io->GetWord();
  m_usT3     = io->GetWord();
  m_usReset  = io->GetWord();
}
///

/// Thresholds::InstallDefaults
// Install the defaults for a given bits per pixel value.
void Thresholds::InstallDefaults(UBYTE bpp,UWORD near)
{
  m_usMaxVal = (1 << bpp) - 1;
  
  if (m_usMaxVal >= 128) {
    UWORD factor = m_usMaxVal;
    if (factor > 4095)
      factor = 4095;
    factor = (factor + 128) >> 8;
    m_usT1 = factor * ( 3 - 2) + 2 + 3 * near;
    if (m_usT1 > m_usMaxVal || m_usT1 < near + 1) m_usT1 = near + 1;
    m_usT2 = factor * ( 7 - 3) + 3 + 5 * near;
    if (m_usT2 > m_usMaxVal || m_usT2 < m_usT1  ) m_usT2 = m_usT1;
    m_usT3 = factor * (21 - 4) + 4 + 7 * near;
    if (m_usT3 > m_usMaxVal || m_usT3 < m_usT2  ) m_usT3 = m_usT2;
  } else {
    UWORD factor = 256 / (m_usMaxVal + 1);
    m_usT1 = 3 / factor + 3 * near;
    if (m_usT1 < 2) m_usT1 = 2;
    if (m_usT1 > m_usMaxVal || m_usT1 < near + 1) m_usT1 = near + 1;
    m_usT2 = 7 / factor + 5 * near;
    if (m_usT2 < 3) m_usT2 = 3;
    if (m_usT2 > m_usMaxVal || m_usT2 < m_usT1  ) m_usT2 = m_usT1;
    m_usT3 = 21 / factor + 7 * near;
    if (m_usT3 < 4) m_usT3 = 4;
    if (m_usT3 > m_usMaxVal || m_usT3 < m_usT2  ) m_usT3 = m_usT2;
  }
  m_usReset  = 64;
}
///
