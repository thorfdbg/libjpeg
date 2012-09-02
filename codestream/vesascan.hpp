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
** $Id: vesascan.hpp,v 1.16 2012-08-29 08:58:33 thor Exp $
**
*/

#ifndef CODESTREAM_VESASCAN_HPP
#define CODESTREAM_VESASCAN_HPP

//extern bool debug;

/// Includes
#include "codestream/jpeglsscan.hpp"
///

/// class VesaScan
class VesaScan : public JPEGLSScan {
  //
  // Left neighbourhood given the lower three bits of the pixel position.
  static const ULONG m_ulOffset[8];
  //
  // Bit precision of the components.
  UBYTE m_ucDepth[4];
  //
  // Bit budget for a line.
  ULONG m_ulBitBudget;
  //
  // Actually used number of bits per line.
  ULONG m_ulUsedBits;
  //
  // Maximum number of bits an encoded line may generate
  ULONG m_ulMaxOvershoot;
  //
  // Total number of samplesper line.
  ULONG m_ulSamplesPerLine;
  //
  // Available bandwidth in average bits per line.
  ULONG m_ulBandwidth;
  //
  enum {
    // Number of wavelet decomposition levels
    NumLevels   = 5
  };
  //
  // Masks for the EZW-encoding.
  enum {
    Signed      = (1UL << 31),
    Significant = (1UL << 30),
    Encoded     = (1UL << 29),
    ValueMask   = 0xffff
  };
  //
  // Collect component information and install the component dimensions.
  virtual void FindComponentDimensions(void);
  //
  void GetContext(UBYTE comp,LONG x,LONG &a,LONG &b,LONG &c) const
  {
    a = m_pplPrevious[comp][x-1]; // left
    b = m_pplPrevious[comp][x+0];  // top
    c = m_pplPrevious[comp][x+1];  // right
  } 
  //
  // Update the context from the sample at position x so the next line
  // reads the correct context for a and b. Also advances the pointer
  // positions to move to the next sample in the component.
  void UpdateContext(UBYTE comp,LONG x,LONG data)
  {
    m_pplCurrent[comp][x+0] = data;
    m_pplCurrent[comp][x+1] = data; // This defines the proper value for d at the edge.
  }
  //
  // Predict from the top only.
  LONG PredictFromTop(LONG a,LONG b,LONG c) const
  {
    LONG minac = (a > c)?(c):(a);
    LONG maxac = (a > c)?(a):(c);
    
    // 
    // Does this make much sense? probably a simple
    // differential predictor would be satisfying.
    if (b > maxac) {
      return (b + maxac) >> 1;
    } else if (b < minac) {
      return (b + minac) >> 1;
    } else {
      return b; // works better than a mid-point prediction.
    }
  }
  //
  // Compute a high-pass filter on the input line with the given
  // length and the given in-band increment.
  static void ComputeHighpass(LONG *data,ULONG width,LONG increment)
  {
    ULONG x;

    //
    // go to the high-pass.
    data += increment; 
    for(x = increment;x < width - increment;x += increment << 1) {
      data[0]  -= (data[-increment] + data[+increment]) >> 1;
      data     += increment << 1;
    }
    //
    // Last sample in the line: Mirror extend.
    if (x < width)
      data[0] -= data[-increment];
  }
  //
  // Compute the low-pass filter on the input line with the given
  // length and the given in-band increment.
  static void ComputeLowpass(LONG *data,ULONG width,LONG increment)
  {
    ULONG x;

    // First sample: Mirror extend the left from the right.
    data[0] += (data[increment] + 1) >> 1;
    //
    // Advance to the next sample already.
    data    += increment << 1;
    for(x = increment << 1;x < width - increment;x += increment << 1) {
      data[0] += (data[-increment] + data[+increment] + 2) >> 2;
      data    += increment << 1;
    }

    if (x < width)
      data[0] += data[-increment];
  }
  //
  //
  //
  // Reconstruct the high-pass filter on the input line with the given
  // length and the given in-band increment.
  static void ReconstructHighpass(LONG *data,ULONG width,LONG increment)
  {
    ULONG x;

    //
    // go to the high-pass.
    data += increment; 
    for(x = increment;x < width - increment;x += increment << 1) {
      data[0]  += (data[-increment] + data[+increment]) >> 1;
      data     += increment << 1;
    }
    //
    // Last sample in the line: Mirror extend.
    if (x < width)
      data[0] += (data[-increment] + 1) >> 1;
  }
  //
  // Reconstruct the low-pass filter on the input line with the given
  // length and the given in-band increment.
  static void ReconstructLowpass(LONG *data,ULONG width,LONG increment)
  {
    ULONG x;

    // First sample: Mirror extend the left from the right.
    data[0] -= (data[increment] + 1) >> 1;
    //
    // Advance to the next sample already.
    data     += increment << 1;
    for(x = increment << 1;x < width - increment;x += increment << 1) {
      data[0] -= (data[-increment] + data[+increment] + 2) >> 2;
      data    += increment << 1;
    }

    if (x < width)
      data[0] -= (data[-increment] + 1) >> 1;
  }
  //
  // Compute the total number of bitplanes to encode.
  // This returns a count value.
  static UBYTE BitplanesOf(LONG *data,ULONG width)
  {
    ULONG ormask = 0;
    UBYTE planes = 16;

    do {
      ULONG dt = (*data >= 0)?(*data):(-*data);
      ormask  |= dt;
      *data    = (*data >= 0)?(dt & ValueMask):((dt & ValueMask) | Signed);
      data++;
    } while(--width);

    while(planes--) {
      if ((ormask & (1 << planes)))
	return planes + 1;
    }
    
    return 0;
  }
  //
  // Convert back, keeping in mind that some levels might be coded to a higher bitdepths
  // than others.
  // Abort is true if the abortion happened in the middle of the coding, false otherwise.
  // If true, then abortcx,abortlevel and abortbitlevel are the position of the first iteration
  // that was no longer coded, i.e. everything ahead was coded correctly.
  void ConvertToComplement(struct Line *line[4],bool abort,UBYTE abortcx,UBYTE abortlevel,UBYTE abortbitlevel);
  //
  // Clear the EZW status flags, convert back to two's complemenet representation.
  // The bitplane is the last encoded bitplane.
  static void ConvertToComplement(LONG *data,ULONG width,UBYTE inc,UBYTE lastcoded,bool lowpass)
  {
    ULONG validmask = (MAX_ULONG << lastcoded) & ValueMask;
    ULONG lastmask  = (1 << lastcoded) >> 1;
    ULONG increment = 1UL << inc;
    ULONG x;

    for(x = (lowpass)?(0):(increment >> 1);x < width;x += increment) {
      ULONG dt = (ULONG)(data[x]);
      if (dt & validmask) {
	data[x]    = (dt & Signed)?(-((dt & validmask) | lastmask)):
	  ((dt & validmask) | lastmask);
      } else {
	data[x]    = 0;
      }
    }
  }
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
  // Test wehther at the given position a zero tree starts.
  // Increment is the increment from one position to the next position within
  // the same band.
  static bool isZeroTree(ULONG *data,ULONG width,ULONG x,ULONG increment,ULONG bitmask)
  {
    /*
    if ((data[x] & Significant) || (data[x] & bitmask) == 0) {
      if (increment > 1) {
	// Must test next level.
	if (((x - (increment >> 1)) >= width || 
	     isZeroTree(data,width,x - (increment >> 1),increment >> 1,bitmask)) &&
	    ((x + (increment >> 1)) >= width || 
	     isZeroTree(data,width,x + (increment >> 1),increment >> 1,bitmask)))
	  return true;
	return false;
      } else {
	return true;
      }
    }
    return false;
    */
    // Make use of the interleaving and come up with an even simpler algorithm
    // that is iterative.
    ULONG xmin = ((x - increment + 1) < width)?(x - increment + 1):(0);
    ULONG xmax = ((x + increment - 1) < width)?(x + increment - 1):(width - 1);
    
    assert(xmin <= xmax);
    for(x = xmin;x <= xmax;x++)
      if ((data[x] & Significant) == 0 && (data[x] & bitmask))
	return false;
    return true;
  }
  //
  // Mark the complete zero tree as encoded.
  static void markTreeAsEncoded(ULONG *data,ULONG width,ULONG x,ULONG increment)
  {
    /*
    data[x] |= Encoded;

    if (increment > 1) {
      if (x - (increment >> 1) < width)
	markTreeAsEncoded(data,width,x - (increment >> 1),increment >> 1);
      if (x + (increment >> 1) < width)
	markTreeAsEncoded(data,width,x + (increment >> 1),increment >> 1);
    }
    */
    // Due to the interleaved layout of the samples, an iterative approach is possible
    // and even simpler.
    ULONG xmin = ((x - increment + 1) < width)?(x - increment + 1):(0);
    ULONG xmax = ((x + increment - 1) < width)?(x + increment - 1):(width - 1);
    
    assert(xmin <= xmax);
    for(x = xmin;x <= xmax;x++)
      data[x] |= Encoded;
  }
  //
  // Encode a bitplane level with the EZW 
  void EncodeEZWLevel(ULONG *data,ULONG width,UBYTE inc,ULONG bitmask,bool lowpass)
  {
    ULONG x;
    ULONG increment = 1UL << inc;

    for(x = (lowpass)?(0):(increment >> 1);x < width;x += increment) {
      // Anything to do at all?
      if (data[x] & Significant) {
	m_Stream.Put<1>((data[x] & bitmask)?(true):(false));m_ulUsedBits++;
	//if (debug) printf("%d:RF%d\n",x,(data[x] & bitmask)?(true):(false));
      } else if ((data[x] & Encoded) == 0) {
	if (data[x] & bitmask) {
	  // Not a zero-tree, becomes significant.
	  m_Stream.Put<1>(1);m_ulUsedBits++;
	  // Sign-coding.
	  m_Stream.Put<1>((data[x] & Signed)?(true):(false));m_ulUsedBits++;
	  // Becomes significant.
	  data[x] |= Significant;
	  //if (debug) printf("%d:SC%d\n",x,(data[x] & Signed)?(true):(false));
	} else {
	  // Not significant.
	  m_Stream.Put<1>(0);m_ulUsedBits++;
	  //if (debug) printf("%d:Z\n",x);
	  // May be a zero-tree. Makes only sense to test
	  // if this is not the lowest level anyhow or if there are children.
	  // For even position in the lowpass, there are no children either.
	  if ((lowpass && ((x >> inc) & 1)) || (!lowpass && increment > 2)) {
	    if (isZeroTree(data,width,x,(lowpass)?(increment):(increment >> 1),bitmask)) {
	      m_Stream.Put<1>(1);m_ulUsedBits++;
	      //if (debug) printf(" T\n");
	      markTreeAsEncoded(data,width,x,(lowpass)?(increment):(increment >> 1));
	    } else {
	      // Isolated zero.
	      m_Stream.Put<1>(0);m_ulUsedBits++;
	      //if (debug) printf(" I\n");
	    }
	  }
	}
      }
    }
    //if (debug) printf("\n");
  }
  //
  // Decode a bitplane level with the EZW.
  void DecodeEZWLevel(ULONG *data,ULONG width,UBYTE inc,ULONG bitmask,bool lowpass)
  {
    ULONG x;
    ULONG increment = 1UL << inc;

    for(x = (lowpass)?(0):(increment >> 1);x < width;x += increment) { 
      // Significant or not?
      if (data[x] & Significant) {
	if ((m_ulUsedBits++,m_Stream.Get<1>()))
	  data[x] |=  bitmask; // Get the bit itself.
	//if (debug) printf("%d:RF%d\n",x,(data[x] & bitmask)?(true):(false));
      } else if ((data[x] & Encoded) == 0) {
	if ((m_ulUsedBits++,m_Stream.Get<1>())) { // Zero or not?
	  // Here not zero. Get the sign.
	  data[x] |= bitmask;
	  // Get the sign bit.
	  if ((m_ulUsedBits++,m_Stream.Get<1>())) 
	    data[x] |= Signed;
	  // Becomes significant.
	  data[x] |= Significant;
	  //if (debug) printf("%d:SC%d\n",x,(data[x] & Signed)?(true):(false));
	} else {
	  //if (debug) printf("%d:Z\n",x);
	  if ((lowpass && ((x >> inc) & 1)) || (!lowpass && increment > 2)) {
	    // Here a zero-tree or an isolated zero.
	    if (m_ulUsedBits++,m_Stream.Get<1>()) { 
	      // Zero tree.
	      assert(data[x] == 0);
	      markTreeAsEncoded(data,width,x,(lowpass)?(increment):(increment >> 1));
	      //if (debug) printf(" T\n");
	    } //else if (debug) printf(" I\n");
	  }
	} 
      }
    }
    //if (debug) printf("\n");
  }
  //
public:
  // Create a new scan. This is only the base type.
  VesaScan(class Frame *frame,class Scan *scan,
	   UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~VesaScan(void);
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

