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
** A subsequent (refinement) scan of a progressive scan.
**
** $Id: hiddenrefinementscan.cpp,v 1.4 2012-09-23 14:10:12 thor Exp $
**
*/

/// Includes
#include "codestream/hiddenrefinementscan.hpp"
#include "codestream/tables.hpp"
#include "marker/residualmarker.hpp"
#include "marker/frame.hpp"
#include "control/blockbuffer.hpp"
///

/// HiddenScan::HiddenScan
template<class BaseScan>
HiddenScan<BaseScan>::HiddenScan(class Frame *frame,class Scan *scan,
				 class ResidualMarker *marker,
				 UBYTE start,UBYTE stop,UBYTE lowbit,UBYTE highbit,
				 bool residuals,bool differential)
  : BaseScan(frame,scan,start,stop,lowbit,highbit,differential,residuals),
    m_pResidualBuffer(NULL), m_pTarget(NULL), m_pMarker(marker), m_bResiduals(residuals)
{
}
///

/// HiddenScan::~HiddenScan
template<class BaseScan>
HiddenScan<BaseScan>::~HiddenScan(void)
{
  delete m_pResidualBuffer;
}
///

/// HiddenScan::StartParseScan
template<class BaseScan>
void HiddenScan<BaseScan>::StartParseScan(class ByteStream *,class BufferCtrl *ctrl)
{  
  assert(m_pMarker);

  BaseScan::StartParseScan(m_pMarker->StreamOf(),ctrl);
}
///

/// HiddenScan::StartWriteScan
template<class BaseScan>
void HiddenScan<BaseScan>::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  // Prepare the residual buffer for collecting the output stream data.
  assert(m_pResidualBuffer == NULL);
  m_pResidualBuffer = new(BaseScan::m_pEnviron) class MemoryStream(BaseScan::m_pEnviron,4096);
  
  //
  // Where the data actually goes to.
  m_pTarget         = io;
  BaseScan::StartWriteScan(m_pResidualBuffer,ctrl);
}
///

/// HiddenScan::StartMeasureScan
// Measure scan statistics.
template<class BaseScan>
void HiddenScan<BaseScan>::StartMeasureScan(class BufferCtrl *ctrl)
{ 
  BaseScan::StartMeasureScan(ctrl);
}
///

/// HiddenScan::Restart
// Restart the parser at the next restart interval
template<class BaseScan>
void HiddenScan<BaseScan>::Restart(void)
{
  BaseScan::Restart();
} 
///

/// HiddenScan::Flush
// Flush the remaining bits out to the stream on writing.
template<class BaseScan>
void HiddenScan<BaseScan>::Flush(bool final)
{
  BaseScan::Flush(final);

  if (final && BaseScan::m_bMeasure == false) {
    class ResidualMarker *marker = m_pMarker;
    //
    assert(marker);
    //
    // Write out the buffered data as a series of markers.
    marker->WriteMarker(m_pTarget,m_pResidualBuffer);
    //
    delete m_pResidualBuffer;m_pResidualBuffer = NULL;
  }
} 
///

/// HiddenScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
template<class BaseScan>
void HiddenScan<BaseScan>::WriteFrameType(class ByteStream *io)
{
  // This scan doesn't have a frame type as it extends all following scans,
  // so let the following scan do the work.
  assert(BaseScan::m_pScan && BaseScan::m_pScan->NextOf());

  BaseScan::m_pScan->NextOf()->WriteFrameType(io);
}
///

/// HiddenScan::GetRow
// Return the data to process for component c. Will be overridden
// for residual scan types.
template<class BaseScan>
class QuantizedRow *HiddenScan<BaseScan>::GetRow(UBYTE idx) const
{
  if (m_bResiduals) {
    return BaseScan::m_pBlockCtrl->CurrentResidualRow(idx);
  } else {
    return BaseScan::m_pBlockCtrl->CurrentQuantizedRow(idx);
  }
}
///

/// HiddenScan::StartRow
// Check whether there are more rows to process, return true
// if so, false if not. Start with the row if there are.
template<class BaseScan>
bool HiddenScan<BaseScan>::StartRow(void) const
{
  if (m_bResiduals) {
    return BaseScan::m_pBlockCtrl->StartMCUResidualRow(BaseScan::m_pScan);
  } else {
    return BaseScan::m_pBlockCtrl->StartMCUQuantizerRow(BaseScan::m_pScan);
  }
}
///

/// Explicit instanciations
template class HiddenScan<RefinementScan>;
template class HiddenScan<ACRefinementScan>;
template class HiddenScan<SequentialScan>;
template class HiddenScan<ACSequentialScan>;
///

