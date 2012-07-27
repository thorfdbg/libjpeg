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
 * The Art-Deco (MQ) decoder and encoder as specified by the jpeg2000 
 * standard, FDIS, Annex C
 *
 * $Id: arthdeco.cpp,v 1.3 2012-06-02 10:27:13 thor Exp $
 *
 */

/// Includes
#include "io/bytestream.hpp"
#include "interface/types.hpp"
#include "coding/arthdeco.hpp"
///

/// MQCoder::Qe_Value
const UWORD MQCoder::Qe_Value[] = {
  0x5601,0x3401,0x1801,0x0ac1,0x0521,0x0221,0x5601,0x5401,0x4801,0x3801,
  0x3001,0x2401,0x1c01,0x1601,0x5601,0x54ff,0x5401,0x527d,0x5101,0x4c5f,
  0x4801,0x3f80,0x3801,0x35f7,0x3401,0x31f6,0x3001,0x2801,0x2401,0x2201,
  0x1c01,0x1801,0x1601,0x1401,0x1201,0x1101,0x0ac1,0x09c1,0x08a1,0x0521,
  0x0441,0x02a1,0x0221,0x0141,0x0111,0x0085,0x0049,0x0025,0x0015,0x0009,
  0x0005,0x0001,0x5601
};
///

/// MQCoder::Qe_Switch
const bool MQCoder::Qe_Switch[] = {
  1,0,0,0,0,0,1,0,0,0,
  0,0,0,0,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0
};
///

/// MQCoder::Qe_NextMPS
const UBYTE MQCoder::Qe_NextMPS[] = {
  1,2,3,4,5,44,7,8,9,10,
  11,12,13,35,15,16,17,18,19,20,
  21,22,23,24,25,26,27,28,29,30,
  31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,45,47,48,49,50,
  51,51,52
};
///

/// MQCoder::Qe_NextLPS
const UBYTE MQCoder::Qe_NextLPS[] = {
  1,6,9,12,35,39,6,14,14,14,
  20,22,25,27,14,14,14,15,16,17,
  18,19,20,21,22,23,24,25,26,27,
  28,29,30,31,32,33,34,35,36,37,
  38,39,40,41,42,43,44,45,46,47,
  48,49,52
};
///

/// MQCoder::InitContexts
// Initialize the contexts 
void MQCoder::InitContexts(void)
{
  int i;
  
  for(i = 0;i < Count;i++) {
    m_Contexts[i].m_ucIndex = 0;     // Simply start at context zero.
    m_Contexts[i].m_bMPS    = false; // MPS is zero.
  }
}
///

/// MQCoder::OpenForWrite
// Initialize the MQ Coder for writing to the
// indicated bytestream.
void MQCoder::OpenForWrite(class ByteStream *io)
{
  m_ulA   = 0x8000;
  m_ulC   = 0x0000;
  m_ucCT  = 12;
  m_ucB   = 0x00;
  m_bF    = false;
  m_pIO   = io;
  InitContexts();
}
///

/// MQCoder::OpenForRead
// Initialize the MQ Coder for reading the indicated
// bytestream.
void MQCoder::OpenForRead(class ByteStream *io)
{
  UBYTE t;
  
  m_pIO   = io;
  InitContexts();
  
  m_ucB   = io->Get();
  m_ulC   = m_ucB << 16;
  t       = io->Get();
  m_ucCT  = 8;
  
  if (m_ucB == 0xff) {
    if (t < 0x90) {
      m_ulC += t << 8;
      m_ucCT--;
    }
  }
  
  m_ulC  += t << 8;
  m_ucB   = t;
  m_ulC <<= 7;
  m_ucCT -= 7;
  m_ulA   = 0x8000;
}
///

/// MQCoder::Get
// Read a single bit from the MQCoder
// in a given context index.
bool MQCoder::Get(UBYTE ctxtidx)
{ 
  struct MQContext &ctxt = m_Contexts[ctxtidx];
  ULONG q = Qe_Value[ctxt.m_ucIndex];
  UBYTE t;
  bool d;

  assert(ctxtidx < Count);

  m_ulA -= q;
  if ((m_ulC >> 16) >= q) {
    // MPS case
    m_ulC -= q << 16;
    if (m_ulA & 0x8000) {
      // short MPS case.
      return ctxt.m_bMPS;
    }
    // MPS exchange case
    d = m_ulA < q; // true on LPS
  } else {
    // LPS exchange case
    d = m_ulA >= q; // true on LPS
    m_ulA = q;
  }

  if (d) {
    // LPS decoding, check for MPS/LPS exhchange.
    d ^= ctxt.m_bMPS;
    if (Qe_Switch[ctxt.m_ucIndex])
      ctxt.m_bMPS = d;
    ctxt.m_ucIndex = Qe_NextLPS[ctxt.m_ucIndex];
  } else {
    // MPS decoding
    d = ctxt.m_bMPS;
    ctxt.m_ucIndex = Qe_NextMPS[ctxt.m_ucIndex];
  }

  // 
  // Renormalize.
  do {
    if (m_ucCT == 0) {
      t = m_pIO->Get();
      m_ucCT = 8;
      if (m_ucB == 0xff) {
        if (t < 0x90) {
          m_ulC += t << 8;
          m_ucCT--;
        }
      }
      m_ulC += t << 8;
      m_ucB  = t;
    }
    m_ulA  <<= 1;
    m_ulC  <<= 1;
    m_ucCT--;
  } while((m_ulA & 0x8000) == 0);

  return d;
}
///

/// MQCoder::Put
// Write a single bit to the stream.
void MQCoder::Put(UBYTE ctxtidx,bool bit)
{ 
  assert(ctxtidx < Count);
  
  struct MQContext &ctxt = m_Contexts[ctxtidx];
  ULONG q = Qe_Value[ctxt.m_ucIndex];

  m_ulA  -= q;
  // Check for MPS and LPS coding
  if (bit == ctxt.m_bMPS) {
    // MPS coding
    if (m_ulA & 0x8000) {
      // Short MPS case.
      m_ulC += q;
      return;
    } else {
      // Context change.
      if (m_ulA < q) {
	// MPS/LPS exchange.
	m_ulA  = q;
      } else {
	m_ulC += q;
      }
      ctxt.m_ucIndex = Qe_NextMPS[ctxt.m_ucIndex];
    }
  } else {
    // LPS coding here.
    if (m_ulA < q) {
      m_ulC += q;
    } else {
      m_ulA  = q;
    }
    //
    // MPS/LPS switch?
    ctxt.m_bMPS   ^= Qe_Switch[ctxt.m_ucIndex];
    ctxt.m_ucIndex = Qe_NextLPS[ctxt.m_ucIndex];
  }

  //
  // Renormalize
  do {
    m_ulA <<= 1;
    m_ulC <<= 1;
    if (--m_ucCT == 0) {
      if (m_ucB < 0xff) {
        if (m_ulC & 0x8000000) {
	  // Overflow into the b register, remove carry.
	  m_ucB++;
	  m_ulC &= 0x7ffffff;
        }
      }
      if (m_ucB == 0xff) {
        // We either have an 0xff here, or generated one due to carry.
        // in either case, must have buffered something or the overflow
        // could not have happened.
	m_pIO->Put(0xff);
	m_ucB  = m_ulC >> 20;
	m_ulC &= 0xfffff;
	m_ucCT = 7;
      } else {
	if (m_bF)
	  m_pIO->Put(m_ucB);
	m_ucB  = m_ulC >> 19;
	m_ulC &= 0x7ffff;
	m_ucCT = 8;
      }
      m_bF = true;
    }
  } while((m_ulA & 0x8000) == 0);
}
///

/// MQCoder::Flush
// Flush all remaining bits
void MQCoder::Flush(void)
{ 
  int k;
  //
  // Number of bits to push out is then in k.
  //
  m_ulC <<= m_ucCT;
  for(k = 12 - m_ucCT;k > 0;k -= m_ucCT,m_ulC <<= m_ucCT) {
    if (m_ucB < 0xff) {
      if (m_ulC & 0x8000000) {
	m_ucB++;
        m_ulC &= 0x7ffffff;
      }
    }
    if (m_ucB == 0xff) {
      m_pIO->Put(0xff);
      m_ucB  = m_ulC >> 20;
      m_ulC &= 0xfffff;
      m_ucCT = 7;
    } else {
      if (m_bF)
	m_pIO->Put(m_ucB);
      m_ucB  = m_ulC >> 19;
      m_ulC &= 0x7ffff;
      m_ucCT = 8;
    }
    m_bF = 1;
  }

  if (m_ucB < 0xff) {
    if (m_ulC & 0x8000000) {
      m_ucB++;
    }
  }
  if (m_ucB != 0xff && m_bF)
    m_pIO->Put(m_ucB);
}
///
