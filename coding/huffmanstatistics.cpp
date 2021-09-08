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
** This class collects the huffman coder statistics for optimized huffman
** coding.
**
** $Id: huffmanstatistics.cpp,v 1.19 2018/07/27 06:56:43 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "std/string.hpp"
#include "coding/huffmanstatistics.hpp"
#include "coding/huffmantemplate.hpp"
///

/// Defines
#ifdef COLLECT_STATISTICS
// Set the following define to create a universal
// Huffman code that is valid for all sources by
// setting the frequency of all symbols to at least
// one.
#define COMPLETE_CODESET
#endif
///

/// HuffmanStatistics::HuffmanStatistics
#ifdef COMPLETE_CODESET
HuffmanStatistics::HuffmanStatistics(bool dc)
#else
HuffmanStatistics::HuffmanStatistics(bool)
#endif
{
#ifdef COMPLETE_CODESET
  int i,last = 256;
  if (dc)
    last = 16;
  // This should only be set for training purposes
  memset(m_ulCount,0,sizeof(m_ulCount));
  for(i = 0;i < last;i++) {
    m_ulCount[i] = 1;
  }
#else
  memset(m_ulCount,0,sizeof(m_ulCount));
#endif
}
///

/// HuffmanStatistics::CodesizesOf
// Find the number of codesizes of the optimal huffman tree.
// This returns an array of 256 elements, one entry per symbol.
const UBYTE *HuffmanStatistics::CodesizesOf(void)
{
  UQUAD freq[257];    // measured but modified statistics.
  ULONG mstt[256];    // modified statistics.
  int   next[257];    // linkage to other symbols in the same branch.
  UBYTE size[257];
  int i;

  memcpy(mstt,m_ulCount,sizeof(mstt));

  do {
    bool valid = true;
    // Copy statistics over from the temporary.
    for(i = 0;i < 256;i++) {
      freq[i] = mstt[i];
      next[i] = -1;
      size[i] = 0;
    }
    // Reserve a single code point so we do not get a code with
    // all one-bits in the alphabet.
    // The introduction of Annex C in Rec. ITU-T T.81 (1992) | ISO/IEC 10918-1:1994.
    // enforces this, though there is actually not a clear requirement for this
    // except that it is "nice".
    freq[256] = 1;
    next[256] = -1;
    size[256] = 0;
    // Merge frequencies. Always move into the least probable symbol, then remove the
    // second highest. As long as there are symbols left.
    do {
      UQUAD min1 = MAX_UQUAD,min2 = MAX_UQUAD;
      int minarg1 = 0,minarg2 = 0; // shut up the compiler. Not used if less than two symbols.
      
      for(i = 256;i >= 0;i--) { // Give higher entries the deeper nodes in the tree
        if (freq[i] > 0) { // Only valid entries
          if (freq[i] < min1) {
            // min1 becomes available, swap to min2?
            assert(min1 <= min2 || min1 == MAX_UQUAD);
            min2    = min1;
            minarg2 = minarg1;
            min1    = freq[i];
            minarg1 = i;
          } else if (freq[i] < min2) {
            min2    = freq[i];
            minarg2 = i;
          }
        }
      }
      // If there is only one entry left, exit.
      if (min2 == MAX_UQUAD) {
        // If there is only a single symbol, make sure it gets a codesize.
        if (freq[minarg1] == 0)
          freq[minarg1]++;
        break;
      }
      //
      // Merge now, remove the least symbol.
      assert(freq[minarg1] + freq[minarg2] >= freq[minarg1]);
      assert(freq[minarg1] + freq[minarg2] >= freq[minarg2]);
      freq[minarg1] += freq[minarg2];
      freq[minarg2]  = 0;
      //
      // Update the codesize for the subtree at the current position.
      do {
        i = minarg1;
        size[minarg1]++;
        minarg1 = next[minarg1];
      } while(minarg1 >= 0);
      //
      // Merge the two subtrees.
      next[i] = minarg2;
      do {
        size[minarg2]++;
        minarg2 = next[minarg2];
      } while(minarg2 >= 0);
    } while(true);
    //
    // Ok, now check whether all the sizes are at most 16.
    for(i = 0;i < 256;i++) {
      if (size[i] > 16) {
        valid = false;
        break;
      }
      m_ucCodeSize[i] = size[i];
    }
    //
    // If this is a valid assignment, leave.
    if (valid)
      break;
    //
    // Here the optimal huffman code is not JPEG compliant
    // Modify the statistics at the low-frequency end.
    {
      // Find the two minimal *disjoint* code entries.
      ULONG min1 = MAX_ULONG,min2 = MAX_ULONG;
      //
      for(i = 0;i < 256;i++) {
        if (mstt[i] > 0) {
          if (mstt[i] < min1) {
            // min1 becomes available, swap to min2?
            assert(min1 <= min2 || min1 == MAX_ULONG);
            min2    = min1;
            min1    = mstt[i];
          } else if (mstt[i] < min2 && mstt[i] > min1) {
            min2    = mstt[i];
          }
        }
      }
      //
      if (min1 + 1 != min2)
        min2++;
      // Set everything smaller than the second smallest
      // to the second smallest. This will flatten
      // the statistics and hence balance the tree a bit.
      for(i = 0;i < 256;i++) {
        if (mstt[i] > 0 && mstt[i] < min2) {
          mstt[i] = min2;
        }
      }
    }
  } while(true);

  return m_ucCodeSize;
}
///

/// HuffmanStatistics::MergeStatistics
// Merge the counts with the recorded count values in the file.
#ifdef COLLECT_STATISTICS
void HuffmanStatistics::MergeStatistics(FILE *stats,bool ac)
{
  while(!feof(stats)) {
    int symbol;
    unsigned int count;
    const int last = (ac)?256:16;
    if (fscanf(stats,"%d\t%ud\n",&symbol,&count) == 2) {
      if (symbol >= 0 && symbol < last) {
        assert(count + m_ulCount[symbol] >= count);
        assert(count + m_ulCount[symbol] >= m_ulCount[symbol]);
        m_ulCount[symbol] += count;
      }
    }
  }
}
#endif
///

/// HuffmanStatistics::WriteStatistics
// Write the (combined) statistics back to the file.
#ifdef COLLECT_STATISTICS
void HuffmanStatistics::WriteStatistics(FILE *stats,bool ac)
{
  int symbol;
  int last = (ac)?256:16;
  
  for(symbol = 0;symbol < last;symbol++) {
    if (m_ulCount[symbol]) {
      fprintf(stats,"%d\t%d\n",symbol,m_ulCount[symbol]);
    } else {
      fprintf(stats,"%d\t%d\n",symbol,1);
    }
  }
}
#endif
///


