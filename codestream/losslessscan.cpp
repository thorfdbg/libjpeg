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
**
** Represents the scan including the scan header.
**
** $Id: losslessscan.cpp,v 1.28 2012-07-20 23:49:08 thor Exp $
**
*/

/// Includes
#include "codestream/losslessscan.hpp"
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

/// LosslessScan::LosslessScan
LosslessScan::LosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit)
  : EntropyParser(frame,scan), m_pLineCtrl(NULL), m_ucPredictor(predictor), m_ucLowBit(lowbit)
{ 
  m_ucCount = scan->ComponentsInScan();
  
  for(int i = 0;i < 4;i++) {
    m_pDCDecoder[i]    = NULL;
    m_pDCCoder[i]      = NULL;
    m_pDCStatistics[i] = NULL;
  }
}
///

/// LosslessScan::FindComponentDimensions
// Collect the component information.
void LosslessScan::FindComponentDimensions(void)
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

/// LosslessScan::~LosslessScan
LosslessScan::~LosslessScan(void)
{
}
///

/// LosslessScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void LosslessScan::WriteFrameType(class ByteStream *io)
{
  io->PutWord(0xffc3); // lossless sequential
}
///

/// LosslessScan::StartParseScan
void LosslessScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
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

/// LosslessScan::StartWriteScan
void LosslessScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
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

/// LosslessScan::StartMeasureScan
void LosslessScan::StartMeasureScan(class BufferCtrl *ctrl)
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

/// LosslessScan::WriteMCU
// Write a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool LosslessScan::WriteMCU(void)
{
  int i;
  struct Line *top[4],*prev[4];
  ULONG xpos[4],ypos[4];
  UBYTE mcuwidth[4],mcuheight[4];
  bool more;
  int lines = 8; // total number of MCU lines processed.
  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    prev[i]         = m_pLineCtrl->PreviousLineOf(idx);
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
      //
      // Encode a single MCU, which is now a group of pixels.
      for(i = 0;i < m_ucCount;i++) {
	class HuffmanCoder *dc          = m_pDCCoder[i];
	class HuffmanStatistics *dcstat = m_pDCStatistics[i];
	struct Line *line = top[i];
	struct Line *pline= prev[i];
	UBYTE ym = mcuheight[i];
	LONG  x  = xpos[i];
	LONG  y  = (m_bNoPrediction)?(0):(ypos[i]);
	LONG *lp = line->m_pData + x;
	LONG *pp = (pline)?(pline->m_pData + x):(NULL);
	//
	// Predicted value at the left. Either taken from the left if there
	// is a left, or from the top if there is a top, or the default
	// value.
	// Encude MCUwidth * MCUheight coefficients starting at the line top.
	do {
	  UBYTE xm = mcuwidth[i];
	  do {
	    LONG pred;
	    WORD v;
	    // Get the predicted value.
	    if (x == 0) { 
	      if (y == 0) {
		pred = ((1L << m_pFrame->PrecisionOf()) >> 1) >> m_ucLowBit;
	      } else {
		pred = pp[0] >> preshift; // the line on top, or the default on the left edge.
	      }
	    } else if (y == 0) {
	      pred = lp[-1] >> preshift; // at the first line, predict from the left.
	    } else switch(m_ucPredictor) { // Use the user-defined predictor.
	      case 0: // No prediction. Actually, this is invalid here.
		JPG_THROW(INVALID_PARAMETER,"LosslessScan::WriteMCU",
			  "a lossless non-differential scan must use prediction, but prediction is turned off");
		break;
	      case 1:
		pred = lp[-1] >> preshift; // predict from the left.
		break;
	      case 2:
		pred = pp[0] >> preshift;  // predict from the top
		break;
	      case 3:
		pred = pp[-1] >> preshift; // predict from the left top
		break;
	      case 4:
		pred = (lp[-1]  >> preshift) + (pp[0]  >> preshift) - (pp[-1] >> preshift); // linear interpolation.
		break;
	      case 5:
		pred = (lp[-1]  >> preshift) + (((pp[0]  >> preshift) - (pp[-1] >> preshift)) >> 1);
		break;
	      case 6:
		pred = (pp[0]  >> preshift) + (((lp[-1]  >> preshift) - (pp[-1] >> preshift)) >> 1);
		break;
	      case 7:
		pred = ((lp[-1] >> preshift) + (pp[0] >> preshift)) >> 1;
		break;
	      default:
		JPG_THROW(INVALID_PARAMETER,"LosslessScan::WriteMCU",
			  "invalid predictor selected for the lossless mode");
		break;
	      }
	    v = ((lp[0] >> preshift) - pred); // value to be encoded. Standard requires 16 bit modulo arithmetic.
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
	    lp++,pp++;x++;
	  } while(--xm);
	  //
	  // Go to the next line.
	  x  = x  - mcuwidth[i];
	  pp = lp - mcuwidth[i];
	  if (line->m_pNext)
	    line = line->m_pNext;
	  lp = line->m_pData + x;
	  y++;
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
	prev[i] = top[i];
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

/// LosslessScan::ParseMCU
// Parse a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool LosslessScan::ParseMCU(void)
{
  int i;
  struct Line *top[4],*prev[4];
  ULONG xpos[4],ypos[4];
  UBYTE mcuwidth[4],mcuheight[4];
  bool more;
  int lines = 8; // total number of MCU lines processed.
  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    prev[i]         = m_pLineCtrl->PreviousLineOf(idx);
    xpos[i]         = 0;
    ypos[i]         = m_pLineCtrl->CurrentYOf(idx);
    mcuwidth[i]     = comp->MCUWidthOf();
    mcuheight[i]    = comp->MCUHeightOf();
    m_bNoPrediction = false;
  }

  // Loop over lines and columns
  do {
    if (BeginReadMCU(m_Stream.ByteStreamOf()) == false) {
      // An error, clear the entire MCU here.
      ClearMCU(top);
    } else do {
      // Encode a single MCU, which is now a group of pixels.
      for(i = 0;i < m_ucCount;i++) {
	class HuffmanDecoder *dc = m_pDCDecoder[i];
	struct Line *line = top[i];
	struct Line *pline= prev[i];
	UBYTE ym = mcuheight[i];
	LONG  x  = xpos[i];
	LONG  y  = (m_bNoPrediction)?(0):(ypos[i]);
	LONG *lp = line->m_pData + x;
	LONG *pp = (pline)?(pline->m_pData + x):(NULL);
	//
	// Predicted value at the left. Either taken from the left if there
	// is a left, or from the top if there is a top, or the default
	// value.
	// Encude MCUwidth * MCUheight coefficients starting at the line top.
	do {
	  UBYTE xm = mcuwidth[i];
	  do {
	    LONG pred;
	    // Get the predicted value.
	    if (x == 0) {
	      if (y == 0) {
		pred = ((1L << m_pFrame->PrecisionOf()) >> 1) >> m_ucLowBit;
	      } else {
		pred = pp[0] >> preshift; // the line on top, or the default on the left edge.
	      }
	    } else if (y == 0) {
	      pred = lp[-1] >> preshift; // at the first line, predict from the left.
	    } else switch(m_ucPredictor) { // Use the user-defined predictor.
	      case 0: // No prediction. Actually, this is invalid here.
		JPG_THROW(INVALID_PARAMETER,"LosslessScan::ParseMCU",
			  "a lossless non-differential scan must use prediction, but prediction is turned off");
		break;
	      case 1:
		pred = lp[-1] >> preshift; // predict from the left.
		break;
	      case 2:
		pred = pp[0] >> preshift;  // predict from the top
		break;
	      case 3:
		pred = pp[-1] >> preshift; // predict from the left top
		break;
	      case 4:
		pred = (lp[-1]  >> preshift) + (pp[0]  >> preshift) - (pp[-1] >> preshift); // linear interpolation.
		break;
	      case 5:
		pred = (lp[-1]  >> preshift) + (((pp[0]  >> preshift) - (pp[-1] >> preshift)) >> 1);
		break;
	      case 6:
		pred = (pp[0]  >> preshift) + (((lp[-1]  >> preshift) - (pp[-1] >> preshift)) >> 1);
		break;
	      case 7:
		pred = ((lp[-1] >> preshift) + (pp[0] >> preshift)) >> 1;
		break;
	      default:
		JPG_THROW(INVALID_PARAMETER,"LosslessScan::ParseMCU",
			  "invalid predictor selected for the lossless mode");
		break;
	      }
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
	      lp[0] = (v + pred) << preshift;
	    }
	    //
	    // One pixel done. Proceed to the next in the MCU. Note that
	    // the lines have been extended such that always a complete MCU is present.
	    lp++,pp++;x++;
	  } while(--xm);
	  //
	  // Go to the next line.
	  x  = x  - mcuwidth[i];
	  pp = lp - mcuwidth[i];
	  if (line->m_pNext)
	    line = line->m_pNext;
	  lp = line->m_pData + x;
	  y++;
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
      int cnt  = mcuheight[i];
      xpos[i]  = 0;
      ypos[i] += cnt;
      if (m_ulHeight[i] && ypos[i] >= m_ulHeight[i]) {
	more = false;
      } else do {
	prev[i] = top[i];
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

/// LosslessScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool LosslessScan::StartMCURow(void)
{
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
}
///

/// LosslessScan::Flush
// Flush the remaining bits out to the stream on writing.
void LosslessScan::Flush(void)
{  
  if (!m_bMeasure)
    m_Stream.Flush();
  m_bNoPrediction = true;
}
///

/// LosslessScan::Restart
// Restart the parser at the next restart interval
void LosslessScan::Restart(void)
{ 
  m_Stream.OpenForRead(m_Stream.ByteStreamOf());
  m_bNoPrediction = true;
}
///

/// LosslessScan::ClearMCU
// Clear the entire MCU
void LosslessScan::ClearMCU(class Line **top)
{ 
  UBYTE preshift = FractionalColorBitsOf();
  LONG neutral   = ((1L << m_pFrame->PrecisionOf()) >> 1) << preshift;
  
  for(int i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    struct Line *line     = top[i];
    UBYTE ym              = comp->MCUHeightOf();
    //
    do {
      LONG *p = line->m_pData;
      LONG *e = line->m_pData + m_ulWidth[i];
      do {
	*p = neutral;
      } while(++p < e);

      if (line->m_pNext)
	line = line->m_pNext;
    } while(--ym);
  }
}
///
