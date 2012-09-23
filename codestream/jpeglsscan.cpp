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
** A JPEG LS scan. This is the base for all JPEG LS scan types, namely
** separate, line interleaved and sample interleaved.
**
** $Id: jpeglsscan.cpp,v 1.12 2012-09-22 20:51:40 thor Exp $
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
const LONG JPEGLSScan::m_lJ[32] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,5,5,6,6,7,7,8,9,10,11,12,13,14,15};
///
/// JPEGLSScan::JPEGLSScan
// Create a new scan. This is only the base type.
JPEGLSScan::JPEGLSScan(class Frame *frame,class Scan *scan,UBYTE near,const UBYTE *mapping,UBYTE point)
  : EntropyParser(frame,scan), m_pLineCtrl(NULL), m_pDefaultThresholds(NULL), 
    m_lNear(near), m_ucLowBit(point)
{
  memcpy(m_ucMapIdx,mapping,sizeof(m_ucMapIdx));
}
///

/// JPEGLSScan::~JPEGLSScan
JPEGLSScan::~JPEGLSScan(void)
{ 
  int i;

  for(i = 0;i < 4;i++) {
    if (m_Top[i].m_pData)      m_pEnviron->FreeMem(m_Top[i].m_pData     ,(2 + m_ulWidth[i]) * sizeof(LONG));
    if (m_AboveTop[i].m_pData) m_pEnviron->FreeMem(m_AboveTop[i].m_pData,(2 + m_ulWidth[i]) * sizeof(LONG));
  }

  delete m_pDefaultThresholds;
}
///

/// JPEGLSScan::FindComponentDimensions
// Collect the component information.
void JPEGLSScan::FindComponentDimensions(void)
{
  class Thresholds *thres;
  LONG a0;
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
 
  //
  // Init the state variables N,A,B,C
  for(i = 0;i < sizeof(m_lN) / sizeof(LONG);i++)
    m_lN[i] = 1;
  for(i = 0;i < sizeof(m_lB) / sizeof(LONG);i++)
    m_lB[i] = m_lC[i] = 0;

  a0 = (m_lRange + (1 << 5)) >> 6;
  if (a0 < 2) a0 = 2;

  for(i = 0;i < sizeof(m_lA) / sizeof(LONG);i++)
    m_lA[i] = a0;

  //
  // Runlength data.
  memset(m_lRunIndex,0,sizeof(m_lRunIndex));

  // Allocate the line buffers if not yet there.
  for(i = 0;i < m_ucCount;i++) {
    if (m_Top[i].m_pData == NULL)
      m_Top[i].m_pData      = (LONG *)m_pEnviron->AllocMem((2 + m_ulWidth[i]) * sizeof(LONG));
    memset(m_Top[i].m_pData,0,(2 + m_ulWidth[i]) * sizeof(LONG));
    if (m_AboveTop[i].m_pData == NULL)
      m_AboveTop[i].m_pData = (LONG *)m_pEnviron->AllocMem((2 + m_ulWidth[i]) * sizeof(LONG));
    memset(m_AboveTop[i].m_pData,0,(2 + m_ulWidth[i]) * sizeof(LONG));
  
    if (m_ucMapIdx[i]) {
      // FIXME: Find the mapping table.
      JPG_THROW(NOT_IMPLEMENTED,"JPEGLSSScan::FindComponentDimensions",
		"mapping tables are not implemented by this code, sorry");
    }
  }
}
///

/// JPEGLSScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void JPEGLSScan::WriteFrameType(class ByteStream *io)
{
  io->PutWord(0xfff7); // JPEG LS
}
///

/// JPEGLSScan::StartParseScan 
// Fill in the tables for decoding and decoding parameters in general.
void JPEGLSScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{

  FindComponentDimensions();

  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  m_Stream.OpenForRead(io);
}
///

/// JPEGLSScan::StartWriteScan
// Begin writing the scan data
void JPEGLSScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{

  FindComponentDimensions();

  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);

  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io);
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

/// JPEGLSScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool JPEGLSScan::StartMCURow(void)
{
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
}
///

/// JPEGLSScan::Flush
// Flush the remaining bits out to the stream on writing.
void JPEGLSScan::Flush(bool)
{
  m_Stream.Flush();
}
///

/// JPEGLSScan::Restart
// Restart the parser at the next restart interval
void JPEGLSScan::Restart(void)
{
  m_Stream.OpenForRead(m_Stream.ByteStreamOf());
}
///




/// JPEGLSScan::BeginReadMCU
// Scanning for a restart marker is here a bit more tricky due to the
// presence of bitstuffing - the stuffed zero-bit need to be removed
// (and thus the byte containing it) before scanning for the restart
// marker.
bool JPEGLSScan::BeginReadMCU(class ByteStream *io)
{
  //
  // Skip a potentially stuffed zero-bit to reach
  // and read the marker correctly.
  m_Stream.SkipStuffing();

  return EntropyParser::BeginReadMCU(io);
}
///
