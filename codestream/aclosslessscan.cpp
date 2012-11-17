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
** $Id: aclosslessscan.cpp,v 1.26 2012-11-02 20:50:17 thor Exp $
**
*/

/// Includes
#include "codestream/aclosslessscan.hpp"
#include "io/bytestream.hpp"
#include "control/linebuffer.hpp"
#include "control/linebitmaprequester.hpp"
#include "control/lineadapter.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "coding/qmcoder.hpp"
#include "coding/actemplate.hpp"
#include "codestream/tables.hpp"
#include "tools/line.hpp"
#include "std/string.hpp"
///

/// ACLosslessScan::ACLosslessScan
ACLosslessScan::ACLosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,bool differential)
  : PredictiveScan(frame,scan,predictor,lowbit,differential)
{ 
  m_ucCount = scan->ComponentsInScan();
  
  for(int i = 0;i < m_ucCount;i++) {
    m_ucSmall[i]     = 0;
    m_ucLarge[i]     = 1;
  }

  memset(m_plDa,0,sizeof(m_plDa));
  memset(m_plDb,0,sizeof(m_plDb));
}
///

/// ACLosslessScan::~ACLosslessScan
ACLosslessScan::~ACLosslessScan(void)
{
  UBYTE i;
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_plDa[i])
      m_pEnviron->FreeMem(m_plDa[i],sizeof(LONG) * m_ucMCUHeight[i]);
    if (m_plDb[i])
      m_pEnviron->FreeMem(m_plDb[i],sizeof(LONG) * m_ulWidth[i]);
  }
}
///

/// ACLosslessScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ACLosslessScan::WriteFrameType(class ByteStream *io)
{
  if (m_bDifferential) {
    io->PutWord(0xffcf); // differential lossless sequential AC coded
  } else {
    io->PutWord(0xffcb); // lossless sequential AC coded
  }
}
///

/// ACLosslessScan::FindComponentDimensions
// Common setup for encoding and decoding.
void ACLosslessScan::FindComponentDimensions(void)
{
  UBYTE i;

  PredictiveScan::FindComponentDimensions();

  for(i = 0;i < m_ucCount;i++) {
    assert(m_plDa[i] == NULL && m_plDb[i] == NULL);

    m_plDa[i] = (LONG *)(m_pEnviron->AllocMem(sizeof(LONG) * m_ucMCUHeight[i]));
    m_plDb[i] = (LONG *)(m_pEnviron->AllocMem(sizeof(LONG) * m_ulWidth[i]));
  }
}
///

/// ACLosslessScan::StartParseScan
void ACLosslessScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  class ACTemplate *dc;
  int i;

  FindComponentDimensions();

  m_bNoPrediction = true;
  
  for(i = 0;i < m_ucCount;i++) {
    dc = m_pScan->DCConditionerOf(i);
    if (dc) {
      m_ucSmall[i]    = dc->LowerThresholdOf();
      m_ucLarge[i]    = dc->UpperThresholdOf();
    } else {
      m_ucSmall[i]    = 0;
      m_ucLarge[i]    = 1;
    }
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    m_ucContext[i]    = m_pScan->DCTableIndexOf(i); 
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  m_Coder.OpenForRead(io);
}
///

/// ACLosslessScan::StartWriteScan
void ACLosslessScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  class ACTemplate *dc;
  int i;

  FindComponentDimensions();

  m_bNoPrediction = true;
  
  for(i = 0;i < m_ucCount;i++) {
    dc = m_pScan->DCConditionerOf(i);

    if (dc) {
      m_ucSmall[i]    = dc->LowerThresholdOf();
      m_ucLarge[i]    = dc->UpperThresholdOf();
    } else {
      m_ucSmall[i]    = 0;
      m_ucLarge[i]    = 1;
    }  
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    m_ucContext[i]    = m_pScan->DCTableIndexOf(i); 
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
    
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan); 
  
  m_pScan->WriteMarker(io);
  m_Coder.OpenForWrite(io);
}
///

/// ACLosslessScan::StartMeasureScan
void ACLosslessScan::StartMeasureScan(class BufferCtrl *)
{
  JPG_THROW(NOT_IMPLEMENTED,"ACLosslessScan::StartMeasureScan",
	    "arithmetic coding is always adaptive and does not require a measurement phase");
}
///

/// ACLosslessScan::WriteMCU
// This is actually the true MCU-writer, not the interface that reads
// a full line.
void ACLosslessScan::WriteMCU(struct Line **prev,struct Line **top,UBYTE preshift)
{ 
  UBYTE c;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(c = 0;c < m_ucCount;c++) {
    struct QMContextSet &contextset = m_Context[m_ucContext[c]];
    struct Line *line = top[c];
    struct Line *pline= prev[c];
    UBYTE ym = m_ucMCUHeight[c];
    ULONG  x = m_ulX[c];
    ULONG  y = m_ulY[c];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    if (m_bNoPrediction) {
      y           = 0;
      // Reset conditioning to the top
      memset(m_plDb[c] + x,0,sizeof(LONG) * m_ucMCUWidth[c]);
    }
    //
    // Write MCUwidth * MCUheight coefficients starting at the line top.
    do {
      UBYTE xm     = m_ucMCUWidth[c];
      do {
	UBYTE mode = PredictionMode(x,y);
	LONG pred  = PredictSample(mode,preshift,lp,pp);
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v     = WORD((lp[0] >> preshift) - pred); // 16 bit integer arithmetic required.
	// NOTE: C++ does actually not ensure that v has wraparound-characteristics, though it
	// typically does.
	//
	// Get the sign coding context.
	struct QMContextSet::ContextZeroSet &zset = contextset.ClassifySignZero(m_plDa[c][ym-1],m_plDb[c][x],
										m_ucSmall[c],m_ucLarge[c]);
	// 
	if (v) {
	  LONG sz;
	  m_Coder.Put(zset.S0,true);
	  //
	  if (v < 0) {
	    m_Coder.Put(zset.SS,true);
	    sz = -(v + 1);
	  } else {
	    m_Coder.Put(zset.SS,false);
	    sz =   v - 1;
	  }
	  //
	  if (sz >= 1) {
	    struct QMContextSet::MagnitudeSet &mset = contextset.ClassifyMagnitude(m_plDb[c][x],m_ucLarge[c]);
	    int  i = 0;
	    LONG m = 2;
	    //
	    m_Coder.Put((v > 0)?(zset.SP):(zset.SN),true);
	    //
	    while(sz >= m) {
	      m_Coder.Put(mset.X[i],true);
	      m <<= 1;
	      i++;
	    }
	    m_Coder.Put(mset.X[i],false);
	    //
	    m >>= 1;
	    while((m >>= 1)) {
	      m_Coder.Put(mset.M[i],(m & sz)?(true):(false));
	    }
	  } else {
	    m_Coder.Put((v > 0)?(zset.SP):(zset.SN),false);
	  }
	} else {
	  m_Coder.Put(zset.S0,false);
	}
	//
	// Update Da and Db.
	// Is this a bug? 32768 does not exist, but -32768 does. 
	// The reference streams use -32768, so let's stick to that.
	m_plDb[c][x]    = v;
	m_plDa[c][ym-1] = v;
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
	lp++,pp++;x++;
      } while(--xm);
      //
      // Go to the next line.
      x  = x  - m_ucMCUWidth[c];
      pp = lp - m_ucMCUWidth[c];
      if (line->m_pNext)
	line = line->m_pNext;
      lp = line->m_pData + x;
      y++;
    } while(--ym);
  }
}
///

/// ACLosslessScan::ParseMCU
// The actual MCU-parser, write a single group of pixels to the stream,
// or measure their statistics.
void ACLosslessScan::ParseMCU(struct Line **prev,struct Line **top,UBYTE preshift)
{ 
  UBYTE c;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(c = 0;c < m_ucCount;c++) {
    struct QMContextSet &contextset = m_Context[m_ucContext[c]];
    struct Line *line = top[c];
    struct Line *pline= prev[c];
    UBYTE ym = m_ucMCUHeight[c];
    ULONG  x = m_ulX[c];
    ULONG  y = m_ulY[c];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    if (m_bNoPrediction) {
      y        = 0;
      // Reset conditioning to the top
      memset(m_plDb[c] + x,0,sizeof(LONG) * m_ucMCUWidth[c]);
    }
    //
    // Parse MCUwidth * MCUheight coefficients starting at the line top.
    do {
      UBYTE xm     = m_ucMCUWidth[c];
      do {
	UBYTE mode = PredictionMode(x,y);
	LONG pred  = PredictSample(mode,preshift,lp,pp);
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v;
	// Get the sign coding context.
	struct QMContextSet::ContextZeroSet &zset = contextset.ClassifySignZero(m_plDa[c][ym-1],m_plDb[c][x],
										m_ucSmall[c],m_ucLarge[c]);
	//
	if (m_Coder.Get(zset.S0)) {
	  LONG sz   = 0;
	  bool sign = m_Coder.Get(zset.SS); // true for negative.
	  //
	  if (m_Coder.Get((sign)?(zset.SN):(zset.SP))) {
	    struct QMContextSet::MagnitudeSet &mset = contextset.ClassifyMagnitude(m_plDb[c][x],m_ucLarge[c]);
	    int  i = 0;
	    LONG m = 2;
	    //
	    while(m_Coder.Get(mset.X[i])) {
	      m <<= 1;
	      i++;
	    }
	    //
	    m >>= 1;
	    sz  = m;
	    while((m >>= 1)) {
	      if (m_Coder.Get(mset.M[i])) {
		sz |= m;
	      }
	    }
	  }
	  //
	  if (sign) {
	    v = -sz - 1;
	  } else {
	    v =  sz + 1;
	  }
	} else {
	  v = 0;
	}
	//
	// Set the current pixel, do the inverse pointwise transformation.
	// 16-bit transformation required.
	lp[0] = WORD(v + pred) << preshift; 
	//
	// Update Da and Db.
	// Is this a bug? 32768 does not exist, but -32768 does. The streams
	// seem to use -32768 instead.
	m_plDb[c][x]    = v;
	m_plDa[c][ym-1] = v;
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
	lp++,pp++;x++;
      } while(--xm);
      //
      // Go to the next line.
      x  = x  - m_ucMCUWidth[c];
      pp = lp - m_ucMCUWidth[c];
      if (line->m_pNext)
	line = line->m_pNext;
      lp = line->m_pData + x;
      y++;
    } while(--ym);
  }
}
///

/// ACLosslessScan::WriteMCU
// Write a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool ACLosslessScan::WriteMCU(void)
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
      BeginWriteMCU(m_Coder.ByteStreamOf());
      //
      WriteMCU(prev,top,preshift);
    } while(AdvanceToTheRight());
    //
    // Turn on prediction again after the restart line.
    m_bNoPrediction = false;
    // 
    // Reset conditioning to the left
    for(i = 0;i < m_ucCount;i++) {
      memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    }
    //
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);

  return false;
}
///

/// ACLosslessScan::ParseMCU
// Parse a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool ACLosslessScan::ParseMCU(void)
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
      if (BeginReadMCU(m_Coder.ByteStreamOf())) {
	ParseMCU(prev,top,preshift);
      } else {
	ClearMCU(top);
      }
    } while(AdvanceToTheRight());
    //
    // Turn on prediction again after the restart line.
    m_bNoPrediction = false;
    //
    // Reset conditioning to the left
    for(i = 0;i < m_ucCount;i++) {
      memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    }
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);
  
  return false; // no further blocks here.
}
///

/// ACLosslessScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ACLosslessScan::StartMCURow(void)
{
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
}
///

/// ACLosslessScan::Flush
void ACLosslessScan::Flush(bool)
{
  int i;
  
  m_Coder.Flush();

  for(i = 0;i < m_ucCount;i++) {
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    memset(m_plDb[i],0,sizeof(LONG) * m_ulWidth[i]);
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }

  m_bNoPrediction = true;
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_ulX[i]) {
      JPG_WARN(MALFORMED_STREAM,"ACLosslessScan::Flush",
	       "found restart marker in the middle of the line, expect corrupt results");
      break;
    }
  }


  m_Coder.OpenForWrite(m_Coder.ByteStreamOf());
}
///

/// ACLosslessScan::Restart
// Restart the parser at the next restart interval
void ACLosslessScan::Restart(void)
{ 
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    memset(m_plDb[i],0,sizeof(LONG) * m_ulWidth[i]);
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_ulX[i]) {
      JPG_WARN(MALFORMED_STREAM,"ACLosslessScan::Restart",
	       "found restart marker in the middle of the line, expect corrupt results");
      break;
    }
  }

  m_Coder.OpenForRead(m_Coder.ByteStreamOf());
  m_bNoPrediction = true;
}
///
