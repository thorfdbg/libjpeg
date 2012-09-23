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
 * $Id: qmcoder.cpp,v 1.18 2012-09-09 21:36:13 thor Exp $
 *
 */

/// Includes
#include "io/bytestream.hpp"
#include "interface/types.hpp"
#include "coding/qmcoder.hpp"
///

/// Defines
#ifdef DEBUG_QMCODER
static int counter = 0;
#endif
///

/// QMCoder::Qe_Value
const UWORD QMCoder::Qe_Value[] = {
  0x5a1d,0x2586,0x1114,0x080b,0x03d8,0x01da,0x00e5,0x006f,
  0x0036,0x001a,0x000d,0x0006,0x0003,0x0001,0x5a7f,0x3f25,
  0x2cf2,0x207c,0x17b9,0x1182,0x0cef,0x09a1,0x072f,0x055c,
  0x0406,0x0303,0x0240,0x01b1,0x0144,0x00f5,0x00b7,0x008a,
  0x0068,0x004e,0x003b,0x002c,0x5ae1,0x484c,0x3a0d,0x2ef1,
  0x261f,0x1f33,0x19a8,0x1518,0x1177,0x0e74,0x0bfb,0x09f8,
  0x0861,0x0706,0x05cd,0x04de,0x040f,0x0363,0x02d4,0x025c,
  0x01f8,0x01a4,0x0160,0x0125,0x00f6,0x00cb,0x00ab,0x008f,
  0x5b12,0x4d04,0x412c,0x37d8,0x2fe8,0x293c,0x2379,0x1edf,
  0x1aa9,0x174e,0x1424,0x119c,0x0f6b,0x0d51,0x0bb6,0x0a40,
  0x5832,0x4d1c,0x438e,0x3bdd,0x34ee,0x2eae,0x299a,0x2516,
  0x5570,0x4ca9,0x44d9,0x3e22,0x3824,0x32b4,0x2e17,0x56a8,
  0x4f46,0x47e5,0x41cf,0x3c3d,0x375e,0x5231,0x4c0f,0x4639,
  0x415e,0x5627,0x50e7,0x4b85,0x5597,0x504f,0x5a10,0x5522,
  0x59eb,0x5a1d // state 113 is the uniform state, probability approximately 0.5
};
///

/// QMCoder::Qe_Switch
const bool QMCoder::Qe_Switch[] = {
  1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,1,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  1,0,0,0,0,0,0,0,
  1,0,0,0,0,0,0,1,
  0,0,0,0,0,0,0,0,
  0,1,0,0,0,0,1,0,
  1,0
};
///

/// QMCoder::Qe_NextMPS
const UBYTE QMCoder::Qe_NextMPS[] = {
   1, 2, 3, 4, 5, 6, 7, 8,
   9,10,11,12,13,13,15,16,
  17,18,19,20,21,22,23,24,
  25,26,27,28,29,30,31,32,
  33,34,35, 9,37,38,39,40,
  41,42,43,44,45,46,47,48,
  49,50,51,52,53,54,55,56,
  57,58,59,60,61,62,63,32,
  65,66,67,68,69,70,71,72,
  73,74,75,76,77,78,79,48,
  81,82,83,84,85,86,87,71,
  89,90,91,92,93,94,86,96,
  97,98,99,100,93,102,103,104,
  99,106,107,103,109,107,111,109,
  111,113
};
///

/// QMCoder::Qe_NextLPS
const UBYTE QMCoder::Qe_NextLPS[] = {
  1,14,16,18,20,23,25,28,
  30,33,35,9,10,12,15,36,
  38,39,40,42,43,45,46,48,
  49,51,52,54,56,57,59,60,
  62,63,32,33,37,64,65,67,
  68,69,70,72,73,74,75,77,
  78,79,48,50,50,51,52,53,
  54,55,56,57,58,59,61,61,
  65,80,81,82,83,84,86,87,
  87,72,72,74,74,75,77,77,
  80,88,89,90,91,92,93,86,
  88,95,96,97,99,99,93,95,
  101,102,103,104,99,105,106,107,
  103,105,108,109,110,111,110,112,
  112,113
};
///

/// QMCoder::OpenForWrite
// Initialize the MQ Coder for writing to the
// indicated bytestream.
void QMCoder::OpenForWrite(class ByteStream *io)
{
  m_ucST  = 0;
  m_ucSZ  = 0;
  m_ulC   = 0;
  m_ulA   = 0x10000;
  m_ucCT  = 11;
  m_ucB   = 0x00;
  m_bF    = false; // Point to before the segment.
  m_pIO   = io;
}
///

/// QMCoder::ByteOut
// Flush the byte output buffer.
void QMCoder::ByteOut(void)
{    
  ULONG t = m_ulC >> 19; // output bits in the C register.
  
  if (t > 0xff) {
    // Carry overflow.
    if (m_bF) { 
      // Output any stacked zeros as we are writing a non-zero.
      while(m_ucSZ) {
	m_pIO->Put(0x00);
	m_ucSZ--;
      }
      // Output buffer non-empty, carry over into the output buffer.
      m_ucB++; // Overflow into the buffer.
      assert(m_ucB > 0);
      m_pIO->Put(m_ucB);
      // Byte-stuffing procedure.
      if (m_ucB == 0xff) {
	// Stuff 0 (byte-stuffing)
	m_pIO->Put(0x00);
      }
    }
    // Collect stacked zeros into which we now overflow, so
    // all stacked FF's now become a 0x00
    // These should be written out, but they are delayed since
    // the final flush must remove them anyhow.
    m_ucSZ += m_ucST;
    m_ucST  = 0;
    // Finally buffer the output into which any further coding
    // overflow might run into.
    m_ucB   = t; // Intentionally clips off the lower eight bits.
    m_bF    = true;
  } else if (t == 0xff) {
    // Might overflow into t, count the carry-over's,
    // just count the FF's as we might overflow into them,
    // Keep the byte before the 0xff group in the B register.
    m_ucST++;
  } else {
    // Regular case, no 0xff, overflow propagation is not
    // possible. Push out the buffered zeros, the byte buffer
    // and possibly the string of 0xff's we have here.
    if (m_bF) { 
      // Buffered byte is valid.
      if (m_ucB == 0) {
	// If it is a zero byte, just count the number.
	m_ucSZ++;
      } else {
	// Not a zero, output all the zeros collected so far.
	while(m_ucSZ) {
	  m_pIO->Put(0x00);
	  m_ucSZ--;
	}
	// And make room in the buffer.
	m_pIO->Put(m_ucB);
      }
    }
    //
    // Buffer is now empty.
    // Write the buffered 0xff's now.
    if (m_ucST) {
      while(m_ucSZ) {
	m_pIO->Put(0x00);
	m_ucSZ--;
      }
      //
      while(m_ucST) {
	m_pIO->Put(0xff);
	// Byte-stuffing.
	m_pIO->Put(0x00);
	m_ucST--;
      }
    }
    m_ucB = t;
    m_bF  = true; // buffer is valid.
  } 
  // Remove the written bits.
  m_ulC &= 0x7ffff;
}
///

/// QMCoder::OpenForRead
// Initialize the MQ Coder for reading the indicated
// bytestream.
void QMCoder::OpenForRead(class ByteStream *io)
{
  m_pIO   = io;
  
  m_ulA   = 0x10000;
  m_ulC   = 0;
  ByteIn();
  m_ulC <<= 8;

  ByteIn();
  m_ulC <<= 8;

  m_ucCT  = 0;
  m_usC   = m_ulC >> 16;
  m_usA   = m_ulA;
}
///

/// QMCoder::ByteIn
// Fill the byte input buffer
void QMCoder::ByteIn(void)
{
  LONG b = m_pIO->Get();

  if (b == ByteStream::EOF) {
    return; // Read 0x00 on EOF.
  }

  if (b == 0xff) {
    // Might be a marker - or not.
    m_pIO->LastUnDo();
    if (m_pIO->PeekWord() == 0xff00) {
      // What is expected, a byte-stuffed 0x00
      m_pIO->GetWord();
      m_ulC |= 0xff00; //+ would also work.
    } else {
      // Since the encoder drops 0x00 bytes, we need to fit
      // them in here. Though stay at the EOF.
    }
  } else {
    m_ulC += b << 8;
  }
}
///

/// QMCoder::Get
// Read a single bit from the MQCoder
// in a given context index.
#ifndef FAST_QMCODER
bool QMCoder::Get(class QMContext &ctxt)
{ 
  ULONG q = Qe_Value[ctxt.m_ucIndex];
  bool d; // true on lps

  m_ulA -= q;
  if ((m_ulC >> 16) < m_ulA) {
    // MPS case
    if (m_ulA & 0x8000) {
      // short MPS case.
#ifdef DEBUG_QMCODER_CODE
      printf("#%3d <%c%c%c%c:%d>\n",++counter,ctxt.m_ucID[0],ctxt.m_ucID[1],ctxt.m_ucID[2],ctxt.m_ucID[3],ctxt.m_bMPS);
#endif
      return ctxt.m_bMPS;
    }
    // MPS exchange case
    d = m_ulA < q; // true on LPS
  } else {
    // LPS exchange case
    d = m_ulA >= q; // true on LPS
    // Remove from Cx.
    m_ulC -= m_ulA << 16;
    m_ulA  = q;
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
  assert(m_ulA);
  do {
    if (m_ucCT == 0) {
      ByteIn();
      m_ucCT = 8;
    }
    m_ulA  <<= 1;
    m_ulC  <<= 1;
    m_ucCT--;
  } while((m_ulA & 0x8000) == 0);

#ifdef DEBUG_QMCODER_CODE
  printf("#%3d <%c%c%c%c:%d>\n",++counter,ctxt.m_ucID[0],ctxt.m_ucID[1],ctxt.m_ucID[2],ctxt.m_ucID[3],d);
#endif
  return d;
}
///

/// QMCoder::Put
// Write a single bit to the stream.
void QMCoder::Put(class QMContext &ctxt,bool bit)
{ 
  ULONG q = Qe_Value[ctxt.m_ucIndex];

#ifdef DEBUG_QMCODER_CODE
  printf("#%3d <%c%c%c%c:%d>",++counter,ctxt.m_ucID[0],ctxt.m_ucID[1],ctxt.m_ucID[2],ctxt.m_ucID[3],bit);
#endif 

  m_ulA  -= q;
  // Check for MPS and LPS coding
  if (bit == ctxt.m_bMPS) {
    // MPS coding
    if (m_ulA & 0x8000) {
      // Short MPS case. Do nothing else.
#ifdef DEBUG_QMCODER_CODE
      //printf("#--> %02x,%d\n",ctxt.m_ucIndex,ctxt.m_bMPS);
      printf("\n");
#endif
      return;
    } else {
      // Context change.
      if (m_ulA < q) {
	// MPS/LPS exchange.
	m_ulC += m_ulA;
	m_ulA  = q;
      }
      ctxt.m_ucIndex = Qe_NextMPS[ctxt.m_ucIndex];
    }
  } else {
    // LPS coding here.
    if (m_ulA >= q) {
      m_ulC += m_ulA;
      m_ulA  = q;
    }
    //
    // MPS/LPS switch?
    ctxt.m_bMPS   ^= Qe_Switch[ctxt.m_ucIndex];
    ctxt.m_ucIndex = Qe_NextLPS[ctxt.m_ucIndex];
  }

#ifdef DEBUG_QMCODER_CODE
  //printf("#--> %02x,%d\n",ctxt.m_ucIndex,ctxt.m_bMPS);
  printf("\n");
#endif

  //
  // Renormalize
  assert(m_ulA);
  do {
    m_ulA <<= 1;
    m_ulC <<= 1;
    if (--m_ucCT == 0) {
      ByteOut();
   
      m_ucCT = 8;
    }
  } while((m_ulA & 0x8000) == 0);
}
#endif
///

/// QMCoder::Flush
// Flush all remaining bits
void QMCoder::Flush(void)
{ 
  ULONG t = m_ulC + m_ulA - 1;
  //
  // Clear final bits.
  t &= 0xffff0000;
  if (t < m_ulC) {
    t += 0x8000;
  }
  m_ulC = t;
  //
  m_ulC <<= m_ucCT;
  ByteOut();
  
  m_ulC <<= 8;
  ByteOut(); // note that ByteOut delays sequences of zeros, they never appear in the stream.
  
  m_ulC <<= 8;
  ByteOut(); // note that ByteOut delays sequences of zeros, they never appear in the stream.
}
///
/// QMCoder::GetSlow
// Read a single bit from the MQCoder
// in a given context index.
#ifdef FAST_QMCODER
bool QMCoder::GetSlow(class QMContext &ctxt)
{ 
  ULONG q = Qe_Value[ctxt.m_ucIndex];
  bool d;

  if (m_usC < m_usA) {
    // MPS case
    assert((m_usA & 0x8000) == 0);
    // MPS exchange case
    d = m_usA < q; // true on LPS
  } else {
    // LPS exchange case
    d = m_usA >= q; // true on LPS
    // Remove from Cx.
    m_ulC -= m_usA << 16;
    m_usA  = q;
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
  assert(m_usA);
  do {
    if (m_ucCT == 0) {
      ByteIn();
      m_ucCT = 8;
    }
    m_usA  <<= 1;
    m_ulC  <<= 1;
    m_ucCT--;
  } while((WORD)m_usA > 0);

  m_usC = m_ulC >> 16;
  
#ifdef DEBUG_QMCODER_CODE
  printf("#%3d <%c%c%c%c:%d>\n",++counter,ctxt.m_ucID[0],ctxt.m_ucID[1],ctxt.m_ucID[2],ctxt.m_ucID[3],d);
#endif
  return d;
}
///

/// QMCoder::PutSlow
// Write a single bit to the stream.
void QMCoder::PutSlow(class QMContext &ctxt,bool bit)
{ 
  ULONG q = Qe_Value[ctxt.m_ucIndex];

#ifdef DEBUG_QMCODER_CODE
  printf("#%3d <%c%c%c%c:%d>",++counter,ctxt.m_ucID[0],ctxt.m_ucID[1],ctxt.m_ucID[2],ctxt.m_ucID[3],bit);
#endif 

  // Check for MPS and LPS coding
  if (bit == ctxt.m_bMPS) {
    // MPS coding
    assert((m_ulA & 0x8000) == 0);
    // Context change.
    if (m_ulA < q) {
      // MPS/LPS exchange.
      m_ulC += m_ulA;
      m_ulA  = q;
    }
    ctxt.m_ucIndex = Qe_NextMPS[ctxt.m_ucIndex];
  } else {
    // LPS coding here.
    if (m_ulA >= q) {
      m_ulC += m_ulA;
      m_ulA  = q;
    }
    //
    // MPS/LPS switch?
    ctxt.m_bMPS   ^= Qe_Switch[ctxt.m_ucIndex];
    ctxt.m_ucIndex = Qe_NextLPS[ctxt.m_ucIndex];
  }

#ifdef DEBUG_QMCODER_CODE
  //printf("#--> %02x,%d\n",ctxt.m_ucIndex,ctxt.m_bMPS);
  printf("\n");
#endif

  //
  // Renormalize
  assert(m_ulA);
  do {
    m_ulA <<= 1;
    m_ulC <<= 1;
    if (--m_ucCT == 0) {
      ByteOut();
   
      m_ucCT = 8;
    }
  } while((m_ulA & 0x8000) == 0);
}
#endif
///
