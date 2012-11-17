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
** A variant of JPEG LS for the proposed vesa compression. Experimental.
**
** $Id: vesadctscan.hpp,v 1.8 2012-11-01 17:24:32 thor Exp $
**
*/

#ifndef CODESTREAM_VESADCTSCAN_HPP
#define CODESTREAM_VESADCTSCAN_HPP

/// Includes
#include "codestream/jpeglsscan.hpp"
///

/// class VesaDCTScan
class VesaDCTScan : public JPEGLSScan {
  //
  // Bit precision of the components.
  UBYTE        m_ucDepth[4];
  //
  // Bit budget for a line.
  ULONG        m_ulBitBudget;
  //
  // Actually used number of bits per line.
  ULONG        m_ulUsedBits;
  //
  // Maximum number of bits an encoded line may generate
  ULONG        m_ulMaxOvershoot;
  //
  // Total number of samplesper line.
  ULONG        m_ulSamplesPerLine;
  //
  // Available bandwidth in average bits per line.
  ULONG        m_ulBandwidth;
  //
  // Lines - need to buffer four of them.
  LONG        *m_plBuffer[4][4];
  //
  // The DCT buffer for prediction.
  LONG        *m_plDC[4];
  //
  // Scan pattern.
  static const int Scan[16];
  //
  static const int XPos[16],YPos[16];
  //
  // Masks for the EZW-encoding.
  enum {
    Signed      = (1UL << 31),
    Significant = (1UL << 30),
    Encoded     = (1UL << 29),
    ValueMask   = (1UL << 29) - 1
  };
  //
  /*
  // Relative frequencies
  int m_iTotal;            // total number of samples.
  int m_iSignificant;      // significance coding
  int m_iGetsSignificant;  // becomes significant
  int m_iDCGetsSignificant; // DC value gets significant
  int m_iGetsSignificantAfterSignificance; // Gets significant after a significant coefficient
  int m_iZeroTree;         // is start of a ZTree.
  int m_iZeroTreeAfterSignificance;        // is IZ after significant pixel.
  int m_iZeroTreeAfterDC;
  int m_iDCStartsZeroTree;
  int m_iIsolatedZero;     // is an isolated zero.
  int m_iIsolatedZeroAfterSignificance;    // is IZ after significant pixel.
  int m_iIsolatedZeroAfterDC;
  //
    ZT more likely than IZ (by a factor of three)
    but IZ more likely than ZT behind significant symbols.
    completely zero blocks.
    
    block is completely zero: 0
    DC becomes significant: 11
    DC is isolated zero:    10

    Not-DC:
    Isolated zero: 0
    Zerotree:      10
    Becomes significant: 11
   */
  //
  // Backwards transform a single row.
  static void BkwTransformRow(LONG *a,LONG *b,LONG *c,LONG *d)
  {
    LONG b1 = (*a + *c) >> 1;
    LONG b2 = (*b + *d) >> 1;
    LONG b3 = (*a - *c) >> 1;
    LONG b4 = (*b - *d) >> 1;
    // Inverse butterfly.
    *a      = (b1 + b2) >> 1;
    *b      = (b3 + b4) >> 1;
    *c      = (b3 - b4) >> 1;
    *d      = (b1 - b2) >> 1;
  }
  //
  // Backwards transform a complete 4x4 block.
  static void BackwardDCT(LONG *r1,LONG *r2,LONG *r3,LONG *r4)
  {
    // First horizontal.
    BkwTransformRow(r1+0,r1+1,r1+2,r1+3);
    BkwTransformRow(r2+0,r2+1,r2+2,r2+3);
    BkwTransformRow(r3+0,r3+1,r3+2,r3+3);
    BkwTransformRow(r4+0,r4+1,r4+2,r4+3);  
    // Then vertical.
    BkwTransformRow(r1+0,r2+0,r3+0,r4+0);
    BkwTransformRow(r1+1,r2+1,r3+1,r4+1);
    BkwTransformRow(r1+2,r2+2,r3+2,r4+2);
    BkwTransformRow(r1+3,r2+3,r3+3,r4+3);
  }
  //
  // Forward transform an elementary row
  static void FwdTransformRow(LONG *a,LONG *b,LONG *c,LONG *d)
  {
    // Step one: Forwards butterfly.
    LONG b1 = *a + *d;
    LONG b2 = *a - *d;
    LONG b3 = *b + *c;
    LONG b4 = *b - *c;
    // DC and mid-AC coefficients.
    *a      = (b1 + b3);
    *c      = (b1 - b3);
    // Lifting for the rotation.
    *b      = (b2 + b4);
    *d      = (b2 - b4);
  }
  //
  // Return the neutral value of the DC band, initializes the DC offset
  static LONG DCNeutralValue(LONG max)
  {
    return max << 3; // four times four times the DC value, halfed.
  }
  //
  // Forward DCT on a 4x4 block
  static void ForwardDCT(LONG *r1,LONG *r2,LONG *r3,LONG *r4)
  {
    // First vertical
    FwdTransformRow(r1+0,r2+0,r3+0,r4+0);
    FwdTransformRow(r1+1,r2+1,r3+1,r4+1);
    FwdTransformRow(r1+2,r2+2,r3+2,r4+2);
    FwdTransformRow(r1+3,r2+3,r3+3,r4+3);
    // Then horizontal
    FwdTransformRow(r1+0,r1+1,r1+2,r1+3);
    FwdTransformRow(r2+0,r2+1,r2+2,r2+3);
    FwdTransformRow(r3+0,r3+1,r3+2,r3+3);
    FwdTransformRow(r4+0,r4+1,r4+2,r4+3);
  }
  //
  //
  // Update the DC value to what the decoder would reconstruct.
  // Bitlevel is the last bitplane that has been coded.
  void UpdateDC(UBYTE cx,UBYTE bitlevel,int xstart,int xend);
  //
  // Decode a single level in a block.
  void DecodeEZWLevel(UBYTE cx,ULONG x,ULONG bitmask,UBYTE freq)
  {
    LONG &v = m_plBuffer[cx][YPos[freq]][x + XPos[freq]];

    assert(freq <= 15);
    
    if (v & Significant) {
      // Transmit the bit itself.
      v &= ~bitmask;
      if ((m_ulUsedBits++,m_Stream.Get<1>()))
	v |= bitmask;
      v |= bitmask >> 1;
    } else if ((v & Encoded) == 0) {
      if ((m_ulUsedBits++,m_Stream.Get<1>())) {
	// Either IZ for freq = 0, or ZT for later
	// symbols, or significant
	if (freq < 15 && (m_ulUsedBits++,m_Stream.Get<1>()) == 0) {
	  // Is either IZ if freq != 0, or ZT otherwise.
	  if (freq) {
	    UBYTE i = freq + 1;
	    // Zerotree.
	    do {
	      m_plBuffer[cx][YPos[i]][x + XPos[i]] |= Encoded;
	    } while(++i < 16);
	  }
	  return;
	}
	// Becomes significant. Get the sign bit.
	if ((m_ulUsedBits++,m_Stream.Get<1>())) {
	  v = Signed | Significant | bitmask | (bitmask >> 1);
	} else {
	  v = Significant | bitmask | (bitmask >> 1);
	}
      } else if (freq == 0) {
	UBYTE i = freq + 1;
	// Is either IZ or ZT. Is ZT for DC == 0 
	do {
	  m_plBuffer[cx][YPos[i]][x + XPos[i]] |= Encoded;
	} while(++i < 16);
      }
    }
  }
  //
  // Encode a single level in a block.
  void EncodeEZWLevel(UBYTE cx,ULONG x,ULONG bitmask,UBYTE freq)
  {
    LONG &v = m_plBuffer[cx][YPos[freq]][x + XPos[freq]];

    assert(freq <= 15);

    if (v & Significant) {
      // Transmit the bit itself.
      m_Stream.Put<1>((v & bitmask)?(1):(0));m_ulUsedBits++;
    } else if ((v & Encoded) == 0) {
      if (v & bitmask) {
	// Was not significant but became significant.
	if (freq < 15) { // has children.
	  // Is always 11.
	  m_Stream.Put<2>(3);m_ulUsedBits += 2;
	} else {
	  m_Stream.Put<1>(1);m_ulUsedBits += 1;
	}
	// Encode the sign.
	m_Stream.Put<1>((v & Signed)?1:0);m_ulUsedBits++;
	// Becomes significant.
	v |= Significant;
	//
      } else {
	// Insignificant, and remains insignificant.
	if (freq >= 15) {
	  // If no children, a single bit is enough
	  // and no further questions necessary.
	  m_Stream.Put<1>(0);m_ulUsedBits++;
	} else {
	  UBYTE i    = freq + 1;
	  bool ztree = true;
	  // Check whether this is a zero-tree, i.e. all
	  // children are either significant or insignificant
	  do {
	    LONG w = m_plBuffer[cx][YPos[i]][x + XPos[i]];
	    if ((w & Significant) == 0 && (w & bitmask)) {
	      ztree = false;
	      break;
	    }
	  } while(++i < 16);
	  //
	  if (ztree) {
	    // Is a zero-tree. Set all the children to encoded.
	    if (freq == 0) {
	      m_Stream.Put<1>(0);m_ulUsedBits++; // Encode as ZTree.
	    } else {
	      m_Stream.Put<2>(2);m_ulUsedBits+=2; // Encode as ZTree.
	    }
	    i = freq + 1;
	    do {
	      m_plBuffer[cx][YPos[i]][x + XPos[i]] |= Encoded;
	    } while(++i < 16);
	  } else {
	    // Not a zero-tree.
	    if (freq == 0) {
	      m_Stream.Put<2>(2);m_ulUsedBits+=2;
	    } else {
	      m_Stream.Put<1>(0);m_ulUsedBits++; // Encode as ZTree.
	    }
	  }
	}
      }
    }
  }
  //
  // Collect component information and install the component dimensions.
  virtual void FindComponentDimensions(void);
  //
  // Remove all the encoded flags as we move to the next level.
  static void ClearEncodedFlags(ULONG *data,ULONG width)
  {
    do {
      *data++ &= ~Encoded;
    } while(--width);
  }
  //
  // Clear the data completely.
  static void ClearData(ULONG *data,ULONG width)
  {
    do {
      *data++ = 0;
    } while(--width);
  }
  //
  //
public:
  // Create a new scan. This is only the base type.
  VesaDCTScan(class Frame *frame,class Scan *scan,
	      UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~VesaDCTScan(void);
  // 
  // Parse a single MCU in this scan. Return true if there are more
  // MCUs in this row.
  virtual bool ParseMCU(void);
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void); 
};
///


///
#endif

