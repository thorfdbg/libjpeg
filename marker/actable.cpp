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
** This class contains and maintains the AC conditioning
** parameter templates.
**
** $Id: actable.cpp,v 1.4 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "marker/actable.hpp"
#include "io/bytestream.hpp"
#include "coding/actemplate.hpp"
#include "std/string.hpp"
///

/// ACTable::ACTable
ACTable::ACTable(class Environ *env)
  : JKeeper(env)
{
  memset(m_pParameters,0,sizeof(m_pParameters));
}
///

/// ACTable::~ACTable
ACTable::~ACTable(void)
{
  int i;

  for(i = 0;i < 8;i++) {
    delete m_pParameters[i];
  }
}
///

/// ACTable::WriteMarker
// Write the currently defined huffman tables back to a stream.
void ACTable::WriteMarker(class ByteStream *io)
{
  int i = 0;
  ULONG len = 2; // marker size itself.

  for(i = 0;i < 8;i++) {
    if (m_pParameters[i]) {
      len += 2; 
    }
  }

  if (len > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"ACTable::WriteMarker","DAC marker overhead too large");

  io->PutWord(len);
  
  for(i = 0;i < 8;i++) {
    if (m_pParameters[i]) {
      if (i >= 4) {
	io->Put(0x10 | (i & 0x03)); // AC table.
	io->Put(m_pParameters[i]->BandDiscriminatorOf()); 
      } else {
	io->Put(i & 0x03); // DC table.
	io->Put((m_pParameters[i]->UpperThresholdOf() << 4) | m_pParameters[i]->LowerThresholdOf());
      }
    }
  }
}
///

/// ACTable::ParseMarker
// Parse the marker contents of a DHT marker.
void ACTable::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len < 2)
    JPG_THROW(MALFORMED_STREAM,"ACTable::ParseMarker","AC conditioning table length must be at least two bytes long");

  len -= 2; // remove the marker length.

  while(len > 0) {
    LONG t = io->Get();
    
    if (t == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"ACTable::ParseMarker","AC conditioning table marker run out of data");
    len--;
    
    if ((t >> 4) > 1) {
      JPG_THROW(MALFORMED_STREAM,"ACTable::ParseMarker","undefined conditioning table type");
      return;
    }
    t = (t & 0x03) | ((t & 0xf0) >> 2);
    delete m_pParameters[t];m_pParameters[t] = NULL;
    m_pParameters[t] = new(m_pEnviron) ACTemplate(m_pEnviron);
    if (t >= 4)
      m_pParameters[t]->ParseACMarker(io);
    else
      m_pParameters[t]->ParseDCMarker(io);

    len--;
  }
}
///

/// ACTable::DCTemplateOf
// Get the template for the indicated DC table or NULL if it doesn't exist.
class ACTemplate *ACTable::DCTemplateOf(UBYTE idx)
{
  assert(m_pParameters && idx < 4);
    
  if (m_pParameters[idx] == NULL) {
    m_pParameters[idx] = new(m_pEnviron) class ACTemplate(m_pEnviron);
    m_pParameters[idx]->InitDefaults();
  }

  return m_pParameters[idx];
}
///

/// ACTable::ACTemplateOf
// Get the template for the indicated AC table or NULL if it doesn't exist.
class ACTemplate *ACTable::ACTemplateOf(UBYTE idx)
{
  assert(m_pParameters && idx < 4);
    
  idx += 4;

  if (m_pParameters[idx] == NULL) {
    m_pParameters[idx] = new(m_pEnviron) class ACTemplate(m_pEnviron);
    m_pParameters[idx]->InitDefaults();
  }

  return m_pParameters[idx];
}
///
