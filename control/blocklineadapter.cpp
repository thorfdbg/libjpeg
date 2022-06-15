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
** This class adapts to a block buffer in a way that allows the user
** to pull out (or push in) individual lines instead of blocks. The
** purpose of this class is to implement a line-based upsampling or
** downsampling filter for the hierarchical mode. This class does not
** implement a color transformer or a upsampling filter (in the usual sense)
**
** $Id: blocklineadapter.cpp,v 1.27 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "control/blocklineadapter.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "coding/quantizedrow.hpp"
#include "dct/dct.hpp"
#include "tools/line.hpp"
#include "codestream/tables.hpp"
#include "std/string.hpp"
///

/// BlockLineAdapter::BlockLineAdapter
BlockLineAdapter::BlockLineAdapter(class Frame *frame)
  : BlockBuffer(frame), LineAdapter(frame), m_pEnviron(frame->EnvironOf()), m_pFrame(frame),
    m_ppTop(NULL), 
    m_pppQImage(NULL), m_pppImage(NULL), m_pulReadyLines(NULL),
    m_pulPixelsPerComponent(NULL), m_pulLinesPerComponent(NULL)
{
  m_ucCount = m_pFrame->DepthOf();
}
///

/// BlockLineAdapter::~BlockLineAdapter
BlockLineAdapter::~BlockLineAdapter(void)
{
  struct Line *line;
  UBYTE i;

  if (m_ppTop) {
    for(i = 0;i < m_ucCount;i++) {
      while ((line = m_ppTop[i])) {
        m_ppTop[i] = line->m_pNext;
        FreeLine(line,i);
      }
    }
    m_pEnviron->FreeMem(m_ppTop,m_ucCount * sizeof(struct Line *));
  }

  if (m_pulReadyLines)
    m_pEnviron->FreeMem(m_pulReadyLines,m_ucCount * sizeof(ULONG));

  if (m_pppQImage)
    m_pEnviron->FreeMem(m_pppQImage,m_ucCount * sizeof(class QuantizedRow **));

  if (m_pppImage)
    m_pEnviron->FreeMem(m_pppImage,m_ucCount * sizeof(struct Line **));

  if (m_pulPixelsPerComponent)
    m_pEnviron->FreeMem(m_pulPixelsPerComponent,m_ucCount * sizeof(ULONG));

  if (m_pulLinesPerComponent)
    m_pEnviron->FreeMem(m_pulLinesPerComponent,m_ucCount * sizeof(ULONG));
}
///

/// BlockLineAdapter::BuildCommon
void BlockLineAdapter::BuildCommon(void)
{
  UBYTE i;
  
  BlockBuffer::BuildCommon();
  LineAdapter::BuildCommon();

  if (m_ppTop == NULL) {
    m_ppTop = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line *));
    memset(m_ppTop,0,sizeof(struct Line *) * m_ucCount);
  }

  if (m_pulReadyLines == NULL) {
    m_pulReadyLines = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
    memset(m_pulReadyLines,0,sizeof(ULONG) * m_ucCount);
  }

  if (m_pppQImage == NULL) {
    m_pppQImage = (class QuantizedRow ***)m_pEnviron->AllocMem(m_ucCount * sizeof(class QuantizedRow **));
    memset(m_pppQImage,0,m_ucCount * sizeof(class QuantizedRow **));

    for(i = 0;i < m_ucCount;i++) {
      m_pppQImage[i]             = m_ppQTop + i;
    }
  }

  if (m_pppImage == NULL) {
    m_pppImage = (struct Line ***)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
    memset(m_pppImage,0,m_ucCount * sizeof(struct Line **));
    
    for(i = 0;i < m_ucCount;i++) {
      m_pppImage[i]              = m_ppTop  + i;
    }
  }

  if (m_pulPixelsPerComponent == NULL) {
    m_pulPixelsPerComponent = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
    for(i = 0;i < m_ucCount;i++) {
      class Component *comp      = m_pFrame->ComponentOf(i);
      UBYTE subx                 = comp->SubXOf();
      m_pulPixelsPerComponent[i] = (m_ulPixelWidth  + subx - 1) / subx;
    }
  }

  if (m_pulLinesPerComponent == NULL) {
    m_pulLinesPerComponent = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
    for(i = 0;i < m_ucCount;i++) {
      class Component *comp      = m_pFrame->ComponentOf(i);
      UBYTE suby                 = comp->SubYOf();
      m_pulLinesPerComponent[i]  = (m_ulPixelHeight + suby - 1) / suby;
    }
  }
}
///

/// BlockLineAdapter::GetNextLine
// Get the next available line from the output
// buffer on reconstruction. The caller must make
// sure that the buffer is really loaded up to the
// point or the line will be neutral grey.
struct Line *BlockLineAdapter::GetNextLine(UBYTE comp)
{
  ULONG maxval  = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;

  assert(comp < m_ucCount);
  //
  // Still a line available?
  if (m_ppTop[comp] == NULL) {
    // Must pull the next line from the frame.
    struct Line *out[8],**prev = m_ppTop + comp;
    ULONG minx   = 0;
    ULONG maxx   = (m_pulPixelsPerComponent[comp] - 1) >> 3;
    ULONG l,x;
    
    // Create eight lines to deposit the data in.
    for(l = 0;l < 8;l++) {
      struct Line *line;
      line   = AllocLine(comp);
      *prev  = line;
      prev   = &(line->m_pNext);
      out[l] = line;
    }
      
    for(x = minx;x <= maxx;x++) {
      LONG dst[64];
      class QuantizedRow *qrow = *m_pppQImage[comp];
      const LONG *src = (qrow)?(qrow->BlockAt(x)->m_Data):(NULL);
      if (src) {
        m_ppDCT[comp]->InverseTransformBlock(dst,src,(maxval + 1) >> 1);
        //
        // Copy now the buffer temporary buffer into the line. The line is always long enough
        // to cover all pixels, even those outside of the range.
        for(l = 0; l < 8;l++) {
          memcpy(out[l]->m_pData + (x << 3),&dst[l << 3],8 * sizeof(LONG));
        }
      } else {
        for(l = 0; l < 8;l++) {
          memset(out[l]->m_pData + (x << 3),0,8 * sizeof(LONG));
        }
      }
    } // of loop over x
    //
    // Advance the rows.
    class QuantizedRow *qrow = *m_pppQImage[comp];
    if (qrow) m_pppQImage[comp] = &(qrow->NextOf());
  }

  assert(m_ppTop[comp]);
  
  {
    struct Line *line = m_ppTop[comp];
    m_ppTop[comp] = line->m_pNext;
    return line;
  } 
}
///

/// BlockLineAdapter::AllocateLine
// Allocate the next line for encoding. This line must
// later on then be pushed back into this buffer by
// PushLine below.
struct Line *BlockLineAdapter::AllocateLine(UBYTE comp)
{
  assert(comp < m_ucCount);

  struct Line *line = AllocLine(comp);
  assert(*m_pppImage[comp] == NULL);
  *m_pppImage[comp] = line;
  m_pppImage[comp]  = &(line->m_pNext);

  return line;
}
///

/// BlockLineAdapter::PushLine
// Push the next line into the output buffer. If eight lines
// are accumulated (or enough lines up to the end of the image)
// these lines are automatically transferred to the input
// buffer of the block based coding back-end.
void BlockLineAdapter::PushLine(struct Line *,UBYTE comp)
{ 
  ULONG maxval                 = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  //
  // Check whether a next block of eight lines becomes available, or
  // whether the end of the image is reached.
  assert(comp < m_ucCount);
  assert(m_pulReadyLines[comp] < m_pulLinesPerComponent[comp]);
  m_pulReadyLines[comp]++;

  if (m_pulReadyLines[comp] >= m_pulLinesPerComponent[comp] || 
      (m_pulReadyLines[comp] & 0x07) == 0) {
    ULONG minx   = 0;
    ULONG maxx   = (m_pulPixelsPerComponent[comp] - 1) >> 3;
    ULONG cludge = m_pulPixelsPerComponent[comp] & 7; 
    // Offset of the first pixel to fill at the edge
    ULONG x,l;

    if (cludge) { 
      struct Line *line = m_ppTop[comp]; // replicate pixels at the edge
      for(l = 0;l < 8;l++) {
        for(x = cludge + (maxx << 3);x < (maxx + 1) << 3;x++) {
          line->m_pData[x] = line->m_pData[x-1];
        }
        if (line->m_pNext) line = line->m_pNext; // Duplicate the bottom-most line.
      }
    }
    
    for(x = minx;x <= maxx;x++) {
      struct Line *line = m_ppTop[comp];
      LONG src[64];
      //
      assert(line);
      // Copy from the line into the temporary buffer.
      for(l = 0;l < 8;l++) {
        memcpy(&src[l << 3],line->m_pData + (x << 3),8 * sizeof(LONG));
        if (line->m_pNext) line = line->m_pNext; // Duplicate the bottom-most line.
      }
      //
      // Create the target if it is not already there.
      if (*m_pppQImage[comp] == NULL) {
        *m_pppQImage[comp] = new(m_pEnviron) class QuantizedRow(m_pEnviron);
        (*m_pppQImage[comp])->AllocateRow(m_pulPixelsPerComponent[comp]);
      }
      LONG *dst = (*m_pppQImage[comp])->BlockAt(x)->m_Data;
      m_ppDCT[comp]->TransformBlock(src,dst,(maxval + 1) >> 1);
    } /* Of loop over X */
    //
    // Advance the image pointers.
    {
      class QuantizedRow *qrow = *m_pppQImage[comp];
      m_pppQImage[comp]        = &(qrow->NextOf()); 
      
      struct Line *line;
      while ((line = m_ppTop[comp])) {
        m_ppTop[comp]  = line->m_pNext;
        FreeLine(line,comp);
      }
      m_pppImage[comp] = m_ppTop + comp;
    }
  }
}
///

/// BlockLineAdapter::ResetToStartOfImage
// Reset all components on the image side of the control to the
// start of the image. Required when re-requesting the image
// for encoding or decoding.
void BlockLineAdapter::ResetToStartOfImage(void)
{ 
  for(UBYTE i = 0;i < m_ucCount;i++) {
    struct Line *line;
    m_pppQImage[i]     = &m_ppQTop[i];
    m_pppImage[i]      = &m_ppTop[i];
    while ((line = m_ppTop[i])) {
      m_ppTop[i] = line->m_pNext;
      FreeLine(line,i);
    }
    m_pulReadyLines[i] = 0;
  }
}
///

/// BlockLineAdapter::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder. Note that the data here is *not* subsampled.
bool BlockLineAdapter::isNextMCULineReady(void) const
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

/// BlockLineAdapter::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool BlockLineAdapter::isImageComplete(void) const
{
  for(UBYTE i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_pulLinesPerComponent[i])
      return false;
  }
  return true;
}
///

/// BlockLineAdapter::BufferedLines
// Returns the number of lines buffered for the given component.
// Note that subsampling expansion has not yet taken place here,
// this is to be done top-level.
ULONG BlockLineAdapter::BufferedLines(UBYTE i) const
{
  class Component *comp = m_pFrame->ComponentOf(i);
  ULONG curline = m_pulCurrentY[i] + (comp->MCUHeightOf() << 3);
  if (curline >= m_ulPixelHeight) { // end of image
    curline = m_ulPixelHeight;
  } 
  return curline;
}
///

/// BlockLineAdapter::DCOffsetOf
// In case the high-pass has a DC offset in its data, deliver it here.
LONG BlockLineAdapter::DCOffsetOf(void) const
{
  UBYTE bits = m_pFrame->HiddenPrecisionOf() + 
    m_pFrame->TablesOf()->FractionalColorBitsOf(m_pFrame->DepthOf(),m_pFrame->isDCTBased());
  LONG shift = (1UL << bits) >> 1;

  //
  // This is the inverse shift of the DCT.
  return shift;
}
///

/// BlockLineAdapter::PostImageHeight
// Post the height of the frame in lines. This happens
// when the DNL marker is processed.
void BlockLineAdapter::PostImageHeight(ULONG lines)
{
  BlockBuffer::PostImageHeight(lines);
  LineAdapter::PostImageHeight(lines);
  
  assert(m_pulLinesPerComponent);
  
  for(UBYTE i = 0;i < m_ucCount;i++) {
    class Component *comp      = m_pFrame->ComponentOf(i);
    UBYTE suby                 = comp->SubYOf();
    m_pulLinesPerComponent[i]  = (m_ulPixelHeight + suby - 1) / suby;
  }
}
///
