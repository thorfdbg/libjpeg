/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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
** Annex G of ITU Recommendation T.81 (1992) | ISO/IEC 10918-1:1994.
**
** $Id: acrefinementscan.cpp,v 1.34 2024/03/25 18:42:33 thor Exp $
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
#include "dct/dct.hpp"
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
                                   bool,bool residual)
  : EntropyParser(frame,scan)
#if ACCUSOFT_CODE
  , m_pBlockCtrl(NULL),
    m_ucScanStart(start), m_ucScanStop(stop), m_ucLowBit(lowbit), m_ucHighBit(highbit),
    m_bResidual(residual)
#endif
{
#if ACCUSOFT_CODE
  m_ucCount = scan->ComponentsInScan();
  assert(m_ucHighBit == m_ucLowBit + 1);
#else
  NOREF(start);
  NOREF(stop);
  NOREF(lowbit);
  NOREF(highbit);
  NOREF(residual);
#endif
}
///

/// ACRefinementScan::~ACRefinementScan
ACRefinementScan::~ACRefinementScan(void)
{
}
///

/// ACRefinementScan::StartParseScan
void ACRefinementScan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{ 
#if ACCUSOFT_CODE
  int i;

  for(i = 0;i < m_ucCount;i++) {
    m_ulX[i]         = 0;
  }
  m_Context.Init();
  
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
  m_Coder.OpenForRead(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"ACRefinementScan::StartParseScan",
            "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// ACRefinementScan::StartWriteScan
void ACRefinementScan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{ 
#if ACCUSOFT_CODE
  int i;

  for(i = 0;i < m_ucCount;i++) {
    m_ulX[i]           = 0;
  }
  m_Context.Init();

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan); 
  //
  // Actually, always:
  m_bMeasure = false;

  EntropyParser::StartWriteScan(io,chk,ctrl);

  m_pScan->WriteMarker(io);
  m_Coder.OpenForWrite(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"ACRefinementScan::StartWriteScan",
            "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
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
#if ACCUSOFT_CODE
  bool more = m_pBlockCtrl->StartMCUQuantizerRow(m_pScan);

  for(int i = 0;i < m_ucCount;i++) {
    m_ulX[i]   = 0;
  }

  return more;
#else
  return false;
#endif
}
///

/// ACRefinementScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool ACRefinementScan::WriteMCU(void)
{ 
#if ACCUSOFT_CODE
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
#else
  return false;
#endif
}
///

/// ACRefinementScan::Restart
// Restart the parser at the next restart interval
void ACRefinementScan::Restart(void)
{
#if ACCUSOFT_CODE
  m_Context.Init();
  m_Coder.OpenForRead(m_Coder.ByteStreamOf(),m_Coder.ChecksumOf());
#endif
}
///

/// ACRefinementScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool ACRefinementScan::ParseMCU(void)
{
#if ACCUSOFT_CODE
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
#else
  return false;
#endif
}
///

/// ACRefinementScan::EncodeBlock
// Encode a single huffman block
#if ACCUSOFT_CODE
void ACRefinementScan::EncodeBlock(const LONG *block)
{
  //
  // DC coding. This only encodes the LSB of the current bit in
  // the uniform context.
  if (m_ucScanStart == 0 && m_bResidual == false) {
    m_Coder.Put(m_Context.Uniform,(block[0] >> m_ucLowBit) & 0x01);
  }

  if (m_ucScanStop || m_bResidual) {
    LONG data;
    int eob,eobx,k;
    // AC coding. Part one. Find the end of block in this
    // scan, and in the previous scan. As AC coding has
    // to go into a separate scan, StartScan must be at least one.
    assert(m_ucScanStart || m_bResidual);
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
          m_Coder.Put(m_Context.ACZero[k].SE,true); // Code EOB.
          break;
        }
        // Not EOB.
        m_Coder.Put(m_Context.ACZero[k].SE,false);
      }
      //
      // Run coding in S0. Since k is not the eob, at least
      // one non-zero coefficient must follow, so we cannot
      // run over the end of the block.
      do {
        data = block[DCT::ScanOrder[k]];
        data = (data >= 0)?(data >> m_ucLowBit):(-((-data) >> m_ucLowBit));
        if (data == 0) {
          m_Coder.Put(m_Context.ACZero[k].S0,false);
          k++;
        }
      } while(data == 0);
      //
      // The coefficient at k is now nonzero. 
      if (data > 1 || data < -1) {
        // The coefficient was nonzero before, i.e. this is
        // refinement coding. S0 coding is skipped since
        // the decoder can detect this condition as well.
        m_Coder.Put(m_Context.ACZero[k].SC,data & 0x01);
      } else {
        // Zero before, becomes significant.
        m_Coder.Put(m_Context.ACZero[k].S0,true);
        // Can now be only positive or negative. 
        // The sign is encoded in the uniform context.
        m_Coder.Put(m_Context.Uniform,(data < 0)?true:false);
      }
      // Encode the next coefficient. Note that this bails out early without an
      // S0 encoding if the end is reached.
    } while(++k <= m_ucScanStop);
  }
}
#endif
///

/// ACRefinementScan::DecodeBlock
// Decode a single huffman block.
#if ACCUSOFT_CODE
void ACRefinementScan::DecodeBlock(LONG *block)
{
  // DC coding
  if (m_ucScanStart == 0 && m_bResidual == false) {
    // This is only coded in the uniform context, no further modelling
    // done.
    if (m_Coder.Get(m_Context.Uniform))
      block[0] |= 1 << m_ucLowBit;
  }

  // AC coding. No block skipping used here.
  if (m_ucScanStop || m_bResidual) {
    int eobx,k;
    assert(m_ucScanStart || m_bResidual);
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
    while(k < eobx || (k <= m_ucScanStop && !m_Coder.Get(m_Context.ACZero[k].SE))) {
      LONG data;
      //
      // Not yet EOB. Run coding in S0: Skip over zeros. Note that
      // only the necessary decisions are coded. If the data is not
      // zero, refinement coding is required and this decision
      // can be detected here without requiring to fetch a bit.
      while((data = block[DCT::ScanOrder[k]]) == 0 && 
            !m_Coder.Get(m_Context.ACZero[k].S0)) {
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
        if (m_Coder.Get(m_Context.ACZero[k].SC)) {
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
#endif
///

/// ACRefinementScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ACRefinementScan::WriteFrameType(class ByteStream *io)
{
#if ACCUSOFT_CODE
  // is progressive.
  if (m_bResidual) {
    io->PutWord(0xffba); // AC residual refinement
  } else {
    io->PutWord(0xffca);
  }
#else
  NOREF(io);
#endif
}
///

/// ACRefinementScan::Flush
// Flush the remaining bits out to the stream on writing.
void ACRefinementScan::Flush(bool)
{
#if ACCUSOFT_CODE
  m_Coder.Flush();
  m_Context.Init();
  m_Coder.OpenForWrite(m_Coder.ByteStreamOf(),m_Coder.ChecksumOf());
#endif
}
///

/// ACRefinementScan::OptimizeBlock
// Make an R/D optimization for the given scan by potentially pushing
// coefficients into other bins. 
void ACRefinementScan::OptimizeBlock(LONG, LONG, UBYTE ,double ,
                                     class DCT *,LONG [64])
{
  JPG_THROW(NOT_IMPLEMENTED,"ACRefinementScan::OptimizeBlock",
            "Rate-distortion optimization is not implemented for arithmetic coding");
}
///

/// ACRefinementScan::OptimizeDC
// Make an R/D optimization for the given scan by potentially pushing
// coefficients into other bins. 
void ACRefinementScan::OptimizeDC(void)
{
  JPG_THROW(NOT_IMPLEMENTED,"ACRefinementScan::OptimizeDC",
            "Rate-distortion optimization is not implemented for arithmetic coding");
}
///

/// ACRefinementScan::StartOptimizeScan
// Start making an optimization run to adjust the coefficients.
void ACRefinementScan::StartOptimizeScan(class BufferCtrl *)
{  
  JPG_THROW(NOT_IMPLEMENTED,"ACRefinementScan::StartOptimizeScan",
            "Rate-distortion optimization is not implemented for arithmetic coding");
}
///
