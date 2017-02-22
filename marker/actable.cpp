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
** This class contains and maintains the AC conditioning
** parameter templates.
**
** $Id: actable.cpp,v 1.11 2015/03/25 08:45:43 thor Exp $
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
#if ACCUSOFT_CODE
  memset(m_pParameters,0,sizeof(m_pParameters));
#endif
}
///

/// ACTable::~ACTable
ACTable::~ACTable(void)
{
#if ACCUSOFT_CODE
  int i;

  for(i = 0;i < 8;i++) {
    delete m_pParameters[i];
  }
#endif
}
///

/// ACTable::WriteMarker
// Write the currently defined huffman tables back to a stream.
void ACTable::WriteMarker(class ByteStream *io)
{
#if ACCUSOFT_CODE
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
#else
  NOREF(io);
  JPG_THROW(NOT_IMPLEMENTED,"ACTable::WriteMarker",
            "Arithmetic coding option not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// ACTable::ParseMarker
// Parse the marker contents of a DHT marker.
void ACTable::ParseMarker(class ByteStream *io)
{
#if ACCUSOFT_CODE
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
#else 
  NOREF(io);
  JPG_THROW(NOT_IMPLEMENTED,"ACTable::WriteMarker",
            "Arithmetic coding option not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// ACTable::DCTemplateOf
// Get the template for the indicated DC table or NULL if it doesn't exist.
class ACTemplate *ACTable::DCTemplateOf(UBYTE idx)
{
#if ACCUSOFT_CODE
  assert(m_pParameters && idx < 4);
    
  if (m_pParameters[idx] == NULL) {
    m_pParameters[idx] = new(m_pEnviron) class ACTemplate(m_pEnviron);
    m_pParameters[idx]->InitDefaults();
  }

  return m_pParameters[idx];
#else
  NOREF(idx);
  return NULL;
#endif
}
///

/// ACTable::ACTemplateOf
// Get the template for the indicated AC table or NULL if it doesn't exist.
class ACTemplate *ACTable::ACTemplateOf(UBYTE idx)
{
#if ACCUSOFT_CODE
  assert(m_pParameters && idx < 4);
    
  idx += 4;

  if (m_pParameters[idx] == NULL) {
    m_pParameters[idx] = new(m_pEnviron) class ACTemplate(m_pEnviron);
    m_pParameters[idx]->InitDefaults();
  }

  return m_pParameters[idx];
#else
  NOREF(idx);
  return NULL;
#endif
}
///
