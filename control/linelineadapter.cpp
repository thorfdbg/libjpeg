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
** This class adapts to a line buffer in a way that allows the user
** to pull out (or push in) individual lines, thus not too much is to
** do here. Again, this adapts to the upsampling process of the
** hierarchical mode.
**
** $Id: linelineadapter.cpp,v 1.4 2012-07-20 22:55:54 thor Exp $
**
*/

/// Includes
#include "control/linelineadapter.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "tools/line.hpp"
#include "std/string.hpp"
///

/// LineLineAdapter::LineLineAdapter
LineLineAdapter::LineLineAdapter(class Frame *frame)
  : LineBuffer(frame), LineAdapter(frame), m_pEnviron(frame->EnvironOf()), m_pFrame(frame),
    m_pppImage(NULL), m_pulReadyLines(NULL), m_pulLinesPerComponent(NULL)
{
  m_ucCount = m_pFrame->DepthOf();
}
///

/// LineLineAdapter::~LineLineAdapter
LineLineAdapter::~LineLineAdapter(void)
{
  // The buffered image is in the line buffer itself,
  // so do not bother here to transmit.

  if (m_pulReadyLines)
    m_pEnviron->FreeMem(m_pulReadyLines,m_ucCount * sizeof(ULONG));

  if (m_pppImage)
    m_pEnviron->FreeMem(m_pppImage,m_ucCount * sizeof(struct Line **));

  if (m_pulLinesPerComponent)
    m_pEnviron->FreeMem(m_pulLinesPerComponent,m_ucCount * sizeof(ULONG));
}
///

/// LineLineAdapter::BuildCommon
void LineLineAdapter::BuildCommon(void)
{
  UBYTE i;
  
  LineBuffer::BuildCommon();
  LineAdapter::BuildCommon();

  if (m_pulReadyLines == NULL) {
    m_pulReadyLines = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
    memset(m_pulReadyLines,0,sizeof(ULONG) * m_ucCount);
  }

  if (m_pppImage == NULL) {
    m_pppImage = (struct Line ***)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
    memset(m_pppImage,0,m_ucCount * sizeof(struct Line **));
  }

  if (m_pulLinesPerComponent == NULL) {
    m_pulLinesPerComponent = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
  }

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp      = m_pFrame->ComponentOf(i);
    UBYTE suby                 = comp->SubYOf();
    m_pulLinesPerComponent[i]  = (m_ulPixelHeight + suby - 1) / suby;
    m_pppImage[i]              = m_ppTop  + i;
  }
}
///

/// LineLineAdapter::GetNextLine
// Get the next available line from the output
// buffer on reconstruction. The caller must make
// sure that the buffer is really loaded up to the
// point or the line will be neutral grey.
struct Line *LineLineAdapter::GetNextLine(UBYTE comp)
{
  struct Line *line;
  assert(comp < m_ucCount);
  //
  // Get the next line from the image, possibly extend the image
  if (*m_pppImage[comp] == NULL) { 
    line = AllocateLine(comp);
    //
    // Initialize to neutral grey as no data is there.
    memset(line->m_pData,0,sizeof(LONG) * m_pulWidth[comp]);
  } else {
    //
    line = *m_pppImage[comp];
    assert(line);
    m_pppImage[comp]  = &(line->m_pNext);
  }
  
  return line;
}
///

/// LineLineAdapter::AllocateLine
// Allocate the next line for encoding. This line must
// later on then be pushed back into this buffer by
// PushLine below.
struct Line *LineLineAdapter::AllocateLine(UBYTE comp)
{
  struct Line *line;
  
  assert(comp < m_ucCount);

  // Get the next line from the image, possibly extend the image
  if (*m_pppImage[comp] == NULL) { 
    // Line is not yet there, create it.
    line = new(m_pEnviron) class Line;
    *m_pppImage[comp] = line;
    line->m_pData     = (LONG *)m_pEnviron->AllocMem((m_pulWidth[comp] * sizeof(LONG)));
  }
  //
  line = *m_pppImage[comp];
  assert(line);
  m_pppImage[comp] = &(line->m_pNext);

  return line;
}
///

/// LineLineAdapter::PushLine
// Push the next line into the output buffer.
void LineLineAdapter::PushLine(struct Line *line,UBYTE comp)
{ 

  // There is really not much to do here...
  assert(comp < m_ucCount);
  assert(line);
  assert(m_pulReadyLines[comp] < m_pulLinesPerComponent[comp]);
  NOREF(line);
  m_pulReadyLines[comp]++;
}
///

/// LineLineAdapter::ResetToStartOfImage
// Reset all components on the image side of the control to the
// start of the image. Required when re-requesting the image
// for encoding or decoding.
void LineLineAdapter::ResetToStartOfImage(void)
{ 
  for(UBYTE i = 0;i < m_ucCount;i++) {
    m_pppImage[i]      = &m_ppTop[i];
    m_pulReadyLines[i] = 0;
  }
}
///

/// LineLineAdapter::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder. Note that the data here is *not* subsampled.
bool LineLineAdapter::isNextMCULineReady(void) const
{
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_ulPixelHeight) { // There is still data to encode
      class Component *comp = m_pFrame->ComponentOf(i);
      ULONG codedlines      = m_pulCurrentY[i];
      // codedlines + comp->SubYOf() << 3 * comp->MCUHeightOf() is the number of
      // lines that must be buffered to encode the next MCU
      if (m_pulReadyLines[i] < codedlines + 8 * comp->MCUHeightOf())
	return false;
    }
  }
  
  return true;
}
///

/// LineLineAdapter::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool LineLineAdapter::isImageComplete(void) const
{
  for(UBYTE i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_pulLinesPerComponent[i])
      return false;
  }
  return true;
}
///

/// LineLineAdapter::BufferedLines
// Returns the number of lines buffered for the given component.
// Note that subsampling expansion has not yet taken place here,
// this is to be done top-level.
ULONG LineLineAdapter::BufferedLines(UBYTE i) const
{
class Component *comp = m_pFrame->ComponentOf(i);
  ULONG curline = m_pulCurrentY[i] + (comp->MCUHeightOf() << 3);
  if (curline >= m_ulPixelHeight) { // end of image
    curline = m_ulPixelHeight;
  }
  return curline;
}
///

/// LineLineAdapter::PostImageHeight
// Post the height of the frame in lines. This happens
// when the DNL marker is processed.
void LineLineAdapter::PostImageHeight(ULONG lines)
{
  LineBuffer::PostImageHeight(lines);
  LineAdapter::PostImageHeight(lines);

  assert(m_pulLinesPerComponent);

  for(UBYTE i = 0;i < m_ucCount;i++) {
    class Component *comp      = m_pFrame->ComponentOf(i);
    UBYTE suby                 = comp->SubYOf();
    m_pulLinesPerComponent[i]  = (m_ulPixelHeight + suby - 1) / suby;
  }
}
///
