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
** A JPEG LS scan. This is the base for all JPEG LS scan types, namely
** separate, line interleaved and sample interleaved.
**
** $Id: jpeglsscan.cpp,v 1.27 2021/12/01 11:14:12 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bitstream.hpp"
#include "codestream/entropyparser.hpp"
#include "codestream/jpeglsscan.hpp"
#include "codestream/tables.hpp"
#include "control/bufferctrl.hpp"
#include "control/linebuffer.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "marker/thresholds.hpp"
#include "tools/line.hpp"
///

/// JPEGLSScan::m_lJ Runlength array
// The runlength J array. 
#if ACCUSOFT_CODE
const LONG JPEGLSScan::m_lJ[32] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,5,5,6,6,7,7,8,9,10,11,12,13,14,15};
#endif
///
/// JPEGLSScan::JPEGLSScan
// Create a new scan. This is only the base type.
JPEGLSScan::JPEGLSScan(class Frame *frame,class Scan *scan,UBYTE near,const UBYTE *mapping,UBYTE point)
  : EntropyParser(frame,scan)
#if ACCUSOFT_CODE
  , m_pLineCtrl(NULL), m_pDefaultThresholds(NULL), 
    m_lNear(near), m_ucLowBit(point)
#endif
{
#if ACCUSOFT_CODE
  memcpy(m_ucMapIdx,mapping,sizeof(m_ucMapIdx)); 

  //
  // Initialize the golomb decoder lookup.
  
  m_ucLeadingZeros[0] = 8;

  for(UBYTE i = 255;i > 0;i--) {
    UBYTE idx  = i;
    UBYTE zcnt = 0;
    while((idx & 0x80) == 0) {
      idx <<= 1;
      zcnt++;
    }
    m_ucLeadingZeros[i] = zcnt;
  }
#else
  NOREF(near);
  NOREF(mapping);
  NOREF(point);
#endif
}
///

/// JPEGLSScan::~JPEGLSScan
JPEGLSScan::~JPEGLSScan(void)
{ 
#if ACCUSOFT_CODE
  int i;

  for(i = 0;i < 4;i++) {
    if (m_Top[i].m_pData)      m_pEnviron->FreeMem(m_Top[i].m_pData     ,(2 + m_ulWidth[i]) * sizeof(LONG));
    if (m_AboveTop[i].m_pData) m_pEnviron->FreeMem(m_AboveTop[i].m_pData,(2 + m_ulWidth[i]) * sizeof(LONG));
  }

  delete m_pDefaultThresholds;
#endif
}
///

/// JPEGLSScan::FindComponentDimensions
// Collect the component information.
void JPEGLSScan::FindComponentDimensions(void)
{
#if ACCUSOFT_CODE
  class Thresholds *thres;
  unsigned int i;

  m_ulPixelWidth  = m_pFrame->WidthOf();
  m_ulPixelHeight = m_pFrame->HeightOf();

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE subx = comp->SubXOf();
    UBYTE suby = comp->SubYOf();

    m_ulWidth[i]     = (m_ulPixelWidth  + subx - 1) / subx;
    m_ulHeight[i]    = (m_ulPixelHeight + suby - 1) / suby;
    m_ulRemaining[i] = m_ulHeight[i];
  }
  
  thres = m_pScan->FindThresholds();
  if (thres == NULL) {
    if (m_pDefaultThresholds == NULL)
      m_pDefaultThresholds = new(m_pEnviron) class Thresholds(m_pEnviron);
    m_pDefaultThresholds->InstallDefaults(m_pFrame->PrecisionOf(),m_lNear);
    thres = m_pDefaultThresholds;
  }

  m_lMaxVal = thres->MaxValOf();
  m_lT1     = thres->T1Of();
  m_lT2     = thres->T2Of();
  m_lT3     = thres->T3Of();
  m_lReset  = thres->ResetOf();
  //
  // The bucket size.
  m_lDelta  = 2 * m_lNear + 1;

  if (m_lNear == 0) { // Lossless
    m_lRange = m_lMaxVal + 1;
  } else {
    m_lRange = (m_lMaxVal + 2 * m_lNear) / m_lDelta + 1;
  }

  // Compute qbpp
  for (m_lQbpp = 1; (1 << m_lQbpp) < m_lRange; m_lQbpp++) {
  }

  // Compute bpp
  for (m_lBpp  = 1; (1 << m_lBpp) < (m_lMaxVal + 1);m_lBpp++) {
  }
  if (m_lBpp < 2) m_lBpp = 2;

  m_lLimit  = ((m_lBpp + ((m_lBpp < 8)?(8):(m_lBpp))) << 1) - m_lQbpp - 1;
  m_lMaxErr = (m_lRange + 1) >> 1;
  m_lMinErr = m_lMaxErr - m_lRange;

  //
  // Compute minimum and maximum reconstruction values.
  m_lMinReconstruct = -m_lNear;
  m_lMaxReconstruct =  m_lMaxVal + m_lNear;
 
  // Allocate the line buffers if not yet there.
  for(i = 0;i < m_ucCount;i++) {
    if (m_Top[i].m_pData == NULL)
      m_Top[i].m_pData      = (LONG *)m_pEnviron->AllocMem((2 + m_ulWidth[i]) * sizeof(LONG));

    if (m_AboveTop[i].m_pData == NULL)
      m_AboveTop[i].m_pData = (LONG *)m_pEnviron->AllocMem((2 + m_ulWidth[i]) * sizeof(LONG));
  
    if (m_ucMapIdx[i]) {
      // FIXME: Find the mapping table.
      JPG_THROW(NOT_IMPLEMENTED,"JPEGLSSScan::FindComponentDimensions",
                "mapping tables are not implemented by this code, sorry");
    }
  }

  //
  // Init the state variables N,A,B,C
  InitMCU();

#endif
}
///

/// JPEGLSScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void JPEGLSScan::WriteFrameType(class ByteStream *io)
{
  io->PutWord(0xfff7); // JPEG LS SOF55
}
///

/// JPEGLSScan::StartParseScan 
// Fill in the tables for decoding and decoding parameters in general.
void JPEGLSScan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
  FindComponentDimensions();

  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  m_Stream.OpenForRead(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"JPEGLSScan::StartParseScan",
            "JPEG LS not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// JPEGLSScan::StartWriteScan
// Begin writing the scan data
void JPEGLSScan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
  FindComponentDimensions();

  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);

  EntropyParser::StartWriteScan(io,chk,ctrl);
  
  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"JPEGLSScan::StartWriteScan",
            "JPEG LS not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// JPEGLSScan::StartMeasureScan
// Start measuring the statistics. Since JPEG LS is not Huffman based,
// this need not to be implemented.
void JPEGLSScan::StartMeasureScan(class BufferCtrl *)
{
  JPG_THROW(NOT_IMPLEMENTED,"LosslessScan::StartMeasureScan",
            "JPEG LS is not based on Huffman coding and does not require a measurement phase");
}
///

/// JPEGLSScan::StartOptimizeScan
// Start making an optimization run to adjust the coefficients.
void JPEGLSScan::StartOptimizeScan(class BufferCtrl *)
{  
  JPG_THROW(NOT_IMPLEMENTED,"LosslessScan::StartOptimizeScan",
            "JPEG LS is not based on Huffman coding and does not support R/D optimization");
}
///

/// JPEGLSScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool JPEGLSScan::StartMCURow(void)
{
#if ACCUSOFT_CODE
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
#else
  return false;
#endif
}
///

/// JPEGLSScan::Flush
// Flush the remaining bits out to the stream on writing.
void JPEGLSScan::Flush(bool)
{
#if ACCUSOFT_CODE
  m_Stream.Flush();
  InitMCU();
#endif
}
///

/// JPEGLSScan::Restart
// Restart the parser at the next restart interval
void JPEGLSScan::Restart(void)
{
#if ACCUSOFT_CODE
  m_Stream.OpenForRead(m_Stream.ByteStreamOf(),m_Stream.ChecksumOf());
  InitMCU();
#endif
}
///

/// JPEGLSScan::InitMCU
// Initialize MCU for the next restart interval.
void JPEGLSScan::InitMCU(void)
{
#if ACCUSOFT_CODE
  LONG a0;
  unsigned int i;

  // Init the state variables N,A,B,C
  //
  for (i = 0; i < sizeof(m_lN) / sizeof(LONG); i++)
    m_lN[i] = 1;
  for (i = 0; i < sizeof(m_lB) / sizeof(LONG); i++)
    m_lB[i] = m_lC[i] = 0;
  
  a0 = (m_lRange + (1 << 5)) >> 6;
  if (a0 < 2) a0 = 2;
  
  for (i = 0; i < sizeof(m_lA) / sizeof(LONG); i++)
    m_lA[i] = a0;
  
  //
  // Runlength data.
  memset(m_lRunIndex, 0, sizeof(m_lRunIndex));
  
  //
  // Initialize the line buffers.
  for (i = 0; i < m_ucCount; i++) {
    memset(m_Top[i].m_pData,      0, (2 + m_ulWidth[i]) * sizeof(LONG));
    memset(m_AboveTop[i].m_pData, 0, (2 + m_ulWidth[i]) * sizeof(LONG));
  }
#endif
}
///

/// JPEGLSScan::BeginReadMCU
// Scanning for a restart marker is here a bit more tricky due to the
// presence of bitstuffing - the stuffed zero-bit need to be removed
// (and thus the byte containing it) before scanning for the restart
// marker.
bool JPEGLSScan::BeginReadMCU(class ByteStream *io)
{
#if ACCUSOFT_CODE
  //
  // Skip a potentially stuffed zero-bit to reach
  // and read the marker correctly.
  m_Stream.SkipStuffing();
#endif
  return EntropyParser::BeginReadMCU(io);
}
///

/// JPEGLSScan::OptimizeBlock
// Make an R/D optimization for the given scan by potentially pushing
// coefficients into other bins. 
void JPEGLSScan::OptimizeBlock(LONG, LONG, UBYTE ,double ,
                               class DCT *,LONG [64])
{
  JPG_THROW(NOT_IMPLEMENTED,"JPEGLSScan::OptimizeBlock",
            "Rate-distortion optimization is not available for line-based coding modes");
}
///

/// JPEGLSScan::OptimizeDC
// Make an R/D optimization of the DC scan. This includes all DC blocks in
// total, not just a single block. This is because the coefficients are not
// coded independently.
void JPEGLSScan::OptimizeDC(void)
{ 
  JPG_THROW(NOT_IMPLEMENTED,"JPEGLSScan::OptimizeDC",
            "Rate-distortion optimization is not available for line-based coding modes");
}
///
