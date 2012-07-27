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
** This class implements an encoder for a single group of bits in a huffman
** decoder.
**
** $Id: huffmancoder.hpp,v 1.7 2012-07-15 11:59:44 thor Exp $
**
*/

#ifndef CODING_HUFFMANCODER_HPP
#define CODING_HUFFMANCODER_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bitstream.hpp"
#include "std/string.hpp"
#include "std/assert.hpp"
///

/// class HuffmanCoder
// This class encodes a symbol to its huffman code. There is only one huffman
// coder in total, defining the code and the code length. A huffman code can
// be at most 16 bit wide, and there can be at most 255 codes.
class HuffmanCoder : public JObject {
  //
  // The number of bits of the i'th symbol. From 0 (undefined) to 16
  UBYTE                m_ucBits[256];
  //
  // The code for the i'th symbol, right-aligned.
  UWORD                m_usCode[256];
  //
public:
  HuffmanCoder(const UBYTE *lengths,const UBYTE *symbols);
  //
  ~HuffmanCoder(void)
  {
  }
  //
  // Encode the given symbol.
  void Put(BitStream<false> *io,UBYTE symbol) const
  {
    if (m_ucBits[symbol] == 0) {
      class Environ *m_pEnviron = io->EnvironOf();
      JPG_THROW(INVALID_HUFFMAN,"HuffmanCoder::Put",
		"Huffman table is unsuitable for selected coding mode - "
		"try to build an optimized Huffman table");
    }
    io->Put(m_ucBits[symbol],m_usCode[symbol]);
  }
};
///

///
#endif
