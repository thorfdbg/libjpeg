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
** This class contains and maintains the huffman code parsers.
**
** $Id: huffmantable.cpp,v 1.10 2012-10-07 15:58:08 thor Exp $
**
*/

/// Includes
#include "marker/huffmantable.hpp"
#include "io/bytestream.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/arithmetictemplate.hpp"
#include "std/string.hpp"
///

/// HuffmanTable::HuffmanTable
HuffmanTable::HuffmanTable(class Environ *env)
  : JKeeper(env)
{
  memset(m_pCoder,0,sizeof(m_pCoder));
}
///

/// HuffmanTable::~HuffmanTable
HuffmanTable::~HuffmanTable(void)
{
  int i;

  for(i = 0;i < 8;i++) {
    delete m_pCoder[i];
  }
}
///

/// HuffmanTable::WriteMarker
// Write the currently defined huffman tables back to a stream.
void HuffmanTable::WriteMarker(class ByteStream *io)
{
  int i = 0;
  ULONG len = 2; // marker size itself.

  for(i = 0;i < 8;i++) {
    if (m_pCoder[i]) {
      len += 1; // Th and Tc field.
      len += m_pCoder[i]->MarkerOverhead();
    }
  }

  if (len > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"HuffmanTable::WriteMarker","DHT marker overhead too large, Huffman tables too complex");

  io->PutWord(len);
  
  for(i = 0;i < 8;i++) {
    if (m_pCoder[i]) {
      UBYTE type = 0;
      if (i >= 4)
	type |= 0x10;    // is an AC table then.
      type |= i & 0x03;  // the huffman table identifier.
      io->Put(type);
      m_pCoder[i]->WriteMarker(io);
    }
  }
}
///  

/// HuffmanTable::ParseMarker
// Parse the marker contents of a DHT marker.
void HuffmanTable::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len < 2)
    JPG_THROW(MALFORMED_STREAM,"HuffmanTable::ParseMarker","huffman table length must be at least two bytes long");

  len -= 2; // remove the marker length.

  while(len > 0) {
    LONG  t = io->Get();
    UQUAD p = io->FilePosition();
    UQUAD q;
    
    if (t == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"HuffmanTable::ParseMarker","huffman table marker run out of data");
    len--;
    
    if ((t >> 4) > 1) {
      JPG_THROW(MALFORMED_STREAM,"HuffmanTable::ParseMarker","undefined huffman table type");
      return;
    }
    t = (t & 0x03) | ((t & 0xf0) >> 2);
    delete m_pCoder[t];m_pCoder[t] = NULL;
    m_pCoder[t] = new(m_pEnviron) HuffmanTemplate(m_pEnviron);
    m_pCoder[t]->ParseMarker(io);
    
    q = io->FilePosition();
    assert(q >= p);
    q -= p;

    if (q > ULONG(len))
      JPG_THROW(MALFORMED_STREAM,"HuffmanTable::ParseMarker","huffman table size corrupt");
    len -= q;
  }

  if (len)
    JPG_THROW(MALFORMED_STREAM,"HuffmanTable::ParseMarker","huffman table size is corrupt");
}
///

/// HuffmanTable::AdjustToStatistics
// Adjust all coders in here to the statistics collected before, i.e.
// find optimal codes.
void HuffmanTable::AdjustToStatistics(void)
{
  for(int i = 0;i < 8;i++) {
    if (m_pCoder[i])
      m_pCoder[i]->AdjustToStatistics();
  }
}
///

/// HuffmanTable::DCTemplateOf
// Get the template for the indicated DC table or NULL if it doesn't exist.
class HuffmanTemplate *HuffmanTable::DCTemplateOf(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden,bool residual)
{
  assert(m_pCoder && idx < 4);
  
  if (m_pCoder[idx] == NULL) {
    m_pCoder[idx] = new(m_pEnviron) class HuffmanTemplate(m_pEnviron);
    // Provide a default that seems sensible. Everything else requires
    // measurement.
    if (idx == 0) {
      m_pCoder[idx]->InitDCLuminanceDefault(type,depth,hidden,residual);
    } else {
      m_pCoder[idx]->InitDCChrominanceDefault(type,depth,hidden,residual);
    }
  }
  
  return m_pCoder[idx];
}
///

/// HuffmanTable::ACTemplateOf
// Get the template for the indicated AC table or NULL if it doesn't exist.
class HuffmanTemplate *HuffmanTable::ACTemplateOf(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden,bool residual)
{
  assert(m_pCoder && idx < 4);

  idx += 4;
  
  if (m_pCoder[idx] == NULL) {
    m_pCoder[idx] = new(m_pEnviron) class HuffmanTemplate(m_pEnviron);
    // Provide a default that seems sensible. Everything else requires
    // measurement.
    if (idx == 0) {
      m_pCoder[idx]->InitACLuminanceDefault(type,depth,hidden,residual);
    } else {
      m_pCoder[idx]->InitACChrominanceDefault(type,depth,hidden,residual);
    }
  }
  
  return m_pCoder[idx];
}
///
