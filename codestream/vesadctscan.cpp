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
** $Id: vesadctscan.cpp,v 1.11 2012-11-09 13:09:16 thor Exp $
**
*/

/// Includes
#include "codestream/jpeglsscan.hpp"
#include "codestream/vesadctscan.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
///

/// Scan pattern.
#define P(x,y) ((x) + ((y) << 2))
const int VesaDCTScan::Scan[16] = {
  P(0,0),
  P(1,0), P(0,1),
  P(0,2), P(1,1), P(2,0),
  P(3,0), P(2,1), P(1,2), P(0,3),
  P(1,3), P(2,2), P(3,1),
  P(3,2), P(2,3),
  P(3,3)
};
#undef P

const int VesaDCTScan::XPos[16] = {0,
				   1,0,
				   0,1,2,
				   3,2,1,0,
				   1,2,3,
				   3,2,
				   3};

const int VesaDCTScan::YPos[16] = {0,
				   0,1,
				   2,1,0,
				   0,1,2,3,
				   3,2,1,
				   2,3,
				   3};
///

/// VesaDCTScan::VesaDCTScan
// Create a new scan. This is only the base type.
VesaDCTScan::VesaDCTScan(class Frame *frame,class Scan *scan,
		   UBYTE near,const UBYTE *mapping,UBYTE point)
  : JPEGLSScan(frame,scan,near,mapping,point)
{
  memset(m_plBuffer,0,sizeof(m_plBuffer));
  memset(m_plDC    ,0,sizeof(m_plDC));

  /*
  m_iTotal = 0;            // total number of samples.
  m_iSignificant = 0;      // significance coding
  m_iGetsSignificant = 0;  // becomes significant
  m_iDCGetsSignificant = 0; // DC value gets significant
  m_iGetsSignificantAfterSignificance = 0; // Gets significant after a significant coefficient
  m_iZeroTree = 0;         // is start of a ZTree.
  m_iZeroTreeAfterSignificance = 0;        // is IZ after significant pixel.
  m_iZeroTreeAfterDC = 0;
  m_iIsolatedZero = 0;     // is an isolated zero.
  m_iIsolatedZeroAfterSignificance = 0;    // is IZ after significant pixel.
  m_iIsolatedZeroAfterDC = 0;
  m_iDCStartsZeroTree = 0;
  */
}
///

/// VesaDCTScan::~VesaDCTScan
// Dispose a scan.
VesaDCTScan::~VesaDCTScan(void)
{
  int i,j;

  for(i = 0;i < 4;i++) {
    for(j = 0;j < 4;j++) {
      if (m_plBuffer[i][j])
	m_pEnviron->FreeMem(m_plBuffer[i][j],((m_ulWidth[i] + 3) & -4) * sizeof(LONG));
    }
    if (m_plDC[i])
      m_pEnviron->FreeMem(m_plDC[i],((m_ulWidth[i] + 3) >> 2) * sizeof(LONG));
  }

  /*
  printf("Total symbols coded: %d\n",m_iTotal);
  printf("Percentage of significant symbols: %g\n",m_iSignificant * 100.0 / m_iTotal);
  printf("Percentage of symbols becoming significant: %g\n",m_iGetsSignificant * 100.0 / m_iTotal);
  printf("Percentage of DC symbols becoming significant: %g\n",m_iDCGetsSignificant * 100.0 / m_iTotal);
  printf("Percentage of symbols becomming significant behind significant symbols: %g\n",m_iGetsSignificantAfterSignificance * 100.0 / m_iTotal);
  printf("Percentage of Zero tree coefficients: %g\n",m_iZeroTree * 100.0 / m_iTotal);
  printf("Percentage of DC coefficients starting a ZT: %g\n",m_iDCStartsZeroTree * 100.0 / m_iTotal);
  printf("Percentage of Zero tree coefficients behind significant symbols: %g\n",m_iZeroTreeAfterSignificance * 100.0 / m_iTotal);
  printf("Percentage of zero tree coefficients behind significant DC symbols: %g\n",m_iZeroTreeAfterDC * 100.0 /m_iTotal);
  printf("Percentage of isolated zeros: %g\n",m_iIsolatedZero * 100.0 / m_iTotal);
  printf("Percentage of isolated zeros behind significant symbols: %g\n",m_iIsolatedZeroAfterSignificance * 100.0 / m_iTotal);
  printf("Percentage of isolated zeros behind significant DC symbols: %g\n",m_iIsolatedZeroAfterDC * 100.0 / m_iTotal);
  */
}
///


/// VesaDCTScan::FindComponentDimensions
// Collect component information and install the component dimensions.
// Also called (indirectly) to start writing or parsing a new scan.
void VesaDCTScan::FindComponentDimensions(void)
{ 
  UWORD restart = m_pFrame->TablesOf()->RestartIntervalOf();
  UBYTE cx;
  ULONG y;
   
  JPEGLSScan::FindComponentDimensions();

  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();
  LONG max       = ((m_lMaxVal + 1) << preshift) - 1;

  if (m_ucCount > 4)
    JPG_THROW(OVERFLOW_PARAMETER,"VesaDCTScan::FindComponentDimensions",
	      "Vesa DCT scan does not support more than four components");
  
  //
  // Check that all MCU dimensions are 1.
  for(cx = 0;cx < m_ucCount;cx++) {
    class Component *comp = ComponentOf(cx);
    if (comp->MCUHeightOf() != 1 || comp->MCUWidthOf() != 1)
      JPG_THROW(INVALID_PARAMETER,"VesaDCTScan::FindComponentDimensions",
		"sample interleaved JPEG LS does not support subsampling");
    m_ucDepth[cx]   = comp->PrecisionOf();
    for(y = 0;y < 4;y++) {
      m_plBuffer[cx][y] = (LONG *)m_pEnviron->AllocMem(((m_ulWidth[cx] + 3) & -4) * sizeof(LONG));
    }
    m_plDC[cx] = (LONG *)m_pEnviron->AllocMem(((m_ulWidth[cx] + 3) >> 2) * sizeof(LONG));
    for(y = 0;y < m_ulWidth[cx];y += 4) {
      m_plDC[cx][y >> 2] = DCNeutralValue(max + 1);
    }
  }

  m_ulSamplesPerLine = 0;
  m_ulMaxOvershoot   = 0;
  m_ulUsedBits       = 0;
  for(cx = 0;cx < m_ucCount;cx++) {
    m_ulSamplesPerLine += m_ulWidth[cx];
    if (m_ulWidth[cx] << 1 > m_ulMaxOvershoot)
      m_ulMaxOvershoot = m_ulWidth[cx] << 1;
  }
  m_ulBandwidth      = (8 * m_lNear * m_ulSamplesPerLine * 4) / 100;
  if (restart)
    m_ulBandwidth   /= restart;
  m_ulBitBudget      = m_ulBandwidth;
}
///

/// VesaDCTScan::UpdateDC
// Update the DC value to what the decoder would reconstruct.
// Bitlevel is the last bitplane that has been coded.
void VesaDCTScan::UpdateDC(UBYTE cx,UBYTE bitlevel,int xstart,int xend)
{
  int x;
  
  for(x = xstart;x < xend;x+=4) { 
    LONG v = m_plBuffer[cx][0][x];
    // Mask out all bitplanes below the coded bitplane.
    v     &= ~((1 << bitlevel) - 1);
    // Include the 0.5 reconstruction point if v is nonzero.
    if (v & ValueMask) {
      v   |= (1 << bitlevel) >> 1;
    }
    if (v & Signed) {
      m_plDC[cx][x >> 2] -= (v & ValueMask);
    } else {
      m_plDC[cx][x >> 2] += (v & ValueMask);
    }
    /*
    if (cx == 0 && x == 0) 
      printf("%d %d\n",x,m_plDC[cx][x >> 2]);
    */
  }
  /*
  if (cx == 0) 
    printf("\n");
  */
}
///

/// VesaDCTScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more
// MCUs in this row.
bool VesaDCTScan::ParseMCU(void)
{
  int lines             = m_ulRemaining[0]; // total number of MCU lines processed.
  bool second           = false;
  UWORD restart         = m_pFrame->TablesOf()->RestartIntervalOf();
  int xstart[4];
  int xend[4];          // Start and end of the precincts.
  int precwidth[4];
  UBYTE cx;
  UBYTE bits[4];
  UBYTE maxbits;
  int x,y;
  
  //
  // A "MCU" in respect to the code organization is eight lines.
  if (lines > 8) {
    lines = 8;
  }
  m_ulRemaining[0] -= lines;
  assert(lines > 0);
  assert(m_ucCount < 4);

  lines = (lines + 3) & -4;

  //
  // Loop over the lines.
  do {
    for(cx = 0;cx < m_ucCount;cx++) {
      if (restart == 0) {
	precwidth[cx] = m_ulWidth[cx];
      } else {
	precwidth[cx] = (m_ulWidth[cx] + restart - 1) / restart;
      }
      xstart[cx] = 0;
    }
    //
    // Loop over the precincts.
    bool more = false;
    do {
      // Compute the end of the current precinct.
      for(cx = 0;cx < m_ucCount;cx++) {
	xend[cx] = xstart[cx] + precwidth[cx];
	if (xend[cx] > LONG(m_ulWidth[cx]))
	  xend[cx] = m_ulWidth[cx];
	xend[cx] = (xend[cx] + 3) & -4;
      }
      //
      // Get the max bit counter from the stream. This is always over a precinct
      // and all components.
      UBYTE m = 0;
      maxbits = 0;
      for(cx = 0;cx < m_ucCount;cx++) {
	m = 0;
	while((m_Stream.Get<1>()))
	  m++;
	bits[cx] = m;
	if (m > maxbits)
	  maxbits = m;
	//
	// Clear the data.
	for(y = 0;y < 4;y++) {
	  memset(m_plBuffer[cx][y] + xstart[cx],0,(xend[cx] - xstart[cx]) * sizeof(LONG));
	}
      }
      //
      // Receive the bits.
      {
	bool abort          = false;
	UBYTE bitlevel;
	assert(m_ucCount < 4);
	//
	bitlevel = maxbits;
	while(bitlevel && abort == false) {
	  ULONG level  = 0; // Position within the DCT scan pattern.
	  for(cx = 0;cx < m_ucCount;cx++) {
	    for(y = 0;y < 4;y++) {
	      ClearEncodedFlags((ULONG *)(m_plBuffer[cx][y] + xstart[cx]),xend[cx] - xstart[cx]);
	    }
	  }
	  bitlevel--;
	  do {
	    for(cx = 0;cx < m_ucCount;cx++) {
	      if (bitlevel < bits[cx]) {
		// Enough data available?
		if (m_ulUsedBits + (((xend[cx] - xstart[cx]) >> 2) << 2) >= m_ulBitBudget) {
		  abort = true;
		  break;
		} else {
		  for(x = xstart[cx];x < xend[cx];x += 4) {
		    DecodeEZWLevel(cx,x,1UL << bitlevel,level);
		    //printf("%d:%d\n",x,m_ulUsedBits);
		  }
		}
	      }
	    }
	  } while(abort == false && ++level < 16);
	}
      }
      //
      // Advance to the next precinct.
      more = false;
      for(cx = 0;cx < m_ucCount;cx++) {
	if (xend[cx] > xstart[cx]) {
	  more = true;
	  xstart[cx] = xend[cx];
	}
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
      // Check for a restart marker. This is a simple 0xff byte here.
      if (restart) {
	m_Stream.SkipStuffing();
	if (m_Stream.ByteStreamOf()->Get() != 0xff) {
	  JPG_WARN(MALFORMED_STREAM,"VesaDCTScan::ParseMCU",
		   "missing synchronization byte, trying to resync");
	  while(m_Stream.ByteStreamOf()->Get() != 0xff){}
	}
	m_Stream.OpenForRead(m_Stream.ByteStreamOf());
      }
      //
      // Repeat until there are no more precincts on the line.
    } while(more);
    //
    // Inverse transform the data.
    for(cx = 0;cx < m_ucCount;cx++) {
      struct Line *line = CurrentLine(cx);
      //
      // Convert from sign-magnitude to two's complement.
      for(y = 0;y < 4;y++) {
	for(x = 0;x < LONG((m_ulWidth[cx] + 3) & -4);x++) {
	  LONG &v = m_plBuffer[cx][y][x];
	  if (v & Signed) {
	    v = -(v & ValueMask);
	  } else {
	    v &= ValueMask;
	  }
	}
      }
      //
      for(x = 0;x < LONG(m_ulWidth[cx]);x+=4) {
	// DC shift.
	m_plBuffer[cx][0][x] += m_plDC[cx][x >> 2];
	m_plDC[cx][x >> 2]    = m_plBuffer[cx][0][x];
	/*
	  if (cx == 0 && x == 0) 
	  printf("%d %d\n",x,m_plDC[cx][x >> 2]);
	*/
	BackwardDCT(m_plBuffer[cx][0]+x,m_plBuffer[cx][1]+x,
		    m_plBuffer[cx][2]+x,m_plBuffer[cx][3]+x);
      }
      /*
	if (cx == 0) 
	printf("\n");
      */
      //
      // Forward by four lines in the lower pair.
      if (second) {
	for(y = 0;y < 4;y++) {
	  if (line)
	    line = line->m_pNext;
	}
      }
      //
      // Copy the data out.
      for(y = 0;y < 4;y++) {
	if (line) {
	  memcpy(line->m_pData,m_plBuffer[cx][y],m_ulWidth[cx] * sizeof(LONG));
	  line = line->m_pNext;
	}
      }
    }
    //
    // Go to the next line
    lines           -= 4;
    second           = true;
  } while(lines);
  
  return false;
}
///

/// VesaDCTScan::WriteMCU
// Write a single MCU in this scan.
bool VesaDCTScan::WriteMCU(void)
{
  int lines             = m_ulRemaining[0]; // total number of MCU lines processed.
  bool second           = false;
  UWORD restart         = m_pFrame->TablesOf()->RestartIntervalOf();
  int precwidth[4];   // size of a precinct in pixels.
  int xstart[4];      // start X position of the current precinct.
  int xend[4];        // end of the precinct.
  UBYTE cx;
  UBYTE bits[4];
  UBYTE maxbits;
  int x,y;
  
  //
  // A "MCU" in respect to the code organization is eight lines.
  if (lines > 8) {
    lines = 8;
  }
  m_ulRemaining[0] -= lines;
  assert(lines > 0);
  assert(m_ucCount < 4);

  lines = (lines + 3) & -4;

  do {
    //
    // Fill the line pointers.
    for(cx = 0;cx < m_ucCount;cx++) {
      struct Line *line = CurrentLine(cx);
      if (second) {
	for(y = 0;y < 4;y++) {
	  if (line->m_pNext)
	    line = line->m_pNext;
	}
      }
      //
      // Compute the precinct width and initialize the start
      // position of the precinct.
      if (restart == 0) {
	precwidth[cx] = m_ulWidth[cx];
      } else {
	precwidth[cx] = (m_ulWidth[cx] + restart - 1) / restart;
      }
      xstart[cx] = 0;
      //
      for(y = 0;y < 4;y++) {
	LONG last;
	memcpy(m_plBuffer[cx][y],line->m_pData,m_ulWidth[cx] * sizeof(LONG));
	last = m_plBuffer[cx][y][m_ulWidth[cx] - 1];
	switch(m_ulWidth[cx] & 3) {
	case 1:
	  m_plBuffer[cx][y][m_ulWidth[cx]+2] = last;
	case 2:
	  m_plBuffer[cx][y][m_ulWidth[cx]+1] = last;
	case 3:
	  m_plBuffer[cx][y][m_ulWidth[cx]]   = last;	
	case 0:
	  break;
      }
	if (line->m_pNext)
	  line = line->m_pNext;
      }
      for(x = 0;x < LONG(m_ulWidth[cx]);x+=4) {
	ForwardDCT(m_plBuffer[cx][0]+x,m_plBuffer[cx][1]+x,
		   m_plBuffer[cx][2]+x,m_plBuffer[cx][3]+x);
	// DC shift.
	m_plBuffer[cx][0][x] -= m_plDC[cx][x >> 2];
      }
    }
    //
    // Loop over the precincts.
    bool more = false;
    do {
      // Find the maximum for each component and transmit.
      maxbits = 0;
      more    = false;
      for(cx = 0;cx < m_ucCount;cx++) {
	ULONG max = 0;
	//
	// Compute the end position of the precinct.
	xend[cx] = xstart[cx] + precwidth[cx];
	if (xend[cx] > LONG(m_ulWidth[cx]))
	  xend[cx] = m_ulWidth[cx];
	xend[cx] = (xend[cx] + 3) & -4;
	//
	for(y = 0;y < 4;y++) {
	  for(x = xstart[cx];x < xend[cx];x++) {
	    LONG v = m_plBuffer[cx][y][x];
	    if (v < 0) {
	      v = -v;
	      m_plBuffer[cx][y][x] = v | Signed;
	    }
	    if (ULONG(v) > max) max = v;
	  }
	}
	bits[cx] = 0;
	while(max) {
	  m_Stream.Put<1>(1);
	  max >>=1;
	  bits[cx]++;
	  if (bits[cx] > maxbits)
	    maxbits = bits[cx];
	}
	m_Stream.Put<1>(0);
      }
      //
      // Transmit the bits.
      {
	bool abort          = false;
	UBYTE bitlevel;
	assert(m_ucCount < 4);
	//
	bitlevel = maxbits;
	while(bitlevel && abort == false) {
	  ULONG level  = 0; // Position within the DCT scan pattern.
	  for(cx = 0;cx < m_ucCount;cx++) {
	    for(y = 0;y < 4;y++) {
	      ClearEncodedFlags((ULONG *)(m_plBuffer[cx][y] + xstart[cx]),xend[cx] - xstart[cx]);
	    }
	  }
	  bitlevel--;
	  do {
	    for(cx = 0;cx < m_ucCount;cx++) {
	      if (bitlevel < bits[cx]) {
		// Enough data available?
		if (m_ulUsedBits + (((xend[cx] - xstart[cx]) >> 2) << 2) >= m_ulBitBudget) {
		  UBYTE c;
		  abort = true;
		  if (level) {
		    for(c = 0;c < m_ucCount;c++) {
		      UpdateDC(c,bitlevel,xstart[cx],xend[cx]);
		    }
		  } else {
		    for(c = 0;c < cx;c++) {
		      UpdateDC(c,bitlevel,xstart[cx],xend[cx]);
		    }
		    if (bitlevel + 1 < maxbits) {
		      for(c = cx;c < m_ucCount;c++) {
			UpdateDC(c,bitlevel + 1,xstart[cx],xend[cx]);
		      }
		    }
		  }
		  break;
		} else {
		  for(x = xstart[cx];x < xend[cx];x += 4) {
		    EncodeEZWLevel(cx,x,1UL << bitlevel,level);
		    //printf("%d:%d\n",x,m_ulUsedBits);
		  }
		}
	      }
	    }
	  } while(abort == false && ++level < 16);
	}
	if (abort == false) {
	  for(cx = 0;cx < m_ucCount;cx++) {
	    UpdateDC(cx,0,xstart[cx],xend[cx]);
	  }
	}
      }
      //
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
      // Advance to the next precinct.
      for(cx = 0;cx < m_ucCount;cx++) {
	if (xend[cx] > xstart[cx]) {
	  more = true;
	  xstart[cx] = xend[cx];
	}
      }
      //
      // Write a restart indicator - here a simple 0xff byte.
      if (restart) {
	m_Stream.Flush();
	m_Stream.ByteStreamOf()->Put(0xff);
      }
    } while(more);
    //
    lines           -= 4;
    second           = true;
  } while(lines);

  return false;
}
///
