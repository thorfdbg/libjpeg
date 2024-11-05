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
** $Id: aclosslessscan.cpp,v 1.44 2024/11/05 06:39:25 thor Exp $
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
#include "codestream/predictorbase.hpp"
#include "tools/line.hpp"
#include "std/string.hpp"
///

/// ACLosslessScan::ACLosslessScan
ACLosslessScan::ACLosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,bool differential)
  : PredictiveScan(frame,scan,predictor,lowbit,differential)
{ 
#if ACCUSOFT_CODE
  m_ucCount = scan->ComponentsInScan();
  
  for(int i = 0;i < m_ucCount;i++) {
    m_ucSmall[i]     = 0;
    m_ucLarge[i]     = 1;
  }

  memset(m_plDa,0,sizeof(m_plDa));
  memset(m_plDb,0,sizeof(m_plDb));
#endif
}
///

/// ACLosslessScan::~ACLosslessScan
ACLosslessScan::~ACLosslessScan(void)
{
#if ACCUSOFT_CODE
  UBYTE i;
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_plDa[i])
      m_pEnviron->FreeMem(m_plDa[i],sizeof(LONG) * m_ucMCUHeight[i]);
    if (m_plDb[i])
      m_pEnviron->FreeMem(m_plDb[i],sizeof(LONG) * m_ucMCUWidth[i] * m_ulWidth[i]);
  }
#endif
}
///

/// ACLosslessScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ACLosslessScan::WriteFrameType(class ByteStream *io)
{
#if ACCUSOFT_CODE
  if (m_bDifferential) {
    io->PutWord(0xffcf); // differential lossless sequential AC coded
  } else {
    io->PutWord(0xffcb); // lossless sequential AC coded
  }
#else
  NOREF(io);
#endif
}
///

/// ACLosslessScan::FindComponentDimensions
// Common setup for encoding and decoding.
#if ACCUSOFT_CODE
void ACLosslessScan::FindComponentDimensions(void)
{
  UBYTE i;

  PredictiveScan::FindComponentDimensions();

  for(i = 0;i < m_ucCount;i++) {
    assert(m_plDa[i] == NULL && m_plDb[i] == NULL);

    m_plDa[i] = (LONG *)(m_pEnviron->AllocMem(sizeof(LONG) * m_ucMCUHeight[i]));
    m_plDb[i] = (LONG *)(m_pEnviron->AllocMem(sizeof(LONG) * m_ucMCUWidth[i] * m_ulWidth[i]));
  }
}
#endif
///

/// ACLosslessScan::StartParseScan
void ACLosslessScan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
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
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]); 
    memset(m_plDb[i],0,sizeof(LONG) * m_ucMCUWidth[i] * m_ulWidth[i]);
    m_ucContext[i]    = m_pScan->DCTableIndexOf(i); 
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  m_Coder.OpenForRead(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"ACLosslessScan::StartParseScan",
            "JPEG lossless not available your code release, please contact Accusoft for a full version");
#endif
}
///

/// ACLosslessScan::StartWriteScan
void ACLosslessScan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
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
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    memset(m_plDb[i],0,sizeof(LONG) * m_ucMCUWidth[i] * m_ulWidth[i]);
    m_ucContext[i]    = m_pScan->DCTableIndexOf(i); 
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
    
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan); 

  EntropyParser::StartWriteScan(io,chk,ctrl);
  
  m_pScan->WriteMarker(io);
  m_Coder.OpenForWrite(io,chk);
#else 
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"ACLosslessScan::StartWriteScan",
            "JPEG lossless not available your code release, please contact Accusoft for a full version");
#endif
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
void ACLosslessScan::WriteMCU(struct Line **prev,struct Line **top)
{ 
#if ACCUSOFT_CODE
  UBYTE c;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(c = 0;c < m_ucCount;c++) {
    struct QMContextSet &contextset = m_Context[m_ucContext[c]];
    struct Line *line = top[c];
    struct Line *pline= prev[c];
    UBYTE ym = m_ucMCUHeight[c];
    class PredictorBase *mcupred = m_pPredict[c];
    ULONG  x = m_ulX[c];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    // Write MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[c];
      do {
        // Decode now the difference between the predicted value and
        // the real value.
        LONG v = pred->EncodeSample(lp,pp);
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
      } while(--xm && (lp++,pp++,x++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + (x = m_ulX[c]),line = (line->m_pNext)?(line->m_pNext):(line),
                     lp = line->m_pData + x,mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}
///

/// ACLosslessScan::ParseMCU
// The actual MCU-parser, write a single group of pixels to the stream,
// or measure their statistics.
void ACLosslessScan::ParseMCU(struct Line **prev,struct Line **top)
{ 
#if ACCUSOFT_CODE
  UBYTE c;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(c = 0;c < m_ucCount;c++) {
    struct QMContextSet &contextset = m_Context[m_ucContext[c]];
    struct Line *line = top[c];
    struct Line *pline= prev[c];
    UBYTE ym = m_ucMCUHeight[c];
    ULONG  x = m_ulX[c];
    class PredictorBase *mcupred = m_pPredict[c];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    // Parse MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[c];
      do {
        // Decode now the difference between the predicted value and
        // the real value.
        LONG v;
        //
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
              if (++i >= QMContextSet::MagnitudeSet::MagnitudeContexts)
                JPG_THROW(MALFORMED_STREAM,"ACLosslessScan::ParseMCU",
                          "received an out-of-bounds signal while parsing an AC-coded lossless symbol");
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
        // Use the prediction to fill in the sample.
        lp[0] = pred->DecodeSample(v,lp,pp);
        // Update Da and Db.
        // Is this a bug? 32768 does not exist, but -32768 does. The streams
        // seem to use -32768 instead.
        m_plDb[c][x]    = v;
        m_plDa[c][ym-1] = v;
        //
        // One pixel done. Proceed to the next in the MCU. Note that
        // the lines have been extended such that always a complete MCU is present.
      } while(--xm && (lp++,pp++,x++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + (x = m_ulX[c]),line = (line->m_pNext)?(line->m_pNext):(line),
                     lp = line->m_pData + x,mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}
///

/// ACLosslessScan::WriteMCU
// Write a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool ACLosslessScan::WriteMCU(void)
{
#if ACCUSOFT_CODE
  int i;
  struct Line *top[4],*prev[4];
  int lines      = 8; // total number of MCU lines processed.
  
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
      WriteMCU(prev,top);
    } while(AdvanceToTheRight());
    //
    // Reset conditioning to the left
    for(i = 0;i < m_ucCount;i++) {
      memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    }
    //
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);
#endif
  return false;
}
///

/// ACLosslessScan::ParseMCU
// Parse a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool ACLosslessScan::ParseMCU(void)
{
#if ACCUSOFT_CODE
  int i;
  struct Line *top[4],*prev[4];
  int lines      = 8; // total number of MCU lines processed.

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
        ParseMCU(prev,top);
      } else {
        // Only if this is not due to a DNL marker that has been detected.
        if (m_ulPixelHeight != 0 && !hasFoundDNL()) {
          ClearMCU(top);
        } else {
          // The problem is here that the DNL marker might have been detected, even though decoding
          // is not yet done completely. This may be because there are still just enough bits in the
          // AC coding engine present to run a single decode. Big Outch! Just continue decoding in
          // this case.
          ParseMCU(prev,top);
        }
      }
    } while(AdvanceToTheRight());
    //
    // Reset conditioning to the left
    for(i = 0;i < m_ucCount;i++) {
      memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    }
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);
#endif
  return false; // no further blocks here.
}
///

/// ACLosslessScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ACLosslessScan::StartMCURow(void)
{
#if ACCUSOFT_CODE
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
#else
  return false;
#endif
}
///

/// ACLosslessScan::Flush
void ACLosslessScan::Flush(bool)
{
#if ACCUSOFT_CODE
  int i;
  
  m_Coder.Flush();

  for(i = 0;i < m_ucCount;i++) {
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    memset(m_plDb[i],0,sizeof(LONG) * m_ucMCUWidth[i] * m_ulWidth[i]); // Reset conditioning to the top
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  PredictiveScan::FlushOnMarker();
  
  m_Coder.OpenForWrite(m_Coder.ByteStreamOf(),m_Coder.ChecksumOf());
#endif
}
///

/// ACLosslessScan::Restart
// Restart the parser at the next restart interval
void ACLosslessScan::Restart(void)
{ 
#if ACCUSOFT_CODE
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    memset(m_plDa[i],0,sizeof(LONG) * m_ucMCUHeight[i]);
    memset(m_plDb[i],0,sizeof(LONG) * m_ucMCUWidth[i] * m_ulWidth[i]);
  }
  for(i = 0;i < 4;i++) {
    m_Context[i].Init();
  }
  
  PredictiveScan::RestartOnMarker();

  m_Coder.OpenForRead(m_Coder.ByteStreamOf(),m_Coder.ChecksumOf());
#endif
}
///
