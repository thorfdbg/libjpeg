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
** $Id: aclosslessscan.cpp,v 1.20 2012-07-17 19:39:21 thor Exp $
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
///

/// ACLosslessScan::ACLosslessScan
ACLosslessScan::ACLosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit)
  : EntropyParser(frame,scan), m_pLineCtrl(NULL), m_ucPredictor(predictor), m_ucLowBit(lowbit)
{ 
  m_ucCount = scan->ComponentsInScan();
  
  for(int i = 0;i < m_ucCount;i++) {
    m_ucSmall[i]     = 0;
    m_ucLarge[i]     = 1;
  }
}
///

/// ACLosslessScan::FindComponentDimensions
// Collect the component information.
void ACLosslessScan::FindComponentDimensions(void)
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

/// ACLosslessScan::~ACLosslessScan
ACLosslessScan::~ACLosslessScan(void)
{
}
///

/// ACLosslessScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ACLosslessScan::WriteFrameType(class ByteStream *io)
{
  io->PutWord(0xffcb); // lossless sequential AC coded
}
///

/// ACLosslessScan::StartParseScan
void ACLosslessScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  class ACTemplate *dc;
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    dc = m_pScan->DCConditionerOf(i);
    if (dc) {
      m_ucSmall[i]    = dc->LowerThresholdOf();
      m_ucLarge[i]    = dc->UpperThresholdOf();
    } else {
      m_ucSmall[i]    = 0;
      m_ucLarge[i]    = 1;
    }
    m_lDa[i] = 0;
    m_lDb[i] = 0;
  }
  
  m_Context.Init();
  
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

  for(i = 0;i < m_ucCount;i++) {
    dc = m_pScan->DCConditionerOf(i);

    if (dc) {
      m_ucSmall[i]    = dc->LowerThresholdOf();
      m_ucLarge[i]    = dc->UpperThresholdOf();
    } else {
      m_ucSmall[i]    = 0;
      m_ucLarge[i]    = 1;
    }
    m_lDa[i] = 0;
    m_lDb[i] = 0;
  }

  m_Context.Init();
    
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

/// ACLosslessScan::Classify
// Find the context class depending on the previous DC and
// the values of L and U given in the conditioner.
int ACLosslessScan::Classify(LONG diff,UBYTE l,UBYTE u)
{
  LONG abs = (diff > 0)?(diff):(-diff);
  
  if (abs <= ((1 << l) >> 1)) {
    // the zero cathegory.
    return 0;
  }
  if (abs <= (1 << u)) {
    if (diff < 0) {
      return -1;
    } else {
      return 1;
    }
  }
  if (diff < 0) {
    return -2;
  } else {
    return 2;
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
    BeginWriteMCU(m_Coder.ByteStreamOf());
    do { 
      // Encode a single MCU, which is now a group of pixels.
      for(i = 0;i < m_ucCount;i++) {
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
		JPG_THROW(INVALID_PARAMETER,"ACLosslessScan::WriteMCU",
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
		JPG_THROW(INVALID_PARAMETER,"ACLosslessScan::WriteMCU",
			  "invalid predictor selected for the lossless mode");
		break;
	      }
	    //
	    // value to be encoded. Standard requires 16 bit modulo arithmetic.	    
	    v = ((lp[0] >> preshift) - pred); 

	    struct QMContextSet::ContextZeroSet &ctxt = m_Context.SignZeroCoding
	      [Classify(m_lDa[i],m_ucSmall[i],m_ucLarge[i]) + 2][Classify(m_lDb[i],m_ucSmall[i],m_ucLarge[i]) + 2];
	    //
	    if (v) {
	      LONG sz;
	      m_Coder.Put(ctxt.S0,true);
	      //
	      if (v < 0) {
		m_Coder.Put(ctxt.SS,true);
		sz = -(v + 1);
	      } else {
		m_Coder.Put(ctxt.SS,false);
		sz =   v - 1;
	      }
	      //
	      if (sz >= 1) {
		int  i = 0;
		LONG m = 2;
		int cl = Classify(m_lDb[i],m_ucSmall[i],m_ucLarge[i]);
		struct QMContextSet::MagnitudeSet &mag = (cl >= 2 || cl <= -2)?(m_Context.MagnitudeHigh):
		  (m_Context.MagnitudeLow);
		m_Coder.Put((v > 0)?(ctxt.SP):(ctxt.SN),true);
		//
		while(sz >= m) {
		  m_Coder.Put(mag.X[i],true);
		  m <<= 1;
		  i++;
		}
		m_Coder.Put(mag.X[i],false);
		//
		m >>= 1;
		while((m >>= 1)) {
		  m_Coder.Put(mag.M[i],(m & sz)?(true):(false));
		}
	      } else {
		m_Coder.Put((v > 0)?(ctxt.SP):(ctxt.SN),false);
	      }
	    } else {
	      m_Coder.Put(ctxt.S0,false);
	    }
	    //
	    // Update Da and Db. The logic is a bit strange here....
	    if (y == 0) {
	      m_lDb[i] = 0;
	    } else {
	      m_lDb[i] = (pp[0] >> preshift) - (lp[0] >> preshift);
	    }
	    if (x == 0) {
	      m_lDa[i] = 0;
	    } else {
	      m_lDa[i] = (lp[-1] >> preshift) - (lp[0] >> preshift);
	    }
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

/// ACLosslessScan::ParseMCU
// Parse a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool ACLosslessScan::ParseMCU(void)
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
    if (BeginReadMCU(m_Coder.ByteStreamOf()) == false) {
      ClearMCU(top);
    } else do {
      // Encode a single MCU, which is now a group of pixels.
      for(i = 0;i < m_ucCount;i++) {
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
	    //
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
		JPG_THROW(INVALID_PARAMETER,"ACLosslessScan::ParseMCU",
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
		JPG_THROW(INVALID_PARAMETER,"ACLosslessScan::ParseMCU",
			  "invalid predictor selected for the lossless mode");
		break;
	      }
	    //
	    // Decode now the difference between the predicted value and
	    // the real value.
	    {
	      WORD v; // 16 bit integer arithmetic required.
	      // NOTE: C++ does actually not ensure that v has wraparound-characteristics, 
	      // though it typically does.
	      //
	      struct QMContextSet::ContextZeroSet &ctxt = m_Context.SignZeroCoding
		[Classify(m_lDa[i],m_ucSmall[i],m_ucLarge[i]) + 2][Classify(m_lDb[i],m_ucSmall[i],m_ucLarge[i]) + 2];
	      //
	      if (m_Coder.Get(ctxt.S0)) {
		LONG sz   = 0;
		bool sign = m_Coder.Get(ctxt.SS); // true for negative.
		//
		if (m_Coder.Get((sign)?(ctxt.SN):(ctxt.SP))) {
		  int  i = 0;
		  LONG m = 2;
		  int cl = Classify(m_lDb[i],m_ucSmall[i],m_ucLarge[i]);
		  struct QMContextSet::MagnitudeSet &mag = (cl >= 2 || cl <= -2)?(m_Context.MagnitudeHigh):
		    (m_Context.MagnitudeLow);
		  //
		  while(m_Coder.Get(mag.X[i])) {
		    m <<= 1;
		    i++;
		  }
		  //
		  m >>= 1;
		  sz  = m;
		  while((m >>= 1)) {
		    if (m_Coder.Get(mag.M[i])) {
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
	      lp[0] = (v + pred) << preshift;
	    }
	    //
	    // Update Da and Db. The logic is a bit strange here....
	    if (y == 0) {
	      m_lDb[i] = 0;
	    } else {
	      m_lDb[i] = (pp[0] >> preshift) - (lp[0] >> preshift);
	    }
	    if (x == 0) {
	      m_lDa[i] = 0;
	    } else {
	      m_lDa[i] = (lp[-1] >> preshift) - (lp[0] >> preshift);
	    }
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

/// ACLosslessScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ACLosslessScan::StartMCURow(void)
{
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
}
///

/// ACLosslessScan::Flush
void ACLosslessScan::Flush(void)
{
  m_Coder.Flush();
  m_Context.Init();

  for(int i = 0;i < m_ucCount;i++) {
    m_lDa[i] = 0;
    m_lDb[i] = 0;
  }
  m_bNoPrediction = true;

  m_Coder.OpenForWrite(m_Coder.ByteStreamOf());
}
///

/// ACLosslessScan::Restart
// Restart the parser at the next restart interval
void ACLosslessScan::Restart(void)
{ 
  for(int i = 0;i < m_ucCount;i++) {
    m_lDa[i] = 0;
    m_lDb[i] = 0;
  }
  
  m_Context.Init();
  m_Coder.OpenForRead(m_Coder.ByteStreamOf());
  m_bNoPrediction = true;
}
///

/// ACLosslessScan::ClearMCU
// Clear the entire MCU
void ACLosslessScan::ClearMCU(class Line **top)
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
