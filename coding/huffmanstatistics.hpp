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
** This class collects the huffman coder statistics for optimized huffman
** coding.
**
** $Id: huffmanstatistics.hpp,v 1.4 2012-10-07 20:42:32 thor Exp $
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
  HuffmanStatistics(void);
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
