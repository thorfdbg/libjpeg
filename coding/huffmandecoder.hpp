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
** This class implements a decoder for a single group of bits in a huffman
** decoder.
**
** $Id: huffmandecoder.hpp,v 1.6 2012-07-15 11:59:44 thor Exp $
**
*/

#ifndef CODING_HUFFMANDECODER_HPP
#define CODING_HUFFMANDECODER_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bitstream.hpp"
#include "std/string.hpp"
///

/// class HuffmanDecoder
// This class decodes a group of bits from the IO stream, generating a symbol. This
// is the base class.
class HuffmanDecoder : public JObject {
  //
  // The number of symbols on this level.
  const ULONG           m_ulEntries;
  //
  // The symbols in this stage.
  const UBYTE          *m_pucValues;
  //
  // Number of bits on this level.
  const UBYTE           m_ucBits;
  //
  // The Huffman decoder for the next larger number of bits.
  class HuffmanDecoder *m_pNext;
  //
  // Return a code on the next level. The first code on this stage is
  // provided as second argument.
  static UBYTE Get(const class HuffmanDecoder *that,BitStream<false> *io,ULONG top)
  { 
    do {
      ULONG dt = io->Get(that->m_ucBits) | (top << that->m_ucBits);
      //
      // If enough entries at this stage, simply decode.
      if (dt < that->m_ulEntries) {
	return that->m_pucValues[dt];
      } else if (that->m_pNext) {
	// Forward to the next stage
	top  = dt  - that->m_ulEntries;
	that = that->m_pNext;
      } else {
	class Environ *m_pEnviron = io->EnvironOf();
	JPG_WARN(MALFORMED_STREAM,"HuffmanDecoder::Get","found invalid huffman code");
	return 0;
      }
    } while(true);
  }
  //
public:
  HuffmanDecoder(const UBYTE *values,UBYTE bits,ULONG entries)
    : m_ulEntries(entries), m_pucValues(values), m_ucBits(bits), m_pNext(NULL)
  {
  }
  //
  ~HuffmanDecoder(void)
  {
    delete m_pNext; // recursively delete the next entry.
  }
  //
  // Extend the code by tagging on a huffman decoder for the next bits.
  void ExtendBy(class HuffmanDecoder *next)
  {
    m_pNext = next;
  }
  //
  // Return the next symbol.
  UBYTE Get(BitStream<false> *io) const
  {
    return Get(this,io,0);
  }
};
///

///
#endif
