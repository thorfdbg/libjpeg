/*
** This class collects the huffman coder statistics for optimized huffman
** coding.
**
** $Id: huffmanstatistics.hpp,v 1.10 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CODING_HUFFMANSTATISTICS_HPP
#define CODING_HUFFMANSTATISTICS_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bitstream.hpp"
#include "std/string.hpp"
#include "std/assert.hpp"
#include "coding/huffmantemplate.hpp"
#ifdef COLLECT_STATISTICS
#include "std/stdio.hpp"
#endif
///

/// class HuffmanStatistics
// This class collects the huffman coder statistics for optimized huffman
// coding.
class HuffmanStatistics : public JObject {
  //
  // The counter how often symbol #n occured. There is
  // room for at most 256 symbols.
  ULONG m_ulCount[256];
  //
  // Codesizes of the huffman codes. One
  // codesize per symbol.
  UBYTE m_ucCodeSize[256];
  //
public:
  HuffmanStatistics(bool dconly);
  //
  ~HuffmanStatistics(void)
  {
  }
  //
  // Encode the given symbol.
  void Put(UBYTE symbol)
  {
    m_ulCount[symbol]++;
  }
  //
  // Find the number of codesizes of the optimal huffman tree.
  // This returns an array of 256 elements, one entry per symbol.
  const UBYTE *CodesizesOf(void);
  //
  // Functions for measuring the statistics over a larger set of files.
#ifdef COLLECT_STATISTICS
  //
  // Merge the counts with the recorded count values in the file.
  void MergeStatistics(FILE *stats);
  //
  // Write the (combined) statistics back to the file.
  void WriteStatistics(FILE *stats);
  //
#endif
};
///

///
#endif
