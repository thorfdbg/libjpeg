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
** This class represents the quantization tables.
**
** $Id: quantization.cpp,v 1.19 2012-11-09 13:09:16 thor Exp $
**
*/

/// Includes
#include "marker/quantization.hpp"
#include "io/bytestream.hpp"
#include "dct/dct.hpp"
///

/// Quantization::Quantization
Quantization::Quantization(class Environ *env)
  : JKeeper(env)
{
  memset(m_pDelta,0,sizeof(m_pDelta));
}
///

/// Quantization::~Quantization
Quantization::~Quantization(void)
{
  int i;

  for(i = 0;i < 4;i++) {
    if (m_pDelta[i])
      m_pEnviron->FreeMem(m_pDelta[i],sizeof(UWORD) * 64);
  }
}
///


/// Quantization::WriteMarker
// Write the DQT marker to the stream.
void Quantization::WriteMarker(class ByteStream *io)
{
  int i,j;
  UWORD len = 2; // The marker size
  UBYTE types[4];

  for(i = 0;i < 4;i++) {
    types[i] = 0;

    if (m_pDelta[i]) {
      for(j = 0;j < 64;j++) {
	if (m_pDelta[i][j] > 255) {
	  types[i] = 1;
	  len     += 64;
	  break;
	}
      }
      len += 64 + 1;
    }
  }
  
  io->PutWord(len);
  for(i = 0;i < 4;i++) {
    if (m_pDelta[i]) {
      io->Put((types[i] << 4) | i);
      if (types[i]) {
	for(j = 0;j < 64;j++) {
	  io->PutWord(m_pDelta[i][DCT::ScanOrder[j]]);
	}
      } else {
	for(j = 0;j < 64;j++) {
	  io->Put(m_pDelta[i][DCT::ScanOrder[j]]);
	}
      }
    }
  }
}
///

/// Quantization::InitDefaultTables
// Initialize the quantization table to the standard example
// tables for quality q, q=0...100
void Quantization::InitDefaultTables(UBYTE quality,UBYTE hdrquality,bool colortrafo)
{
  int i,j;
  const int *table = NULL;
  int scale;
  int hdrscale;

  static const int std_luminance_quant_tbl[64] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
  };
  static const int std_chrominance_quant_tbl[64] = {
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
  };

  if (quality < 1)
    quality = 1;
  if (quality > 100)
    quality = 100;

  if (quality < 50)
    scale = 5000 / quality;
  else
    scale = 200 - quality * 2;

  if (hdrquality == 0) {
    hdrscale = MAX_UWORD;
  } else if (hdrquality < 50) {
    hdrscale = 5000 / hdrquality;
  } else if (hdrquality <= 100) {
    hdrscale = 200 - hdrquality * 2;
  } else {
    hdrscale = -1;
  }
    
  for(i = 0;i < 4;i++) {
    switch(i) {
    case 0:
      table = std_luminance_quant_tbl;
      break;
    case 1:
      if (colortrafo) {
	table = std_chrominance_quant_tbl;
      } else {
	table = NULL;
      }
      break;
    case 2:
      if (hdrscale >= 0)
	table = std_luminance_quant_tbl;
      else
	table = NULL;
      break;
    case 3:
      if (hdrscale >= 0 && colortrafo) 
	table = std_chrominance_quant_tbl;
      else
	table = NULL;
    }
    if (table) {
      if (m_pDelta[i] == NULL)
	m_pDelta[i] = (UWORD *)m_pEnviron->AllocMem(sizeof(UWORD) * 64);
      for(j = 0;j < 64;j++) {
	int mult  = (i >= 2)?(hdrscale):(scale);
	int delta = (table[j] * mult + 50) / 100;
#if 0
	if ((j & 7) + (j >> 3) > 3) {
	  delta = (table[j] * mult + 50) / 100;
	} else if ((j & 7) + (j >> 3) > 2) {
	  int sc = (mult > 400)?(400):mult;
	  delta  = (table[j] * sc + 50) / 100;
	} else if ((j & 7) + (j >> 3) >= 1) {
	  int sc = (mult > 200)?(200):mult;
	  delta  = (table[j] * sc + 50) / 100;
	} else { 
	  int sc = (mult > 100)?(100):mult;
	  delta  = (table[j] * sc + 50) / 100;
	}
#endif
	if (delta <= 0) 
	  delta = 1;
	if (delta > 32767)
	  delta = 32767;
	if (i == 3 && delta > 1) // The range of the chroma components was extended by one extra bit.
	  delta <<= 1;
	m_pDelta[i][j] = delta;
      }
    } else {
      if (m_pDelta[i]) {
	m_pEnviron->FreeMem(m_pDelta[i],sizeof(UWORD) * 64);
	m_pDelta[i] = NULL;
      }
    }
  }
}
///

/// Quantization::ParseMarker
void Quantization::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len < 2)
    JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker must be at least two bytes long");

  len -= 2; // remove the marker length.

  while(len > 2) {
    LONG type   = io->Get();
    LONG target;
    int i;

    if (type == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker run out of data");

    target = (type & 0x0f);
    type >>= 4;
    if (type != 0 && type != 1)
      JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker entry type must be either 0 or 1");
    if (target < 0 || target > 3)
      JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker target table must be between 0 and 3");

    //
    // For multiple tables the current table is replaced by the new table. Interestingly, this
    // shall be supported according to the specs.
    if (m_pDelta[target] == NULL)
      m_pDelta[target] = (UWORD *)m_pEnviron->AllocMem(sizeof(UWORD) * 64);
    
    len -= 1; // remove the byte.

    if (type == 0) {
      // UBYTE entries.
      if (len < 64)
	JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker contains insufficient data");
      //
      for(i = 0;i < 64;i++) {
	LONG data = io->Get();
	if (data == ByteStream::EOF)
	  JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker run out of data");
	m_pDelta[target][DCT::ScanOrder[i]] = data;
      }
      len -= 64;
    } else {
      assert(type == 1);
      
      if (len < 64 * 2)
	JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker contains insufficient data");
      //
      for(i = 0;i < 64;i++) {
	LONG data = io->GetWord();
	if (data == ByteStream::EOF)
	  JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker run out of data");
	m_pDelta[target][DCT::ScanOrder[i]] = data;
      }
      len -= 64 * 2;
    }
  }

  if (len != 0)
    JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker size corrupt");
}
///
