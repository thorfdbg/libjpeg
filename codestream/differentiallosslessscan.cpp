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
** Represents the lossless scan - lines are coded directly with predictive
** coding, just no prediction takes place here for the differential
** lossless scan.
**
** $Id: differentiallosslessscan.cpp,v 1.8 2012-07-20 23:49:08 thor Exp $
**
*/

/// Includes
#include "codestream/differentiallosslessscan.hpp"
#include "io/bytestream.hpp"
#include "control/linebuffer.hpp"
#include "control/linebitmaprequester.hpp"
#include "control/lineadapter.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "io/bitstream.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
#include "codestream/tables.hpp"
#include "tools/line.hpp"
///

/// DifferentialLosslessScan::DifferentialLosslessScan
DifferentialLosslessScan::DifferentialLosslessScan(class Frame *frame,class Scan *scan,UBYTE lowbit)
  : EntropyParser(frame,scan), m_pLineCtrl(NULL), m_ucLowBit(lowbit)
{ 
  m_ucCount = scan->ComponentsInScan();
  
  for(int i = 0;i < 4;i++) {
    m_pDCDecoder[i]    = NULL;
    m_pDCCoder[i]      = NULL;
    m_pDCStatistics[i] = NULL;
  }
}
///

/// DifferentialLosslessScan::FindComponentDimensions
// Collect the component information.
void DifferentialLosslessScan::FindComponentDimensions(void)
{ 
  int i;

  m_ulPixelWidth  = m_pFrame->WidthOf();
  m_ulPixelHeight = m_pFrame->HeightOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE subx = comp->SubXOf();
    UBYTE suby = comp->SubYOf();
    
    m_ulWidth[i]  = (m_ulPixelWidth  + subx - 1) / subx;
    m_ulHeight[i] = (m_ulPixelHeight + suby - 1) / suby;
  }
}
///

/// DifferentialLosslessScan::~DifferentialLosslessScan
DifferentialLosslessScan::~DifferentialLosslessScan(void)
{
}
///

/// DifferentialLosslessScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void DifferentialLosslessScan::WriteFrameType(class ByteStream *io)
{
  io->PutWord(0xffc7); // differential lossless sequential
}
///

/// DifferentialLosslessScan::StartParseScan
void DifferentialLosslessScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCDecoder[i]  = m_pScan->DCHuffmanDecoderOf(i);
  }
  
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  m_Stream.OpenForRead(io);
}
///

/// DifferentialLosslessScan::StartWriteScan
void DifferentialLosslessScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCCoder[i]       = m_pScan->DCHuffmanCoderOf(i);
    m_pDCStatistics[i]  = NULL;
  }
  
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan); 

  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io); 

  m_bMeasure = false;
}
///

/// DifferentialLosslessScan::StartMeasureScan
void DifferentialLosslessScan::StartMeasureScan(class BufferCtrl *ctrl)
{
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCCoder[i]       = NULL;
    m_pDCStatistics[i]  = m_pScan->DCHuffmanStatisticsOf(i);
  }
 
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);

  m_bMeasure = true;
}
///

/// DifferentialLosslessScan::WriteMCU
// Write a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool DifferentialLosslessScan::WriteMCU(void)
{
  int i;
  struct Line *top[4];
  ULONG xpos[4],ypos[4];
  UBYTE mcuwidth[4],mcuheight[4];
  bool more;
  int lines = 8; // total number of MCU lines processed.
  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    xpos[i]         = 0;
    ypos[i]         = m_pLineCtrl->CurrentYOf(idx);
    mcuwidth[i]     = comp->MCUWidthOf();
    mcuheight[i]    = comp->MCUHeightOf();
    m_bNoPrediction = false;
  }

  // Loop over lines and columns
  do {
    BeginWriteMCU(m_Stream.ByteStreamOf());
    do {      
      // Encode a single MCU, which is now a group of pixels.
      for(i = 0;i < m_ucCount;i++) {
	class HuffmanCoder *dc          = m_pDCCoder[i];
	class HuffmanStatistics *dcstat = m_pDCStatistics[i];
	struct Line *line = top[i];
	UBYTE ym = mcuheight[i];
	LONG  x  = xpos[i];
	LONG *lp = line->m_pData + x;
	//
	// Predicted value at the left. Either taken from the left if there
	// is a left, or from the top if there is a top, or the default
	// value.
	// Encude MCUwidth * MCUheight coefficients starting at the line top.
	do {
	  UBYTE xm = mcuwidth[i];
	  do {
	    WORD v;
	    //
	    v = lp[0] >> preshift; // value to be encoded. Standard requires 16 bit modulo arithmetic.
	    if (dcstat) {
	      if (v == 0) {
		dcstat->Put(0);
	      } else if (v == -32768) {
		dcstat->Put(16); // Do not append bits
	      } else {
		UBYTE symbol = 0;
		do {
		  symbol++;
		  if (v > -(1 << symbol) && v < (1 << symbol)) {
		    dcstat->Put(symbol);
		    break;
		  }
		} while(true);
	      }
	    } else {
	      if (v == 0) {
		dc->Put(&m_Stream,0);
	      } else if (v == -32768) {
		dc->Put(&m_Stream,16); // Do not append bits
	      } else {
		UBYTE symbol = 0;
		do {
		  symbol++;
		  if (v > -(1 << symbol) && v < (1 << symbol)) {
		    dc->Put(&m_Stream,symbol);
		    if (v >= 0) {
		      m_Stream.Put(symbol,v);
		    } else {
		      m_Stream.Put(symbol,v - 1);
		    }
		    break;
		  }
		} while(true);
	      }
	    }
	    //
	    // One pixel done. Proceed to the next in the MCU. Note that
	    // the lines have been extended such that always a complete MCU is present.
	    lp++,x++;
	  } while(--xm);
	  //
	  // Go to the next line.
	  x  = x  - mcuwidth[i];
	  if (line->m_pNext)
	    line = line->m_pNext;
	  lp = line->m_pData + x;
	} while(--ym);
      }
      //
      // End MCU coding. Proceed to next MCU.
      for(i = 0,more = true;i < m_ucCount;i++) {
	xpos[i] += mcuwidth[i];
	if (xpos[i] >= m_ulWidth[i])
	  more = false;
      }
    } while(more);
    //
    // Advance to the next line.
    for(i = 0,more = true;i < m_ucCount;i++) {
      int cnt = mcuheight[i];
      xpos[i]  = 0;
      ypos[i] += cnt;
      if (ypos[i] >= m_ulHeight[i]) {
	more = false;
      } else do {
	if (top[i]->m_pNext) {
	  top[i] = top[i]->m_pNext;
	}
      } while(--cnt);
    }
    m_bNoPrediction = false;
  } while(--lines && more);
  
  return false; // no further blocks here.
}
///

/// DifferentialLosslessScan::ParseMCU
// Parse a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool DifferentialLosslessScan::ParseMCU(void)
{
  int i;
  struct Line *top[4];
  ULONG xpos[4],ypos[4];
  UBYTE mcuwidth[4],mcuheight[4];
  bool more;
  int lines = 8; // total number of MCU lines processed.
  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    xpos[i]         = 0;
    ypos[i]         = m_pLineCtrl->CurrentYOf(idx);
    mcuwidth[i]     = comp->MCUWidthOf();
    mcuheight[i]    = comp->MCUHeightOf();
    m_bNoPrediction = false;
  }

  // Loop over lines and columns
  do {
    if (BeginReadMCU(m_Stream.ByteStreamOf()) == false) {
      ClearMCU(top);
    } else do {
      // Encode a single MCU, which is now a group of pixels.
      for(i = 0;i < m_ucCount;i++) {
	class HuffmanDecoder *dc = m_pDCDecoder[i];
	struct Line *line = top[i];
	UBYTE ym = mcuheight[i];
	LONG  x  = xpos[i];
	LONG *lp = line->m_pData + x;
	//
	// Predicted value at the left. Either taken from the left if there
	// is a left, or from the top if there is a top, or the default
	// value.
	// Encude MCUwidth * MCUheight coefficients starting at the line top.
	do {
	  UBYTE xm = mcuwidth[i];
	  do {
	    //
	    // Decode now the difference between the predicted value and
	    // the real value.
	    {
	      WORD v; // 16 bit integer arithmetic required.
	      // NOTE: C++ does actually not ensure that v has wraparound-characteristics, though it
	      // typically does.
	      UBYTE symbol = dc->Get(&m_Stream);
	      
	      if (symbol == 0) {
		v = 0;
	      } else if (symbol == 16) {
		v = -32768;
	      } else {
		LONG thre = 1L << (symbol - 1);
		LONG diff = m_Stream.Get(symbol); // get the number of bits 
		if (diff < thre) {
		  diff += (-1L << symbol) + 1;
		}
		v = diff;
	      }
	      //
	      // Set the current pixel, do the inverse pointwise transformation.
	      lp[0] = v << preshift;
	    }
	    //
	    // One pixel done. Proceed to the next in the MCU. Note that
	    // the lines have been extended such that always a complete MCU is present.
	    lp++;x++;
	  } while(--xm);
	  //
	  // Go to the next line.
	  x  = x  - mcuwidth[i];
	  if (line->m_pNext)
	    line = line->m_pNext;
	  lp = line->m_pData + x;
	} while(--ym);
      }
      //
      // End MCU coding. Proceed to next MCU.
      for(i = 0,more = true;i < m_ucCount;i++) {
	xpos[i] += mcuwidth[i];
	if (xpos[i] >= m_ulWidth[i])
	  more = false;
      }
    } while(more);
    //
    // Advance to the next line.
    for(i = 0,more = true;i < m_ucCount;i++) {
      int cnt = mcuheight[i];
      xpos[i]  = 0;
      ypos[i] += cnt;
      if (m_ulHeight[i] && ypos[i] >= m_ulHeight[i]) {
	more = false;
      } else do {
	if (top[i]->m_pNext) {
	  top[i] = top[i]->m_pNext;
	}
      } while(--cnt);
    }
    m_bNoPrediction = false;
  } while(--lines && more);
  
  return false; // no further blocks here.
}
///

/// DifferentialLosslessScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool DifferentialLosslessScan::StartMCURow(void)
{
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
}
///

/// DifferentialLosslessScan::Flush
void DifferentialLosslessScan::Flush(void)
{ 
  if (!m_bMeasure)
    m_Stream.Flush();
  m_bNoPrediction = true;
}
///

/// DifferentialLosslessScan::Restart
// Restart the parser at the next restart interval
void DifferentialLosslessScan::Restart(void)
{ 
  m_Stream.OpenForRead(m_Stream.ByteStreamOf());
  m_bNoPrediction = true;
}
///

/// DifferentialLosslessScan::ClearMCU
// Clear the entire MCU
void DifferentialLosslessScan::ClearMCU(class Line **top)
{
  for(int i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    struct Line *line     = top[i];
    UBYTE ym              = comp->MCUHeightOf();
    //
    do {
      memset(line->m_pData,0,m_ulWidth[i] * sizeof(LONG));
      if (line->m_pNext)
	line = line->m_pNext;
    } while(--ym);
  }
}
///
