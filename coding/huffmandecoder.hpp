/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
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
** This class implements a decoder for a single group of bits in a huffman
** decoder.
**
** $Id: huffmandecoder.hpp,v 1.13 2014/11/12 21:24:45 thor Exp $
**
*/

#ifndef CODING_HUFFMANDECODER_HPP
#define CODING_HUFFMANDECODER_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "std/string.hpp"
///

/// class HuffmanDecoder
// This class decodes a group of bits from the IO stream, generating a symbol. This
// is the base class.
class HuffmanDecoder : public JKeeper {
  //
  // Decoder table: Delivers for each 8-bit value the symbol.
  UBYTE  m_ucSymbol[256];
  //
  // Decoder length: Delivers for each 8-bit value the length of the symbol in bits.
  UBYTE  m_ucLength[256];
  //
  // If 8 bits are not sufficient, here are pointers that each point to a 256 byte
  // array indexed by the LSBs.
  UBYTE *m_pucSymbol[256];
  //
  // And ditto for the length.
  UBYTE *m_pucLength[256];
  //
public:
  HuffmanDecoder(class Environ *env,
                 UBYTE *&symbols,UBYTE *&sizes,UBYTE **&lsbsymb,UBYTE **&lsbsize)
    : JKeeper(env)
  {
    symbols = m_ucSymbol;
    sizes   = m_ucLength;
    lsbsymb = m_pucSymbol;
    lsbsize = m_pucLength;

    // Fill the unused area with invalid sizes.
    memset(m_ucLength ,0xff,sizeof(m_ucLength));
    memset(m_pucSymbol,0   ,sizeof(m_pucSymbol));
    memset(m_pucLength,0   ,sizeof(m_pucLength));
  }
  //
  ~HuffmanDecoder(void)
  { 
    int i;

    for(i = 0;i < 256;i++) {
      if (m_pucSymbol[i]) m_pEnviron->FreeMem(m_pucSymbol[i],256 * sizeof(UBYTE));      
      if (m_pucLength[i]) m_pEnviron->FreeMem(m_pucLength[i],256 * sizeof(UBYTE));
    }
  }
  //
  // Decode the next symbol.
  UBYTE Get(BitStream<false> *io)
  {
    UWORD data;
    UBYTE symbol;
    UBYTE size;
    UBYTE lsb,msb;

    data   = io->PeekWord();
    msb    = data >> 8;
    if (likely(m_ucLength[msb])) {
      symbol = m_ucSymbol[msb];
      size   = m_ucLength[msb];
    } else {
      lsb    = data;
      symbol = m_pucSymbol[msb][lsb];
      size   = m_pucLength[msb][lsb];
    }
    
    io->SkipBits(size);

    return symbol;
  }
};
///

///
#endif
