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
** $Id: huffmanstatistics.cpp,v 1.8 2012-10-07 20:42:32 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "std/string.hpp"
#include "coding/huffmanstatistics.hpp"
///

/// HuffmanStatistics::HuffmanStatistics
HuffmanStatistics::HuffmanStatistics(void)
{
  memset(m_ulCount,0,sizeof(m_ulCount));
}
///

/// HuffmanStatistics::CodesizesOf
// Find the number of codesizes of the optimal huffman tree.
// This returns an array of 256 elements, one entry per symbol.
const UBYTE *HuffmanStatistics::CodesizesOf(void)
{
  ULONG stat[256];    // measured but modified statistics.
  bool valid;

  memcpy(stat,m_ulCount,sizeof(stat));

  do {
    ULONG freqs[514];   // combined frequencies.
    UWORD top[514];     // links
    int   free = 257;   // next available symbol

    // Copy statistics over, reset them.
    memcpy(freqs       ,stat     ,256 * sizeof(LONG));
    memset(freqs + 257,0         ,257 * sizeof(LONG));
    memset(top        ,MAX_UBYTE ,sizeof(top));
    freqs[256] = 1; // this will get the all-1 code which is not allowed.
    
    do {
      // Find the two minimal code entries.
      ULONG min1 = MAX_ULONG,min2 = MAX_ULONG;
      int minarg1 = 0,minarg2 = 0; // shut up the compiler. Not used if less than two symbols.
      int i;
      
      for(i = 0;i < 514;i++) {
	if (freqs[i] > 0) {
	  if (freqs[i] < min1) {
	    // min1 becomes available, swap to min2?
	    assert(min1 <= min2 || min1 == MAX_ULONG);
	    min2    = min1;
	    minarg2 = minarg1;
	    min1    = freqs[i];
	    minarg1 = i;
	  } else if (freqs[i] < min2) {
	    min2    = freqs[i];
	    minarg2 = i;
	  }
	}
      }
      if (min2 == MAX_ULONG)
	break;
      assert(minarg1 != minarg2);   
      //
      // Combine the two minimum symbols.
      freqs[free]     = freqs[minarg1] + freqs[minarg2];
      freqs[minarg1]  = 0; // remove both subtrees.
      freqs[minarg2]  = 0;
      top[minarg1]    = free;
      top[minarg2]    = free;
      
      free++;
      assert(free < 514);
    } while(true);
    
    // 
    // Now assign the codesizes to all entries.
    valid = true;
    //
    for(int i = 0;i < 256;i++) {
      if (freqs[i] > 0) {
	// Special case: A single element.
	m_ucCodeSize[i] = 1;
      } else {
	int codesize = 0,symbol = i;
	// Codesize is the length of the path to
	// the root.
	while(top[symbol] < MAX_UWORD) {
	  codesize++;
	  symbol = top[symbol];
	}
	m_ucCodeSize[i] = codesize;
	//
	// The JPEG markers allow only a maximal code size of 16
	// bits. If the codesize grows beyond that, bail out.
	if (codesize > 16) {
	  valid = false;
	  break;
	}
      }
    }

    if (valid)
      return m_ucCodeSize;

    // Here the optimal huffman code is not JPEG compliant
    // Modify the statistics at the low-frequency end.
    {
      // Find the two minimal *disjoint* code entries.
      ULONG min1 = MAX_ULONG,min2 = MAX_ULONG;
      int i;
      
      for(i = 0;i < 256;i++) {
	if (stat[i] > 0 && stat[i] < min1) {
	  min1    = stat[i];
	}
      }
      for(i = 0;i < 256;i++) {
	if (stat[i] > min1 && stat[i] < min2) {
	  min2    = stat[i];
	}
      }

      //
      // Set everything smaller than the second smallest
      // to the second smallest. This will flatten
      // the statistics and hence balance the tree a bit.
      for(i = 0;i < 256;i++) {
	if (stat[i] > 0 && stat[i] < min2) {
	  stat[i] = min2;
	}
      }
    }
  } while(true);
}
///

/// HuffmanStatistics::MergeStatistics
// Merge the counts with the recorded count values in the file.
#ifdef COLLECT_STATISTICS
void HuffmanStatistics::MergeStatistics(FILE *stats)
{
  while(!feof(stats)) {
    int symbol,count;
    if (fscanf(stats,"%d\t%d\n",&symbol,&count) == 2) {
      if (symbol >= 0 && symbol < 256) {
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
void HuffmanStatistics::WriteStatistics(FILE *stats)
{
  int symbol;
  
  for(symbol = 0;symbol < 256;symbol++) {
    if (m_ulCount[symbol]) {
      fprintf(stats,"%d\t%d\n",symbol,m_ulCount[symbol]);
    } else {
      fprintf(stats,"%d\t%d\n",symbol,1);
    }
  }
}
#endif
///


