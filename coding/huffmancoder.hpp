/*
** This class implements an encoder for a single group of bits in a huffman
** decoder.
**
** $Id: huffmancoder.hpp,v 1.14 2014/09/30 08:33:16 thor Exp $
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
};
///

///
#endif
