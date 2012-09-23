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
** $Id: losslessscan.cpp,v 1.33 2012-09-22 20:51:40 thor Exp $
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
LosslessScan::LosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,bool differential)
  : PredictiveScan(frame,scan,predictor,lowbit,differential)
{ 
  for(int i = 0;i < 4;i++) {
    m_pDCDecoder[i]    = NULL;
    m_pDCCoder[i]      = NULL;
    m_pDCStatistics[i] = NULL;
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
  if (m_bDifferential) {
    io->PutWord(0xffc7); // differential lossless sequential
  } else {
    io->PutWord(0xffc3); // lossless sequential
  }
}
///

/// LosslessScan::StartParseScan
void LosslessScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCDecoder[i]       = m_pScan->DCHuffmanDecoderOf(i);
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
  int lines      = 8; // total number of MCU lines processed.
  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    prev[i]         = m_pLineCtrl->PreviousLineOf(idx);
    m_ulX[i]        = 0;
    m_ulY[i]        = m_pLineCtrl->CurrentYOf(idx);
  }

  // Loop over lines and columns
  do {
    do {
      BeginWriteMCU(m_Stream.ByteStreamOf());
      //
      if (m_bMeasure) {
	MeasureMCU(prev,top,preshift);
      } else {
	WriteMCU(prev,top,preshift);
      }
    } while(AdvanceToTheRight());
    //
    // Turn on prediction again after the restart line.
    m_bNoPrediction = false;
    //
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);

  return false;
}
///

/// LosslessScan::WriteMCU
// The actual MCU-writer, write a single group of pixels to the stream,
// or measure their statistics.
void LosslessScan::WriteMCU(struct Line **prev,struct Line **top,UBYTE preshift)
{
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanCoder *dc = m_pDCCoder[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    UBYTE ym = m_ucMCUHeight[i];
    ULONG  x = m_ulX[i];
    ULONG  y = m_ulY[i];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    if (m_bNoPrediction) {
      y = 0;
    }
    //
    // Write MCUwidth * MCUheight coefficients starting at the line top.
    do {
      UBYTE xm     = m_ucMCUWidth[i];
      do {
	UBYTE mode = PredictionMode(x,y);
	LONG pred  = PredictSample(mode,preshift,lp,pp);
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v;  // 16 bit integer arithmetic required.
	//
	v = WORD((lp[0] >> preshift) - pred); // value to be encoded. Standard requires 16 bit modulo arithmetic.
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
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
	lp++,pp++;x++;
      } while(--xm);
      //
      // Go to the next line.
      x  = x  - m_ucMCUWidth[i];
      pp = lp - m_ucMCUWidth[i];
      if (line->m_pNext)
	line = line->m_pNext;
      lp = line->m_pData + x;
      y++;
    } while(--ym);
  }
}
///

/// LosslessScan::MeasureMCU
// The actual MCU-writer, write a single group of pixels to the stream,
// or measure their statistics. This here only measures the statistics
// to design an optimal Huffman table
void LosslessScan::MeasureMCU(struct Line **prev,struct Line **top,UBYTE preshift)
{
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanStatistics *dcstat = m_pDCStatistics[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    UBYTE ym = m_ucMCUHeight[i];
    ULONG  x = m_ulX[i];
    ULONG  y = m_ulY[i];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    if (m_bNoPrediction) {
      y = 0;      
    }
    //
    // Write MCUwidth * MCUheight coefficients starting at the line top.
    do {
      UBYTE xm     = m_ucMCUWidth[i];
      do {
	UBYTE mode = PredictionMode(x,y);
	LONG pred  = PredictSample(mode,preshift,lp,pp);
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v;  // 16 bit integer arithmetic required.
	//
	v = WORD((lp[0] >> preshift) - pred); // value to be encoded. Standard requires 16 bit modulo arithmetic.
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
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
	lp++,pp++;x++;
      } while(--xm);
      //
      // Go to the next line.
      x  = x  - m_ucMCUWidth[i];
      pp = lp - m_ucMCUWidth[i];
      if (line->m_pNext)
	line = line->m_pNext;
      lp = line->m_pData + x;
      y++;
    } while(--ym);
  }
}
///

/// LosslessScan::ParseMCU
// This is actually the true MCU-parser, not the interface that reads
// a full line.
void LosslessScan::ParseMCU(struct Line **prev,struct Line **top,UBYTE preshift)
{ 
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanDecoder *dc = m_pDCDecoder[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    UBYTE ym = m_ucMCUHeight[i];
    ULONG  x = m_ulX[i];
    ULONG  y = m_ulY[i];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    if (m_bNoPrediction) {
      y = 0;
    }
    //
    // Parse MCUwidth * MCUheight coefficients starting at the line top.
    do {
      UBYTE xm     = m_ucMCUWidth[i];
      do {
	UBYTE mode = PredictionMode(x,y);
	LONG pred  = PredictSample(mode,preshift,lp,pp);
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v;
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
	lp[0] = WORD(v + pred) << preshift;
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
	lp++,pp++;x++;
      } while(--xm);
      //
      // Go to the next line.
      x  = x  - m_ucMCUWidth[i];
      pp = lp - m_ucMCUWidth[i];
      if (line->m_pNext)
	line = line->m_pNext;
      lp = line->m_pData + x;
      y++;
    } while(--ym);
  }
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
  int lines      = 8; // total number of MCU lines processed.
  UBYTE preshift = m_ucLowBit + FractionalColorBitsOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    prev[i]         = m_pLineCtrl->PreviousLineOf(idx);
    m_ulX[i]        = 0;
    m_ulY[i]        = m_pLineCtrl->CurrentYOf(idx);
  }

  // Loop over lines and columns
  do {
    do {
      if (BeginReadMCU(m_Stream.ByteStreamOf())) {
	ParseMCU(prev,top,preshift);
      } else {
	ClearMCU(top);
      }
    } while(AdvanceToTheRight());
    //
    // Turn on prediction again after the restart line.
    m_bNoPrediction = false;
    //
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);
  
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
void LosslessScan::Flush(bool)
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
