/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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
** $Id: huffmancoder.hpp,v 1.15 2016/10/28 13:58:53 thor Exp $
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
  //
  // Return the length of the given symbol.
  UBYTE Length(UBYTE symbol) const
  { 
    if (m_ucBits[symbol])
      return m_ucBits[symbol];
    return MAX_UBYTE;
  }
  //
  // Returns non-zero if the Huffman alphabet contains
  // the passed in symbol, or zero otherwise.
  UBYTE isDefined(UBYTE symbol) const
  {
    return m_ucBits[symbol];
  }
};
///

///
#endif
