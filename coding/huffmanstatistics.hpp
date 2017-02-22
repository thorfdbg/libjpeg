/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
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
