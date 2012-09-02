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
** $Id: vesascan.cpp,v 1.15 2012-09-02 16:31:43 thor Exp $
**
*/

/// Includes
#include "codestream/jpeglsscan.hpp"
#include "codestream/vesascan.hpp"
///

/// VesaScan::VesaScan
// Create a new scan. This is only the base type.
VesaScan::VesaScan(class Frame *frame,class Scan *scan,
		   UBYTE near,const UBYTE *mapping,UBYTE point)
  : JPEGLSScan(frame,scan,near,mapping,point)
{
}
///

/// VesaScan::~VesaScan
// Dispose a scan.
VesaScan::~VesaScan(void)
{
}
///


/// VesaScan::FindComponentDimensions
// Collect component information and install the component dimensions.
// Also called (indirectly) to start writing or parsing a new scan.
void VesaScan::FindComponentDimensions(void)
{ 
  UBYTE cx;
  
  JPEGLSScan::FindComponentDimensions();

  if (m_ucCount > 4)
    JPG_THROW(OVERFLOW_PARAMETER,"VesaScan::FindComponentDimensions",
	      "JPEG LS Vesa scan does not support more than four components");
  
  //
  // Check that all MCU dimensions are 1.
  for(cx = 0;cx < m_ucCount;cx++) {
    class Component *comp = ComponentOf(cx);
    if (comp->MCUHeightOf() != 1 || comp->MCUWidthOf() != 1)
      JPG_THROW(INVALID_PARAMETER,"VesaScan::FindComponentDimensions",
		"sample interleaved JPEG LS does not support subsampling");
    m_ucDepth[cx]   = comp->PrecisionOf();
  }

  // Initialize the predictor - somewhat.
  for(cx = 0;cx < m_ucCount;cx++) {
    StartLine(cx);
    LONG *bp = m_pplPrevious[cx] - 1;    
    LONG   w = m_ulWidth[cx] + 2;
    do {
      *bp++  = (m_lMaxVal + 1) >> 1;
    } while(--w);
  }
  //
  m_ulSamplesPerLine = 0;
  m_ulMaxOvershoot   = 0;
  for(cx = 0;cx < m_ucCount;cx++) {
    m_ulSamplesPerLine += m_ulWidth[cx];
    if (m_ulWidth[cx] << 1 > m_ulMaxOvershoot)
      m_ulMaxOvershoot = m_ulWidth[cx] << 1;
  }
  m_ulBandwidth      = (8 * m_lNear * m_ulSamplesPerLine) / 100;
  m_ulBitBudget      = m_ulBandwidth;
}
///

/// VesaScan::ConvertToComplement
// Convert back, keeping in mind that some levels might be coded to a higher bitdepths
// than others.
// Abort is true if the abortion happened in the middle of the coding, false otherwise.
// If true, then abortcx,abortlevel and abortbitlevel are the position of the first iteration
// that was no longer coded, i.e. everything ahead was coded correctly.
void VesaScan::ConvertToComplement(struct Line *line[4],bool abort,UBYTE abortcx,UBYTE abortlevel,UBYTE abortbitlevel)
{
  UBYTE cx;
  
  if (abort == false) {
    // Everything was coded.
    for(cx = 0;cx < m_ucCount;cx++) {
      ConvertToComplement(line[cx]->m_pData,m_ulWidth[cx],0,0,true);
    }
  } else {
    // Ahead of the abortion position, the bitplane # abortbitlevel was still coded,
    // but is not anymore beyond that point.
    ULONG level = NumLevels;
    UBYTE last  = abortbitlevel;
    do {
      UBYTE increment = (level == NumLevels)?(level):(level + 1);
      bool lowpass    = (level == NumLevels)?(true):(false);
      for(cx = 0;cx < m_ucCount;cx++) {
	if (abortcx == cx && abortlevel == level) {
	  // This is the position where coding stopped. Now the topmost bitplane is last+1
	  last++;
	}
	ConvertToComplement(line[cx]->m_pData,m_ulWidth[cx],increment,last,lowpass);
      }
    } while(level--);
  }
}
///

/// VesaScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more
// MCUs in this row.
bool VesaScan::ParseMCU(void)
{
  int lines             = m_ulRemaining[0]; // total number of MCU lines processed.;
  UBYTE preshift        = m_ucLowBit + FractionalColorBitsOf();
  LONG max              = ((m_lMaxVal + 1) << preshift) - 1;
  struct Line *line[4];
  UBYTE cx,lvl;
  static int linecounter = 0;
  
  //
  // A "MCU" in respect to the code organization is eight lines.
  if (lines > 8) {
    lines = 8;
  }
  m_ulRemaining[0] -= lines;
  assert(lines > 0);
  assert(m_ucCount < 4);

  //
  // Fill the line pointers.
  for(cx = 0;cx < m_ucCount;cx++) {
    line[cx] = CurrentLine(cx);
  }

  // Loop over lines and columns
  do {
    ULONG x;
    //
    // Get the line pointers and initialize the internal backup lines.
    for(cx = 0;cx < m_ucCount;cx++) {
      StartLine(cx); 
    }
    //
    BeginReadMCU(m_Stream.ByteStreamOf()); 
    m_ulUsedBits = 0;
    //    
    // Decode the levels
    //debug = (linecounter >= 19) || (linecounter <= 24);
    {
      UBYTE bitplane[4];
      UBYTE maxbits  = 0;
      bool abort     = false;
      UBYTE bitlevel      = 0;
      ULONG abortlevel    = 0;
      UBYTE abortcx       = 0;
      UBYTE abortbitlevel = 0;
      assert(m_ucCount < 4);
      // First phase: Decode the number of bitplanes in each component.
      for(cx = 0;cx < m_ucCount;cx++) {
	bitlevel = 0;
	while((m_ulUsedBits++,m_Stream.Get<1>()) == 0)
	  bitlevel++;
	bitplane[cx] = bitlevel;
	if (bitlevel > maxbits)
	  maxbits = bitlevel;
	ClearData((ULONG *)(line[cx]->m_pData),m_ulWidth[cx]);
      }
      //
      // Now decode level by level. The abort condition is implicit.
      bitlevel = maxbits;
      while(bitlevel && abort == false) {
	ULONG level = NumLevels;
	bitlevel--;
	for(cx = 0;cx < m_ucCount;cx++) {
	  ClearEncodedFlags((ULONG *)(line[cx]->m_pData),m_ulWidth[cx]);
	}
	do {
	  UBYTE increment = (level == NumLevels)?(level):(level + 1);
	  bool lowpass    = (level == NumLevels)?(true):(false);
	  //
	  for(cx = 0;cx < m_ucCount;cx++) {
	    if (bitlevel < bitplane[cx]) {
	      if (m_ulUsedBits + ((m_ulWidth[cx] >> increment) << 1) >= m_ulBitBudget) {
		abortcx       = cx;
		abortlevel    = level;
		abortbitlevel = bitlevel;
		abort         = true;
		break;
	      } else {
		//if (debug) printf("Line : %d,%d,%d,%d\n",linecounter,cx,increment,bitlevel);
		DecodeEZWLevel((ULONG *)(line[cx]->m_pData),m_ulWidth[cx],increment,1 << bitlevel,lowpass);
	      }
	    }
	  }
	} while(abort == false && level--);
      }
      //
      // Convert back, keeping in mind that some levels might be coded to a higher bitdepths
      // than others.
      ConvertToComplement(line,abort,abortcx,abortlevel,abortbitlevel);
    }
    //
    /*
    printf("%d:\t",linecounter);
    for(x = 0;x < m_ulWidth[0];x++) {
      printf("%4d:%3d\n",x,line[0]->m_pData[x]);
    }
    printf("\n");
    */
    // Perform the decoding process so we can provide the prediction values
    // for the next level.
    for(cx = 0;cx < m_ucCount;cx++) {
      lvl = NumLevels;
      do {
	lvl--;
	int dist = 1 << lvl;
	ReconstructLowpass(line[cx]->m_pData,m_ulWidth[cx],dist);
	ReconstructHighpass(line[cx]->m_pData,m_ulWidth[cx],dist);
      } while(lvl);
    }
    //  
    //
    // Predict from top. 
    for(cx = 0;cx < m_ucCount;cx++) {
      for(x = 0;x < m_ulWidth[cx];x++) {
	LONG a,b,c,px;
	
	GetContext(cx,x,a,b,c);
	//
	px                   = PredictFromTop(a,b,c);
	line[cx]->m_pData[x] = (line[cx]->m_pData[x] + px) << preshift;	
	if (line[cx]->m_pData[x] < 0) 
	  line[cx]->m_pData[x] = 0;
	if (line[cx]->m_pData[x] > max) 
	  line[cx]->m_pData[x] = max;
	UpdateContext(cx,x,line[cx]->m_pData[x] >> preshift);
      }
    }
    //
    // Advance the line pointers.
    for(cx = 0;cx < m_ucCount;cx++) {
      EndLine(cx);
      line[cx] = line[cx]->m_pNext;
    }
    //
    // Check how many bits are available for the next line.
    if (m_ulUsedBits < m_ulBitBudget) {
      m_ulBitBudget -= m_ulUsedBits;
    } else {
      m_ulBitBudget  = 0;
    }
    m_ulBitBudget   += m_ulBandwidth;
    m_ulUsedBits     = 0;
    linecounter++;
    //
  } while(--lines);
  
  return false;

}
///

/// VesaScan::WriteMCU
// Write a single MCU in this scan.
bool VesaScan::WriteMCU(void)
{
  int lines             = m_ulRemaining[0]; // total number of MCU lines processed.
  UBYTE preshift        = m_ucLowBit + FractionalColorBitsOf();
  LONG max              = ((m_lMaxVal + 1) << preshift) - 1;
  struct Line *line[4];
  UBYTE cx,lvl;
  static int linecounter = 0;
  
  //
  // A "MCU" in respect to the code organization is eight lines.
  if (lines > 8) {
    lines = 8;
  }
  m_ulRemaining[0] -= lines;
  assert(lines > 0);
  assert(m_ucCount < 4);

  //
  // Fill the line pointers.
  for(cx = 0;cx < m_ucCount;cx++) {
    line[cx] = CurrentLine(cx);
  }

  // Loop over lines and columns
  do {
    ULONG x;
    // Get the line pointers and initialize the internal backup lines.
    for(cx = 0;cx < m_ucCount;cx++) {
      StartLine(cx); 
    }
    //
    BeginWriteMCU(m_Stream.ByteStreamOf()); 
    m_ulUsedBits = 0;
    //
    // Phase one: Predict from top.
    for(cx = 0;cx < m_ucCount;cx++) {
      for(x = 0;x < m_ulWidth[0];x++) {
	LONG a,b,c,px;
	
	GetContext(cx,x,a,b,c);
	//
	px                   = PredictFromTop(a,b,c);
	line[cx]->m_pData[x] = (line[cx]->m_pData[x] >> preshift) - px;	
      }
    }
    //
    // Phase two: A simple 5/3 wavelet. Can we do better?
    for(cx = 0;cx < m_ucCount;cx++) {
      for(lvl = 0;lvl < NumLevels;lvl++) {
	int dist = 1 << lvl;
	ComputeHighpass(line[cx]->m_pData,m_ulWidth[cx],dist);
	ComputeLowpass(line[cx]->m_pData,m_ulWidth[cx],dist);
      }
    }
    //
    // Encode the levels
    //debug = (linecounter >= 19) || (linecounter <= 24);
    {
      UBYTE bitplane[4];
      UBYTE maxbits = 0;
      bool abort    = false;
      UBYTE bitlevel      = 0;
      ULONG abortlevel    = 0;
      UBYTE abortcx       = 0;
      UBYTE abortbitlevel = 0;
      assert(m_ucCount < 4);
      for(cx = 0;cx < m_ucCount;cx++) {
	bitplane[cx]  = BitplanesOf(line[cx]->m_pData,m_ulWidth[cx]);
	//
	// First encode the number of bitplanes.
	if (bitplane[cx])
	  m_Stream.Put(bitplane[cx],0);
	if (bitplane[cx] > maxbits)
	  maxbits = bitplane[cx];
	m_Stream.Put<1>(1);
	m_ulUsedBits += bitplane[cx] + 1;
      }
      //
      bitlevel = maxbits;
      while(bitlevel && abort == false) {
	ULONG level  = NumLevels;
	for(cx = 0;cx < m_ucCount;cx++) {
	  ClearEncodedFlags((ULONG *)(line[cx]->m_pData),m_ulWidth[cx]);
	}
	bitlevel--;
	do {
	  UBYTE increment = (level == NumLevels)?(level):(level + 1);
	  bool lowpass    = (level == NumLevels)?(true):(false);
	  //
	  for(cx = 0;cx < m_ucCount;cx++) {
	    if (bitlevel < bitplane[cx]) {
	      // Enough data available?
	      if (m_ulUsedBits + ((m_ulWidth[cx] >> increment) << 1) >= m_ulBitBudget) {
		abortcx       = cx;
		abortlevel    = level;
		abortbitlevel = bitlevel;
		abort         = true;
		break;
	      } else {
		//if (debug) printf("Line : %d,%d,%d,%d\n",linecounter,cx,increment,bitlevel);
		EncodeEZWLevel((ULONG*)(line[cx]->m_pData),m_ulWidth[cx],increment,1 << bitlevel,lowpass);
	      }
	    }
	  }
	} while(abort == false && level--);
      } 
      //
      // Convert back, keeping in mind that some levels might be coded to a higher bitdepths
      // than others.
      ConvertToComplement(line,abort,abortcx,abortlevel,abortbitlevel);
    }
    //
    // Mirror the decoding process so we can provide the prediction values
    // for the next level.
    for(cx = 0;cx < m_ucCount;cx++) {
      lvl = NumLevels;
      do {
	lvl--;
	int dist = 1 << lvl;
	ReconstructLowpass(line[cx]->m_pData,m_ulWidth[cx],dist);
	ReconstructHighpass(line[cx]->m_pData,m_ulWidth[cx],dist);
      } while(lvl);
    }
    //
    // Insert the reconstructed values for the next line.
    for(cx = 0;cx < m_ucCount;cx++) {
      for(x = 0;x < m_ulWidth[cx];x++) {
	LONG a,b,c,px;
	
	GetContext(cx,x,a,b,c);
	//
	px                   = PredictFromTop(a,b,c);
	line[cx]->m_pData[x] = (line[cx]->m_pData[x] + px) << preshift;	
	if (line[cx]->m_pData[x] < 0) 
	  line[cx]->m_pData[x] = 0;
	if (line[cx]->m_pData[x] > max) 
	  line[cx]->m_pData[x] = max;
	UpdateContext(cx,x,line[cx]->m_pData[x] >> preshift);
      }
    }
    //
    // Advance the line pointers.
    for(cx = 0;cx < m_ucCount;cx++) {
      EndLine(cx);
      line[cx] = line[cx]->m_pNext;
    }
    //
    // Check how many bits are available for the next line.
    if (m_ulUsedBits < m_ulBitBudget) {
      m_ulBitBudget -= m_ulUsedBits;
    } else {
      m_ulBitBudget  = 0;
    }
    m_ulBitBudget   += m_ulBandwidth;
    m_ulUsedBits     = 0;
    //
    linecounter++;
  } while(--lines);

  return false;
}
///
