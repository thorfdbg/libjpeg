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
** Represents the scan including the scan header.
**
** $Id: acsequentialscan.cpp,v 1.52 2022/05/23 05:56:51 thor Exp $
**
*/

/// Includes
#include "codestream/acsequentialscan.hpp"
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
#include "control/blockbitmaprequester.hpp"
#include "control/blockbuffer.hpp"
#include "control/blocklineadapter.hpp"
#include "coding/actemplate.hpp"
#include "marker/actable.hpp"
///

/// ACSequentialScan::ACSequentialScan
ACSequentialScan::ACSequentialScan(class Frame *frame,class Scan *scan,
                                   UBYTE start,UBYTE stop,UBYTE lowbit,UBYTE,
                                   bool differential,bool residual,bool large)
  : EntropyParser(frame,scan)
#if ACCUSOFT_CODE
  , m_pBlockCtrl(NULL),
    m_ucScanStart(start), m_ucScanStop(stop), m_ucLowBit(lowbit),
    m_bMeasure(false), m_bDifferential(differential), m_bResidual(residual), m_bLargeRange(large)
#endif
{
#if ACCUSOFT_CODE
  m_ucCount = scan->ComponentsInScan();
  
  for(UBYTE i = 0;i < m_ucCount;i++) {
    m_ucSmall[i]     = 0;
    m_ucLarge[i]     = 1;
    m_ucBlockEnd[i]  = 5;
  }
#else
  NOREF(start);
  NOREF(stop);
  NOREF(lowbit);
  NOREF(differential);
  NOREF(residual);
  NOREF(large);
#endif
}
///

/// ACSequentialScan::~ACSequentialScan
ACSequentialScan::~ACSequentialScan(void)
{
}
///

/// ACSequentialScan::StartParseScan
void ACSequentialScan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{ 
#if ACCUSOFT_CODE
  class ACTemplate *ac,*dc;
  int i;

  for(i = 0;i < m_ucCount;i++) {
    dc = m_pScan->DCConditionerOf(i);
    ac = m_pScan->ACConditionerOf(i); 
    
    m_ucDCContext[i]  = m_pScan->DCTableIndexOf(i);
    m_ucACContext[i]  = m_pScan->ACTableIndexOf(i);

    if (dc) {
      m_ucSmall[i]    = dc->LowerThresholdOf();
      m_ucLarge[i]    = dc->UpperThresholdOf();
    } else {
      m_ucSmall[i]    = 0;
      m_ucLarge[i]    = 1;
    }

    if (ac) {
      m_ucBlockEnd[i] = ac->BandDiscriminatorOf();
    } else {
      m_ucBlockEnd[i] = 5;
    }

    m_lDC[i]         = 0; 
    m_lDiff[i]       = 0;
    m_ulX[i]         = 0;
  }
  
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
  m_Coder.OpenForRead(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"ACSequentialScan::StartParseScan",
            "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// ACSequentialScan::StartWriteScan
void ACSequentialScan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{ 
#if ACCUSOFT_CODE
  class ACTemplate *ac,*dc;
  int i;

  for(i = 0;i < m_ucCount;i++) {
    dc = m_pScan->DCConditionerOf(i);
    ac = m_pScan->ACConditionerOf(i);
   
    m_ucDCContext[i]  = m_pScan->DCTableIndexOf(i);
    m_ucACContext[i]  = m_pScan->ACTableIndexOf(i);

    if (dc) {
      m_ucSmall[i]    = dc->LowerThresholdOf();
      m_ucLarge[i]    = dc->UpperThresholdOf();
    } else {
      m_ucSmall[i]    = 0;
      m_ucLarge[i]    = 1;
    }

    if (ac) {
      m_ucBlockEnd[i] = ac->BandDiscriminatorOf();
    } else {
      m_ucBlockEnd[i] = 5;
    }

    m_lDC[i]           = 0;
    m_lDiff[i]         = 0;
    m_ulX[i]           = 0;
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);

  EntropyParser::StartWriteScan(io,chk,ctrl);

  m_pScan->WriteMarker(io);
  m_Coder.OpenForWrite(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"ACSequentialScan::StartWriteScan",
            "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// ACSequentialScan::StartMeasureScan
// Measure scan statistics.
void ACSequentialScan::StartMeasureScan(class BufferCtrl *)
{ 
  //
  // This is not required.
  JPG_THROW(NOT_IMPLEMENTED,"ACSequentialScan::StartMeasureScan",
            "arithmetic coding is always adaptive and does not require "
            "to measure the statistics");
}
///

/// ACSequentialScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ACSequentialScan::StartMCURow(void)
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

/// ACSequentialScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool ACSequentialScan::WriteMCU(void)
{ 
#if ACCUSOFT_CODE
  bool more = true;
  int c;

  assert(m_pBlockCtrl);
  
  BeginWriteMCU(m_Coder.ByteStreamOf());

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pComponent[c];
    class QuantizedRow *q    = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    LONG &prevdc             = m_lDC[c];
    LONG &prevdiff           = m_lDiff[c];
    UBYTE l                  = m_ucSmall[c];
    UBYTE u                  = m_ucLarge[c];
    UBYTE kx                 = m_ucBlockEnd[c];
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
          block[0] = prevdc;
        }
        EncodeBlock(block,prevdc,prevdiff,l,u,kx,m_ucDCContext[c],m_ucACContext[c]);
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

/// ACSequentialScan::Restart
// Restart the parser at the next restart interval
void ACSequentialScan::Restart(void)
{
#if ACCUSOFT_CODE
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    m_lDC[i]         = 0; 
    m_lDiff[i]       = 0;
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  m_Coder.OpenForRead(m_Coder.ByteStreamOf(),m_Coder.ChecksumOf());
#endif
}
///

/// ACSequentialScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool ACSequentialScan::ParseMCU(void)
{
#if ACCUSOFT_CODE
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  bool valid = BeginReadMCU(m_Coder.ByteStreamOf());
  
  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pComponent[c];
    class QuantizedRow *q    = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    LONG &prevdc             = m_lDC[c];
    LONG &prevdiff           = m_lDiff[c];
    UBYTE l                  = m_ucSmall[c];
    UBYTE u                  = m_ucLarge[c];
    UBYTE kx                 = m_ucBlockEnd[c];
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
          DecodeBlock(block,prevdc,prevdiff,l,u,kx,m_ucDCContext[c],m_ucACContext[c]);
        } else {
          for(UBYTE i = m_ucScanStart;i <= m_ucScanStop;i++) {
            block[i] = 0;
          }
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

/// ACSequentialScan::Classify
// Find the DC context class depending on the previous DC and
// the values of L and U given in the conditioner.
#if ACCUSOFT_CODE
struct ACSequentialScan::QMContextSet::DCContextZeroSet &ACSequentialScan::QMContextSet::Classify(LONG diff,UBYTE l,UBYTE u)
{
  LONG abs = (diff > 0)?(diff):(-diff);
  
  if (abs <= ((1 << l) >> 1)) {
    // the zero cathegory.
    return DCZero;
  }
  if (abs <= (1 << u)) {
    if (diff < 0) {
      return DCSmallNegative;
    } else {
      return DCSmallPositive;
    }
  }
  if (diff < 0) {
    return DCLargeNegative;
  } else {
    return DCLargePositive;
  }
}
#endif
///

/// ACSequentialScan::EncodeBlock
// Encode a single block
#if ACCUSOFT_CODE
void ACSequentialScan::EncodeBlock(const LONG *block,
                                   LONG &prevdc,LONG &prevdiff,
                                   UBYTE small,UBYTE large,UBYTE kx,UBYTE dc,UBYTE ac)
{
  // DC coding
  if (m_ucScanStart == 0 && m_bResidual == false) {
    struct QMContextSet::DCContextZeroSet &cz = m_Context[dc].Classify(prevdiff,small,large);
    LONG diff;
    // DPCM coding of the DC coefficient.
    diff   = block[0] >> m_ucLowBit; // only correct for two's completement machines
    diff  -= prevdc;
    if (m_bDifferential) {
      prevdc = 0;
    } else {
      prevdc = block[0] >> m_ucLowBit;
    }

    if (diff) {
      LONG sz;
      //
      // Nonzero, encode a one in context zero.
      m_Coder.Put(cz.S0,true);
      //
      // Sign coding. Encode a zero for positive and a 1 for
      // negative.
      if (diff < 0) {
        m_Coder.Put(cz.SS,true);
        sz = -diff - 1;
      } else {
        m_Coder.Put(cz.SS,false);
        sz = diff - 1;
      }
      //
      // Code the magnitude.
      if (sz >= 1) {
        int  i = 0;
        LONG m = 2;
        m_Coder.Put((diff > 0)?(cz.SP):(cz.SN),true);
        //
        // Magnitude category coding.
        while(sz >= m) {
          m_Coder.Put(m_Context[dc].DCMagnitude.X[i],true);
          m <<= 1;
          i++;
        } 
        // Terminate magnitude cathegory coding.
        m_Coder.Put(m_Context[dc].DCMagnitude.X[i],false);
        //
        // Get the MSB to code.
        m >>= 1;
        // Refinement bits: Depend on the magnitude category.
        while((m >>= 1)) {
          m_Coder.Put(m_Context[dc].DCMagnitude.M[i],(m & sz)?(true):(false));
        }
      } else {
        m_Coder.Put((diff > 0)?(cz.SP):(cz.SN),false);
      }
    } else {
      // Difference is zero. Encode a zero in context zero.
      m_Coder.Put(cz.S0,false);
    }
    // Keep the difference for the next block.
    prevdiff = diff;
  }

  if (m_ucScanStop) {
    LONG data;
    int eob,k;
    // AC coding. Part one. Find the end of block.
    // eob is the index of the first zero coefficient from
    // which point on this, and all following coefficients
    // up to coefficient with index 63 are zero.
    eob = m_ucScanStop;
    k   = (m_ucScanStart)?(m_ucScanStart):((m_bResidual)?0:1);
    //
    while(eob >= k) {
      data = block[DCT::ScanOrder[eob]];
      if ((data >= 0)?(data >> m_ucLowBit):((-data) >> m_ucLowBit))
        break;
      eob--;
    }
    // The coefficient at eob is now nonzero, but eob+1 is
    // a zero coefficient or beyond the block end.
    eob++; // the first coefficient *not* to code.

    do {
      LONG data,sz;
      //
      if (k == eob) {
        m_Coder.Put(m_Context[ac].ACZero[k-1].SE,true); // Code EOB.
        break;
      }
      // Not EOB.
      m_Coder.Put(m_Context[ac].ACZero[k-1].SE,false);
      //
      // Run coding in S0. Since k is not the eob, at least
      // one non-zero coefficient must follow, so we cannot
      // run over the end of the block.
      do {
        data = block[DCT::ScanOrder[k]];
        data = (data >= 0)?(data >> m_ucLowBit):(-((-data) >> m_ucLowBit));
        if (data == 0) {
          m_Coder.Put(m_Context[ac].ACZero[k-1].S0,false);
          k++;
        }
      } while(data == 0);
      m_Coder.Put(m_Context[ac].ACZero[k-1].S0,true);
      //
      // The coefficient at k is now nonzero. First code
      // the sign. This context is the uniform.
      if (data < 0) {
        m_Coder.Put(m_Context[ac].Uniform,true);
        sz = -data - 1;
      } else {
        m_Coder.Put(m_Context[ac].Uniform,false);
        sz =  data - 1;
      }
      //
      // Code the magnitude category. 
      if (sz >= 1) {
        m_Coder.Put(m_Context[ac].ACZero[k-1].SP,true); // SP or SN coding.
        if (sz >= 2) {
          int  i = 0;
          LONG m = 4;
          struct QMContextSet::ACContextMagnitudeSet &acm = (k > kx)?(m_Context[ac].ACMagnitudeHigh):(m_Context[ac].ACMagnitudeLow);
          //
          m_Coder.Put(m_Context[ac].ACZero[k-1].SP,true); // X1 coding, identical to SN and SP.
          // Note that AC_SN,AC_SP and AC_X1 are all the same context
          // all following decisions are not conditioned on k directly.
          while(sz >= m) {
            m_Coder.Put(acm.X[i],true);
            m <<= 1;
            i++;
          }
          m_Coder.Put(acm.X[i],false);
          //
          // Get the MSB to code.
          m >>= 1;
          //
          // Magnitude refinement coding.
          while((m >>= 1)) {
            m_Coder.Put(acm.M[i],(m & sz)?true:false);
          }
        } else {
          m_Coder.Put(m_Context[ac].ACZero[k-1].SP,false);
        }
      } else {
        m_Coder.Put(m_Context[ac].ACZero[k-1].SP,false);
      }
      //
      // Encode the next coefficient. Note that this bails out early without an
      // S0 encoding if the end is reached.
    } while(++k <= m_ucScanStop);
  }
}
#endif
///

/// ACSequentialScan::DecodeBlock
// Decode a single block.
#if ACCUSOFT_CODE
void ACSequentialScan::DecodeBlock(LONG *block,
                                   LONG &prevdc,LONG &prevdiff,
                                   UBYTE small,UBYTE large,UBYTE kx,UBYTE dc,UBYTE ac)
{
  // DC coding
  if (m_ucScanStart == 0 && m_bResidual == false) {
    LONG diff;
    struct QMContextSet::DCContextZeroSet &cz = m_Context[dc].Classify(prevdiff,small,large);
    // Check whether the difference is nonzero.
    if (m_Coder.Get(cz.S0)) {
      LONG sz;
      bool sign = m_Coder.Get(cz.SS); // sign coding, is true for negative.
      //
      //
      // Positive and negative are encoded in different contexts.
      // Decode the magnitude cathegory.
      if (m_Coder.Get((sign)?(cz.SN):(cz.SP))) {
        int  i = 0;
        LONG m = 2;
        
        while(m_Coder.Get(m_Context[dc].DCMagnitude.X[i])) {
          m <<= 1;
          if(++i >= QMContextSet::DCContextMagnitudeSet::MagnitudeContexts)
            JPG_THROW(MALFORMED_STREAM,"ACSequentialScan::DecodeBlock",
                      "QMDecoder is out of sync");
        }
        //
        // Get the MSB to decode.
        m >>= 1;
        sz  = m;
        //
        // Refinement coding of remaining bits.
        while((m >>= 1)) {
          if (m_Coder.Get(m_Context[dc].DCMagnitude.M[i])) {
            sz |= m;
          }
        }
      } else {
        sz = 0;
      }
      //
      // Done, finally, include the sign and the offset.
      if (sign) {
        diff = -sz - 1;
      } else {
        diff = sz + 1;
      }
    } else {
      // Difference is zero.
      diff = 0;
    }

    prevdiff = diff;
    if (m_bDifferential) {
      prevdc   = diff;
    } else {
      prevdc  += diff;
    }
    block[0] = prevdc << m_ucLowBit; // point transformation
  }

  if (m_ucScanStop) {
    // AC coding. No block skipping used here.
    int k = (m_ucScanStart)?(m_ucScanStart):((m_bResidual)?0:1);
    //
    // EOB decoding.
    while(k <= m_ucScanStop && !m_Coder.Get(m_Context[ac].ACZero[k-1].SE)) {
      LONG sz;
      bool sign;
      //
      // Not yet EOB. Run coding in S0: Skip over zeros.
      while(!m_Coder.Get(m_Context[ac].ACZero[k-1].S0)) {
        k++;
        if (k > m_ucScanStop)
          JPG_THROW(MALFORMED_STREAM,"ACSequentialScan::DecodeBlock",
                    "QMDecoder is out of sync");
      }
      //
      // Now decode the sign of the coefficient.
      // This happens in the uniform context.
      sign = m_Coder.Get(m_Context[ac].Uniform);
      //
      // Decode the magnitude.
      if (m_Coder.Get(m_Context[ac].ACZero[k-1].SP)) {
        // X1 coding, identical to SN and SP.
        if (m_Coder.Get(m_Context[ac].ACZero[k-1].SP)) {
          int  i = 0;
          LONG m = 4;
          struct QMContextSet::ACContextMagnitudeSet &acm = (k > kx)?(m_Context[ac].ACMagnitudeHigh):(m_Context[ac].ACMagnitudeLow);
          
          while(m_Coder.Get(acm.X[i])) {
            m <<= 1;
            if(++i >= QMContextSet::ACContextMagnitudeSet::MagnitudeContexts)
              JPG_THROW(MALFORMED_STREAM,"ACSequentialScan::DecodeBlock",
                        "QMDecoder is out of sync");
          }
          //
          // Get the MSB to decode
          m >>= 1;
          sz  = m;
          //
          // Proceed to refinement.
          while((m >>= 1)) {
            if (m_Coder.Get(acm.M[i])) {
              sz |= m;
            }
          }
        } else {
          sz = 1;
        }
      } else {
        sz = 0;
      }
      //
      // Done. Finally, include sign and offset.
      sz++;
      if (sign) 
        sz = -sz;
      block[DCT::ScanOrder[k]] = sz << m_ucLowBit;
      //
      // Proceed to the next block.
      k++;
    }
  }
}
#endif
///

/// ACSequentialScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ACSequentialScan::WriteFrameType(class ByteStream *io)
{
#if ACCUSOFT_CODE
  UBYTE hidden = m_pFrame->TablesOf()->HiddenDCTBitsOf();

  if (m_ucScanStart > 0 || m_ucScanStop < 63 || m_ucLowBit > hidden) {
    // is progressive.
    if (m_bResidual) {
      io->PutWord(0xffba); // progressive sequential
    } else {
      if (m_bDifferential) {
        io->PutWord(0xffce);
      } else {
        io->PutWord(0xffca);
      }
    }
  } else {
    if (m_bResidual) {
      io->PutWord(0xffb9); // residual AC sequential
    } else if (m_bDifferential) {
      io->PutWord(0xffcd); // AC differential sequential
    } else if (m_bLargeRange) {
      io->PutWord(0xffbb);
    } else {
      io->PutWord(0xffc9); // AC sequential
    }
  }
#else
  NOREF(io);
#endif
}
///

/// ACSequentialScan::Flush
// Flush the remaining bits out to the stream on writing.
void ACSequentialScan::Flush(bool)
{
#if ACCUSOFT_CODE
  int i;
  
  m_Coder.Flush();

  for(i = 0;i < m_ucCount;i++) {
    m_lDC[i]    = 0;
    m_lDiff[i]  = 0;
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  m_Coder.OpenForWrite(m_Coder.ByteStreamOf(),m_Coder.ChecksumOf());
#endif
}
///

/// ACSequentialScan::OptimizeBlock
// Make an R/D optimization for the given scan by potentially pushing
// coefficients into other bins. 
void ACSequentialScan::OptimizeBlock(LONG, LONG, UBYTE ,double ,
                                     class DCT *,LONG [64])
{
  JPG_THROW(NOT_IMPLEMENTED,"ACSequentialScan::OptimizeBlock",
            "Rate-distortion optimization is not implemented for arithmetic coding");
}
///

/// ACSequentialScan::OptimizeDC
// Make an R/D optimization for the given scan by potentially pushing
// coefficients into other bins. 
void ACSequentialScan::OptimizeDC(void)
{
  JPG_THROW(NOT_IMPLEMENTED,"ACSequentialScan::OptimizeDC",
            "Rate-distortion optimization is not implemented for arithmetic coding");
}
///

/// ACSequentialScan::StartOptimizeScan
// Start making an optimization run to adjust the coefficients.
void ACSequentialScan::StartOptimizeScan(class BufferCtrl *)
{  
  JPG_THROW(NOT_IMPLEMENTED,"ACSequentialScan::StartOptimizeScan",
            "Rate-distortion optimization is not implemented for arithmetic coding");
}
///
