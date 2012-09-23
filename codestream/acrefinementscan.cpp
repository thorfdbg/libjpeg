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
** Encode a refinement scan with the arithmetic coding procedure from
** Annex G.
**
** $Id: acrefinementscan.cpp,v 1.13 2012-09-23 14:10:12 thor Exp $
**
*/

/// Includes
#include "codestream/acrefinementscan.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/rectanglerequest.hpp"
#include "dct/idct.hpp"
#include "dct/sermsdct.hpp"
#include "std/assert.hpp"
#include "interface/bitmaphook.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
#include "tools/traits.hpp"
#include "control/blockbuffer.hpp"
#include "control/blockbitmaprequester.hpp"
#include "control/blocklineadapter.hpp"
#include "coding/actemplate.hpp"
#include "marker/actable.hpp"
///

/// ACRefinementScan::ACRefinementScan
ACRefinementScan::ACRefinementScan(class Frame *frame,class Scan *scan,
				   UBYTE start,UBYTE stop,UBYTE lowbit,UBYTE highbit,
				   bool,bool)
  : EntropyParser(frame,scan), m_pBlockCtrl(NULL),
    m_ucScanStart(start), m_ucScanStop(stop), m_ucLowBit(lowbit), m_ucHighBit(highbit)
{
  m_ucCount = scan->ComponentsInScan();
}
///

/// ACRefinementScan::~ACRefinementScan
ACRefinementScan::~ACRefinementScan(void)
{
}
///

/// ACRefinementScan::StartParseScan
void ACRefinementScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) {
    m_ulX[i]         = 0;
  }
  m_Context.Init();
  
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
  m_Coder.OpenForRead(io);
}
///

/// ACRefinementScan::StartWriteScan
void ACRefinementScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) {
    m_ulX[i]           = 0;
  }
  m_Context.Init();

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan); 
  //
  // Actually, always:
  m_bMeasure = false;


  m_pScan->WriteMarker(io);
  m_Coder.OpenForWrite(io);
}
///

/// ACRefinementScan::StartMeasureScan
// Measure scan statistics.
void ACRefinementScan::StartMeasureScan(class BufferCtrl *)
{ 
  //
  // This is not required.
  JPG_THROW(NOT_IMPLEMENTED,"ACRefinementScan::StartMeasureScan",
	    "arithmetic coding is always adaptive and does not require "
	    "to measure the statistics");
}
///

/// ACRefinementScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ACRefinementScan::StartMCURow(void)
{
  bool more = m_pBlockCtrl->StartMCUQuantizerRow(m_pScan);

  for(int i = 0;i < m_ucCount;i++) {
    m_ulX[i]   = 0;
  }

  return more;
}
///

/// ACRefinementScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool ACRefinementScan::WriteMCU(void)
{ 
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  BeginWriteMCU(m_Coder.ByteStreamOf());

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pComponent[c];
    class QuantizedRow *q    = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    UBYTE mcux               = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    UBYTE mcuy               = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG xmin               = m_ulX[c];
    ULONG xmax               = xmin + mcux;
    ULONG x,y; 
    if (xmax >= q->WidthOf()) {
      more     = false;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *block,dummy[64];
	if (q && x < q->WidthOf()) {
	  block  = q->BlockAt(x)->m_Data;
	} else {
	  block  = dummy;
	  memset(dummy ,0,sizeof(dummy) );
	}
	EncodeBlock(block);
      }
      if (q) q = q->NextOf();
    }
    // Done with this component, advance the block.
    m_ulX[c] = xmax;
  }

  return more;
}
///

/// ACRefinementScan::Restart
// Restart the parser at the next restart interval
void ACRefinementScan::Restart(void)
{
  m_Context.Init();
  m_Coder.OpenForRead(m_Coder.ByteStreamOf());
}
///

/// ACRefinementScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool ACRefinementScan::ParseMCU(void)
{
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  bool valid = BeginReadMCU(m_Coder.ByteStreamOf());
  
  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pComponent[c];
    class QuantizedRow *q    = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    UBYTE mcux               = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    UBYTE mcuy               = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG xmin               = m_ulX[c];
    ULONG xmax               = xmin + mcux;
    ULONG x,y;
    if (xmax >= q->WidthOf()) {
      more     = false;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *block,dummy[64];
	if (q && x < q->WidthOf()) {
	  block  = q->BlockAt(x)->m_Data;
	} else {
	  block  = dummy;
	}
	if (valid) {
	  DecodeBlock(block);
	} 
      }
      if (q) q = q->NextOf();
    }
    // Done with this component, advance the block.
    m_ulX[c] = xmax;
  }

  return more;
}
///

/// ACRefinementScan::EncodeBlock
// Encode a single huffman block
void ACRefinementScan::EncodeBlock(const LONG *block)
{
  //
  // DC coding. This only encodes the LSB of the current bit in
  // the uniform context.
  if (m_ucScanStart == 0) {
    m_Coder.Put(m_Context.Uniform,(block[0] >> m_ucLowBit) & 0x01);
  }

  if (m_ucScanStop) {
    LONG data;
    int eob,eobx,k;
    // AC coding. Part one. Find the end of block in this
    // scan, and in the previous scan. As AC coding has
    // to go into a separate scan, StartScan must be at least one.
    assert(m_ucScanStart);
    // eob is the index of the first zero coefficient from
    // which point on this, and all following coefficients
    // up to coefficient with index 63 are zero.
    eob = m_ucScanStop;
    k   = m_ucScanStart;
    //
    while(eob >= k) {
      data = block[DCT::ScanOrder[eob]];
      if ((data >= 0)?(data >> m_ucLowBit):((-data) >> m_ucLowBit))
	break;
      eob--;
    }
    // The coefficient at eob is now nonzero, but eob+1 is
    // a zero coefficient or beyond the block end.
    eobx = eob;
    eob++; // the first coefficient *not* to code.
    // eobx is the EOB position of the previous scan, i.e. it must
    // necessarily be lower than eob.
    while(eobx >= k) {
      data = block[DCT::ScanOrder[eobx]];
      if ((data >= 0)?(data >> m_ucHighBit):((-data) >> m_ucHighBit))
	break;
      eobx--;
    }
    eobx++;

    do {
      LONG data;
      //
      // The EOB decision is only coded if not known from the last
      // pass.
      if (k >= eobx) {
	if (k == eob) {
	  m_Coder.Put(m_Context.ACZero[k-1].SE,true); // Code EOB.
	  break;
	}
	// Not EOB.
	m_Coder.Put(m_Context.ACZero[k-1].SE,false);
      }
      //
      // Run coding in S0. Since k is not the eob, at least
      // one non-zero coefficient must follow, so we cannot
      // run over the end of the block.
      do {
	data = block[DCT::ScanOrder[k]];
	data = (data >= 0)?(data >> m_ucLowBit):(-((-data) >> m_ucLowBit));
	if (data == 0) {
	  m_Coder.Put(m_Context.ACZero[k-1].S0,false);
	  k++;
	}
      } while(data == 0);
      //
      // The coefficient at k is now nonzero. 
      if (data > 1 || data < -1) {
	// The coefficient was nonzero before, i.e. this is
	// refinement coding. S0 coding is skipped since
	// the decoder can detect this condition as well.
	m_Coder.Put(m_Context.ACZero[k-1].SC,data & 0x01);
      } else {
	// Zero before, becomes significant.
	m_Coder.Put(m_Context.ACZero[k-1].S0,true);
	// Can now be only positive or negative. 
	// The sign is encoded in the uniform context.
	m_Coder.Put(m_Context.Uniform,(data < 0)?true:false);
      }
      // Encode the next coefficient. Note that this bails out early without an
      // S0 encoding if the end is reached.
    } while(++k <= m_ucScanStop);
  }
}
///

/// ACRefinementScan::DecodeBlock
// Decode a single huffman block.
void ACRefinementScan::DecodeBlock(LONG *block)
{
  // DC coding
  if (m_ucScanStart == 0) {
    // This is only coded in the uniform context, no further modelling
    // done.
    if (m_Coder.Get(m_Context.Uniform))
      block[0] |= 1 << m_ucLowBit;
  }

  // AC coding. No block skipping used here.
  if (m_ucScanStop) {
    int eobx,k;
    assert(m_ucScanStart);
    //
    // Determine the eobx, i.e. the eob from the previous scan.
    eobx = m_ucScanStop;
    k    = m_ucScanStart;
    //
    while(eobx >= k) {
      LONG data = block[DCT::ScanOrder[eobx]];
      if ((data >= 0)?(data >> m_ucHighBit):((-data) >> m_ucHighBit))
	break;
      eobx--;
    }
    eobx++;
    //
    // EOB decoding. Only necessary for k >= eobx
    while(k < eobx || (k <= m_ucScanStop && !m_Coder.Get(m_Context.ACZero[k-1].SE))) {
      LONG data;
      //
      // Not yet EOB. Run coding in S0: Skip over zeros. Note that
      // only the necessary decisions are coded. If the data is not
      // zero, refinement coding is required and this decision
      // can be detected here without requiring to fetch a bit.
      while((data = block[DCT::ScanOrder[k]]) == 0 && 
	    !m_Coder.Get(m_Context.ACZero[k-1].S0)) {
	k++;
	if (k > m_ucScanStop)
	  JPG_THROW(MALFORMED_STREAM,"ACRefinementScan::DecodeBlock",
		    "QMDecoder is out of sync");
      }
      //
      // The loop could be aborted for two reasons: Data became nonzero,
      // requiring refinement, or the AC coder returned a one-bit, indicating
      // that we hit an insignificant coefficient that became significant.
      if (data) {
	// Refinement coding.
	if (m_Coder.Get(m_Context.ACZero[k-1].SC)) {
	  // Requires refinement.
	  if (data > 0) {
	    block[DCT::ScanOrder[k]] += 1L << m_ucLowBit;
	  } else {
	    block[DCT::ScanOrder[k]] -= 1L << m_ucLowBit;
	  }
	}
      } else {
	// Became significant. Sign coding happens in the uniform
	// context.
	if (m_Coder.Get(m_Context.Uniform)) {
	  block[DCT::ScanOrder[k]] = -1L << m_ucLowBit;
	} else {
	  block[DCT::ScanOrder[k]] = +1L << m_ucLowBit;
	}
      }
      //
      // Proceed to the next block.
      k++;
    }
  }
}
///

/// ACRefinementScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ACRefinementScan::WriteFrameType(class ByteStream *io)
{
  // is progressive.
  io->PutWord(0xffca);
}
///

/// ACRefinementScan::Flush
// Flush the remaining bits out to the stream on writing.
void ACRefinementScan::Flush(bool)
{
  m_Coder.Flush();
  m_Context.Init();
  m_Coder.OpenForWrite(m_Coder.ByteStreamOf());
}
///
