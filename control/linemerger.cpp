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
** This class merges the two sources of a differential frame together,
** expanding its non-differential source.
**
** $Id: linemerger.cpp,v 1.28 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/line.hpp"
#include "control/lineadapter.hpp"
#include "control/linemerger.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "colortrafo/colortrafo.hpp"
#include "std/string.hpp"
#define COLOR_BITS ColorTrafo::COLOR_BITS
///

/// LineMerger::LineMerger
// The frame to create the line merger from is the highpass frame as
// its line dimensions are identical to that of the required output.
LineMerger::LineMerger(class Frame *frame,class LineAdapter *low,class LineAdapter *high,
                       bool expandhor,bool expandver)
  : LineAdapter(frame)
#if ACCUSOFT_CODE
  , m_pFrame(frame), 
    m_pLowPass(low), m_pHighPass(high), 
    m_ppVBuffer(NULL), m_ppHBuffer(NULL), m_ppIBuffer(NULL),
    m_ppFirstLine(NULL), m_pppImage(NULL),
    m_ppTop(NULL), m_ppCenter(NULL), m_ppBottom(NULL),
    m_pulPixelWidth(NULL), m_pulPixelHeight(NULL), m_pulY(NULL),
    m_bExpandH(expandhor), m_bExpandV(expandver)
#endif
{
#if ACCUSOFT_CODE
#else
  NOREF(frame);
  NOREF(low);
  NOREF(high);
  NOREF(expandhor);
  NOREF(expandver);
#endif
}
///

/// LineMerger::~LineMerger
LineMerger::~LineMerger(void)
{
#if ACCUSOFT_CODE
  UBYTE i;
  
  if (m_ppVBuffer) {
    for(i = 0;i < m_ucCount;i++) {
      FreeLine(m_ppVBuffer[i],i);
    }
    m_pEnviron->FreeMem(m_ppVBuffer,m_ucCount * sizeof(struct Line *));
  }  

  if (m_ppHBuffer) {
    for(i = 0;i < m_ucCount;i++) {
      FreeLine(m_ppHBuffer[i],i);
    }
    m_pEnviron->FreeMem(m_ppHBuffer,m_ucCount * sizeof(struct Line *));
  }
  
  if (m_ppIBuffer) {
    for(i = 0;i < m_ucCount;i++) {
      FreeLine(m_ppIBuffer[i],i);
    }
    m_pEnviron->FreeMem(m_ppIBuffer,m_ucCount * sizeof(struct Line *));
  }

  if (m_ppFirstLine) {
    for(i = 0;i < m_ucCount;i++) {
      struct Line *line;
      while((line = m_ppFirstLine[i])) {
        m_ppFirstLine[i] = line->m_pNext;
        FreeLine(line,i);
      }
    }
    m_pEnviron->FreeMem(m_ppFirstLine,m_ucCount * sizeof(struct Line *));
  }
  
  if (m_pppImage) {
    m_pEnviron->FreeMem(m_pppImage,m_ucCount * sizeof(struct Line **));
  }

  //
  // Top, center and bottom are all part of the image and
  // hence released there.
  if (m_ppTop) {
    m_pEnviron->FreeMem(m_ppTop,m_ucCount * sizeof(struct Line *));
  }
  
  if (m_ppCenter) {
    m_pEnviron->FreeMem(m_ppCenter,m_ucCount * sizeof(struct Line *));
  }
  
  if (m_ppBottom) {
    m_pEnviron->FreeMem(m_ppBottom,m_ucCount * sizeof(struct Line *));
  }
  
  if (m_pulY) {
    m_pEnviron->FreeMem(m_pulY,m_ucCount * sizeof(ULONG));
  }

  if (m_pulPixelWidth) {
    m_pEnviron->FreeMem(m_pulPixelWidth,m_ucCount * sizeof(ULONG));
  }

  if (m_pulPixelHeight) {
    m_pEnviron->FreeMem(m_pulPixelHeight,m_ucCount * sizeof(ULONG));
  }
#endif
}
///

/// LineMerger::BuildCommon
// Second-stage constructor, construct the internal details.
void LineMerger::BuildCommon(void)
{
#if ACCUSOFT_CODE
  UBYTE i;

  LineAdapter::BuildCommon();

  if (m_ppVBuffer == NULL) {
    m_ppVBuffer = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line *));
    memset(m_ppVBuffer,0,sizeof(struct Line *) * m_ucCount);
  }
  
  if (m_ppHBuffer == NULL) {
    m_ppHBuffer = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line *));
    memset(m_ppHBuffer,0,sizeof(struct Line *) * m_ucCount);
  }
  
  if (m_ppIBuffer == NULL) {
    m_ppIBuffer = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line *));
    memset(m_ppIBuffer,0,sizeof(struct Line *) * m_ucCount);
  }

  if (m_pppImage == NULL) {
    m_pppImage = (struct Line ***)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
  }

  if (m_ppTop == NULL) {
    m_ppTop = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
    memset(m_ppTop,0,sizeof(struct Line *) * m_ucCount);
  }

  if (m_ppCenter == NULL) {
    m_ppCenter = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
    memset(m_ppCenter,0,sizeof(struct Line *) * m_ucCount);
  }

  if (m_ppBottom == NULL) {
    m_ppBottom = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
    memset(m_ppBottom,0,sizeof(struct Line *) * m_ucCount);
  }
  
  if (m_ppFirstLine == NULL) {
    m_ppFirstLine = (struct Line **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct Line **));
    memset(m_ppFirstLine,0,sizeof(struct Line *) * m_ucCount);
  }
  
  if (m_pulY == NULL) {
    m_pulY = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
  }
  
  if (m_pulPixelWidth == NULL) {
    ULONG w = m_pFrame->WidthOf();
    ULONG h = m_pFrame->HeightOf();
    
    assert(m_pulPixelHeight == NULL);

    m_pulPixelWidth  = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
    m_pulPixelHeight = (ULONG *)m_pEnviron->AllocMem(m_ucCount * sizeof(ULONG));
    
    for(i = 0;i < m_ucCount;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE subx            = comp->SubXOf();
      UBYTE suby            = comp->SubYOf();
      m_pulPixelWidth[i]    = (w + subx - 1) / subx;
      m_pulPixelHeight[i]   = (h + suby - 1) / suby;
      m_pppImage[i]         = m_ppFirstLine + i;
      m_pulY[i]             = 0;
    }
  }
#endif
}
///


/// LineMerger::GetNextLowPassLine
// Fetch the next line from the low-pass and expand it horizontally if required.
#if ACCUSOFT_CODE
struct Line *LineMerger::GetNextLowpassLine(UBYTE comp)
{
  struct Line *line,*xline = AllocLine(comp);
  assert(m_ppHBuffer[comp] == NULL);
  m_ppHBuffer[comp]        = xline;
  //
  line                     = m_pLowPass->GetNextLine(comp);
  //
  // If horizontal expansion is required, do this now.
  if (m_bExpandH) {
    // Replicate the last pixel to make things simpler. There is enough storage allocated.
    // The destionation gets potentially one line longer.
    LONG *src  = line->m_pData;
    LONG *last = line->m_pData + ((m_pulPixelWidth[comp] + 1) >> 1);
    LONG *dst  = xline->m_pData;
    last[0]    = last[-1];
    do {
      *dst++   =  src[0];
      *dst++   = (src[0] + src[1]) >> 1;
    } while(++src < last);
  } else {
    memcpy(xline->m_pData,line->m_pData,m_pulPixelWidth[comp] * sizeof(LONG));
  }
  //
  m_pLowPass->ReleaseLine(line,comp);

  return xline;
}
#endif
///

/// LineMerger::GetNextExpandedLowPassLine
// Fetch a line from the low-pass filter and expand it in horizontal
// or vertical direction. Do not do anything else.
#if ACCUSOFT_CODE
struct Line *LineMerger::GetNextExpandedLowPassLine(UBYTE comp)
{
  // If vertical expansion is required, a new line is required from the lowpass:
  // 1) for the first line to have a reference,
  // 2) then on every odd line.
  if (m_bExpandV) {
    struct Line *line;
    // Fetch a next line on the first round, or on odd lines. The last
    // line of the image is replicated.
    if (m_pulY[comp] == 0 ||        // first line
        ((m_pulY[comp] & 1) &&      // or odd
         (m_pulPixelHeight[comp] == 0 ||  // and not last line
          ((m_pulY[comp] + 1) >> 1) < ((m_pulPixelHeight[comp] + 1) >> 1)))) {
      line = GetNextLowpassLine(comp);
      //
      // Buffer for the next line in between if this is the first line.
      if (m_pulY[comp] == 0) {
        assert(m_ppVBuffer[comp] == NULL);
        assert(m_ppHBuffer[comp] == line);
        m_ppVBuffer[comp] = line;
        m_ppHBuffer[comp] = NULL;
      }
    } else {
      // The buffered line will do.
      line = m_ppVBuffer[comp];
    }
    //
    // Line contains now the new line to expand. If we are currently an
    // even line, then this is all what is required. If we are an odd
    // line, this line must be merged with the previous line to get the final
    if (m_pulY[comp] & 1) {
      struct Line *out  = m_ppIBuffer[comp]?m_ppIBuffer[comp]:AllocLine(comp);
      struct Line *prev = m_ppVBuffer[comp];
      struct Line *next = line;
      LONG *dst         = out->m_pData;
      LONG *end         = out->m_pData + m_pulPixelWidth[comp];
      LONG *src1        = prev->m_pData;
      LONG *src2        = next->m_pData;
      // Odd line. Final output is composed of: This line (which is the next) 
      // merged with the previous plus the difference.
      do {
        *dst = (*src1++ + *src2++) >> 1;
      } while(++dst < end);
      //
      // Release the old buffered line which is no longer required
      // and store the new one there. This releases the old VBuffer.
      m_ppHBuffer[comp]   = NULL;
      if (prev != next) {
        FreeLine(prev,comp);
        m_ppVBuffer[comp] = next;
      }
      m_ppIBuffer[comp]   = out;
      //
      m_pulY[comp]++;
      return out;
    } else {
      // No interpolation necessary, data comes directly from the output buffer.
      m_pulY[comp]++;
      assert(m_ppVBuffer[comp] == line);
      return line;
    }
  } else {
    struct Line *line;
    line = GetNextLowpassLine(comp);
    //
    // For consistency, place into the VBuffer.
    assert(line == m_ppHBuffer[comp]);
    if (m_ppVBuffer[comp]) FreeLine(m_ppVBuffer[comp],comp);
    m_ppVBuffer[comp] = line;
    m_ppHBuffer[comp] = NULL;
    return line;
  }
}
#endif
///

/// LineMerger::GetNextLine
// Get the next available line from the output
// buffer on reconstruction. The caller must make
// sure that the buffer is really loaded up to the
// point or the line will be neutral grey.
struct Line *LineMerger::GetNextLine(UBYTE comp)
{
#if ACCUSOFT_CODE
  struct Line *low  = GetNextExpandedLowPassLine(comp);
  struct Line *high = m_pHighPass->GetNextLine(comp);
  LONG shift        = m_pHighPass->DCOffsetOf();
  //
  // Add up the lines.
  LONG *src = low->m_pData;
  LONG *dst = high->m_pData;
  LONG *end = high->m_pData + m_pulPixelWidth[comp];
  if (m_pHighPass->isLossless()) {
    do {
      *dst = ((*dst >> COLOR_BITS) + ((*src++ - shift) >> COLOR_BITS)) << COLOR_BITS;
      //*dst++ = *src++; // low-pass only
      //dst++;           // high-pass only
    } while(++dst < end);
  } else {
    do {
      *dst += *src++ - shift; // the DCT always adds the level shift, so subtract it.
      //*dst = *src++; // low-pass only
      //dst++;           // high-pass only
    } while(++dst < end);
  }

  return high;
#else
  NOREF(comp);
  return NULL;
#endif
}
///

/// LineMerger::ReleaseLine
// Release a line passed out to the user. As the lines always come from the highpass,
// let the highpass release it.
void LineMerger::ReleaseLine(struct Line *line,UBYTE comp)
{
#if ACCUSOFT_CODE
  m_pHighPass->ReleaseLine(line,comp);
#else
  NOREF(line);
  NOREF(comp);
#endif
}
///

/// LineMerger::AllocateLine
// Allocate a new line for encoding.
struct Line *LineMerger::AllocateLine(UBYTE comp)
{
#if ACCUSOFT_CODE
  struct Line *line;
  //
  // As we release ourselves...
  line              = AllocLine(comp);
  *m_pppImage[comp] = line;
  m_pppImage[comp]  = &(line->m_pNext);

  return line;
#else
  NOREF(comp);
  return NULL;
#endif
}
///

/// LineMerger::PushLine
// Push the next line into the output buffer. If eight lines
// are accumulated (or enough lines up to the end of the image)
// these lines are automatically transferred to the input
// buffer of the block based coding back-end.
void LineMerger::PushLine(struct Line *line,UBYTE comp)
{
#if ACCUSOFT_CODE
  if (m_bExpandV) {
    // A new differential line can be computed on odd lines, and on the last line of the image.
    // For odd lines, the line before, and the line before that line are required.
    if (m_pulY[comp] & 1) {
      // Place the next row into the bottom buffer, its old content goes into top.
      assert(m_ppTop[comp] == NULL);
      m_ppTop[comp]        = m_ppBottom[comp];
      m_ppBottom[comp]     = line;
      //
      struct Line *center  = m_ppCenter[comp];
      struct Line *bottom  = line;
      struct Line *top     = m_ppTop[comp];
      struct Line *out;
      //
      // Allocate a temporary or final output line. If horizontal subsampling is included,
      // this goes from our buffer, otherwise it comes from the lowpass where the line
      // will end up, finally.
      assert(m_ppHBuffer[comp] == NULL);
      out = AllocLine(comp);
      m_ppHBuffer[comp] = out;
      //
      // Replicate at the edges.
      if (top == NULL)
        top = bottom;
      assert(top && bottom && center);
      //
      // Now filter.
      LONG *dst = out->m_pData;
      LONG *end = out->m_pData + m_pulPixelWidth[comp];
      LONG *tp  = top->m_pData;
      LONG *cp  = center->m_pData;
      LONG *bp  = bottom->m_pData;
      do {
        *dst++ = (*tp++ + (*cp++ << 1) + *bp++ + 1) >> 2;
      } while(dst < end);
      //
      // The new low pass is ready now in HBuffer. Whether it will be pushed
      // directly or filtered depends on whether horizontal filtering is done.
      //
      // The current top buffer is no longer required, leave it in the image,
      // though.
      m_ppTop[comp]    = NULL;
      // The center line is no longer required, too.
      m_ppCenter[comp] = NULL;
    } else {
      // An even line - just keep the buffer.
      assert(m_ppCenter[comp] == NULL);
      m_ppCenter[comp] = line;
      //
      // Special case if this is the last line of the image: This then generates the
      // last line of the low-pass, otherwise the line is just buffered.
      if (m_pulPixelHeight[comp] && m_pulY[comp] >= m_pulPixelHeight[comp] - 1) {
        struct Line *center  = m_ppCenter[comp];
        struct Line *bottom  = m_ppBottom[comp]; // Top line is replicated into bottom.
        struct Line *top     = m_ppBottom[comp];
        struct Line *out;
        //
        assert(m_ppHBuffer[comp] == NULL);
        out = AllocLine(comp);
        m_ppHBuffer[comp] = out;
        //
        //
        if (top && bottom) {
          // If there are lines now, filter. In the special case of only having a single
          // line, just replicate and set the high-pass to zero.
          LONG *dst = out->m_pData;
          LONG *end = out->m_pData + m_pulPixelWidth[comp];
          LONG *tp  = top->m_pData;
          LONG *cp  = center->m_pData;
          LONG *bp  = bottom->m_pData;
          do {
            *dst++ = (*tp++ + (*cp++ << 1) + *bp++ + 1) >> 2;
          } while(dst < end);
        } else {
          // Special case single line image: Just copy the data.
          memcpy(out->m_pData,center->m_pData,sizeof(LONG) * m_pulPixelWidth[comp]);
        }
      }
    }
  } else {
    // No vertical subsampling. Just take the line we got.
    m_ppHBuffer[comp] = line;
  }
  //
  // The vertically filtered (or not filtered) data is now in the HBuffer, or it is NULL
  // if no output has been generated this time.
  if (m_ppHBuffer[comp]) {
    // If here is horizontal filtering, apply now.
    if (m_bExpandH) {
      struct Line *out = m_pLowPass->AllocateLine(comp);
      LONG *dst        = out->m_pData;
      LONG *end        = out->m_pData + ((m_pulPixelWidth[comp] + 1) >> 1);
      LONG *src        = m_ppHBuffer[comp]->m_pData;
      // Replicate at the end.
      src[m_pulPixelWidth[comp]] = src[m_pulPixelWidth[comp] - 1];
      LONG left        = src[1]; // the left pixel is replicated from the right.
      LONG center,right;
      do {
        center = src[0];
        right  = src[1];
        *dst++ = (left + (center << 1) + right + 1) >> 2;
        left   = right;
        src   += 2;
      } while(dst < end);
      //
      // Done with it.
      m_pLowPass->PushLine(out,comp);
      if (m_bExpandV)
        FreeLine(m_ppHBuffer[comp],comp); // was a temporary.
      m_ppHBuffer[comp] = NULL;
    } else {
      struct Line *out = m_pLowPass->AllocateLine(comp);
      struct Line *src = m_ppHBuffer[comp];
      // Just push directly unfiltered. Must copy over as this class needs to
      // keep the original.
      memcpy(out->m_pData,src->m_pData,m_pulPixelWidth[comp] * sizeof(LONG));
      m_pLowPass->PushLine(out,comp); 
      if (m_bExpandV)
        FreeLine(m_ppHBuffer[comp],comp); // was a temporary.
      m_ppHBuffer[comp] = NULL;
    }
  }
  //
  // Next line.
  m_pulY[comp]++;
#else
  NOREF(line);
  NOREF(comp);
#endif
}
///

/// LineMerger::ResetToStartOfImage
// Rewind the image buffer to the start of the image
void LineMerger::ResetToStartOfImage(void)
{
#if ACCUSOFT_CODE
  UBYTE i;

  for(i = 0;i < m_ucCount;i++) {
    m_pppImage[i] = m_ppFirstLine + i;
    m_pulY[i]     = 0;
    if (m_ppVBuffer[i]) {
      FreeLine(m_ppVBuffer[i],i);
      m_ppVBuffer[i] = NULL;
    }
    if (m_ppHBuffer[i]) {
      FreeLine(m_ppHBuffer[i],i);
      m_ppHBuffer[i] = NULL;
    }
  }

  if (m_pHighPass) m_pHighPass->ResetToStartOfImage();
  if (m_pLowPass)  m_pLowPass->ResetToStartOfImage();
#endif
}
///

/// LineMerger::GenerateDifferentialImage
// Generate a differential image by pulling the reconstructed
// image from the low-pass and pushing the differential signal
// into the high-pass.
void LineMerger::GenerateDifferentialImage(void)
{
#if ACCUSOFT_CODE
  UBYTE comp;
  ULONG y;
  LONG shift = m_pHighPass->DCOffsetOf();  

  ResetToStartOfImage(); // also resets the subbands
  
  for(comp = 0;comp < m_ucCount;comp++) {
    ULONG height = m_pulPixelHeight[comp];

    // If this is NULL, then the data has already been pushed
    // before, probably because we measured in the iteration before.
    if (m_ppFirstLine[comp]) {
      for(y = 0;y < height;y++) {
        struct Line *low  = GetNextExpandedLowPassLine(comp);
        struct Line *high = m_pHighPass->AllocateLine(comp);
        struct Line *top  = m_ppFirstLine[comp]; // Old buffered lines.
        //
        if (top == NULL)
          JPG_THROW(OBJECT_DOESNT_EXIST,"LineMerger::GenerateDifferentialImage",
                    "cannot create the next frame of the differential image, the previous frame is still incomplete");
        //
        // Now compute the difference. This becomes the high-pass output.
        LONG *dst = high->m_pData;
        LONG *end = high->m_pData + m_pulPixelWidth[comp];
        LONG *rec = low->m_pData;
        LONG *org = top->m_pData;
        if (m_pHighPass->isLossless()) {
          do {
            // The DCT always removes the level shift, so simply add it here.
            *dst = ((*org++ >> COLOR_BITS) - ((*rec++ + shift) >> COLOR_BITS)) << COLOR_BITS;
          } while(++dst < end);
        } else {
          do {
            // The DCT always removes the level shift, so simply add it here.
            *dst = *org++ - *rec++ + shift;
          } while(++dst < end);
        }
        //
        // Done with it. Also releases high.
        m_pHighPass->PushLine(high,comp);
        //
        // And the corresponding source line can also go.
        m_ppFirstLine[comp]     = top->m_pNext;
        FreeLine(top,comp);
      }
    }
  }
#endif
}
///

/// LineMerger::PostImageHeight
// Post the height of the frame in lines. This happens
// when the DNL marker is processed.
void LineMerger::PostImageHeight(ULONG lines)
{
  LineAdapter::PostImageHeight(lines);

#if ACCUSOFT_CODE
  assert(m_pulPixelHeight);
  
  if (m_pLowPass)
    m_pLowPass->PostImageHeight((lines + 1) >> 1);
  if (m_pHighPass)
    m_pHighPass->PostImageHeight(lines);

  for(UBYTE i = 0;i < m_ucCount;i++) {
    class Component *comp = m_pFrame->ComponentOf(i);
    UBYTE suby            = comp->SubYOf();
    m_pulPixelHeight[i]   = (lines + suby - 1) / suby;
  }
#endif
}
///
