/*
** This class implements an encoder for a single group of bits in a huffman
** decoder.
**
** $Id: huffmancoder.cpp,v 1.10 2014/09/30 08:33:16 thor Exp $
**
*/

/// Includes
#include "std/string.hpp"
#include "coding/huffmancoder.hpp"
///

/// HuffmanCoder::HuffmanCoder
HuffmanCoder::HuffmanCoder(const UBYTE *lengths,const UBYTE *symbols)
{
  int i;
  ULONG value = 0; // current code value.
  UBYTE cnt   = 0;

  memset(m_ucBits,0,sizeof(m_ucBits));
  memset(m_usCode,0,sizeof(m_usCode));

  for(i = 0;i < 16;i++) {
    UBYTE j = lengths[i]; // j = number of codes of size i + 1.
    if (j) {
      do {
	UBYTE symbol = symbols[cnt];
	assert(m_ucBits[symbol] == 0);
	m_ucBits[symbol] = i + 1; // size in bits
	m_usCode[symbol] = value;
	value++;                  // next code
	assert(value < (1UL << (i + 1))); // Overflow?
	cnt++;                    // next entry in the table.
      } while(--j);
    }
    value <<= 1; // shift another bit in.
  }
}
///
