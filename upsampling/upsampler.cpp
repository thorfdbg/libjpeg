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
** This file defines a class that implements the component upsampling
** procedure.
**
** $Id: upsampler.cpp,v 1.17 2018/07/27 06:56:43 thor Exp $
**
*/


/// Includes
#include "upsampling/upsampler.hpp"
#include "std/string.hpp"
///

/// Horizontal and vertical filter cores
// The actual implementations: Filter vertically from the line into the 8x8 buffer
template<int sy>
void VerticalFilterCore(int ymod,struct Line *top,struct Line *cur,struct Line *bot,LONG offset,LONG *target);

template<int sx>
void HorizontalFilterCore(int xmod,LONG *target);
///

/// Upsampler::Upsampler
template<int sx,int sy>
Upsampler<sx,sy>::Upsampler(class Environ *env,ULONG width,ULONG height)
  : UpsamplerBase(env,sx,sy,width,height)
{
}
///

/// Upsampler::~Upsampler
template<int sx,int sy>
Upsampler<sx,sy>::~Upsampler(void)
{
}
///

/// Upsampler::UpsampleRegion
// The actual upsampling process.
template<int sx,int sy>
void Upsampler<sx,sy>::UpsampleRegion(const RectAngle<LONG> &r,LONG *buffer) const
{
  LONG y = (r.ra_MinY / sy);     // The line offset of the current line.
  LONG x = (r.ra_MinX / sx) + 1; // the data offset such that data + offset + 0 is the pixel at the point
  LONG cy = m_lY;
  struct Line *top,*cur,*bot;    // Line pointers.

  assert(y >= m_lY && y < m_lY + m_lHeight); // Must be in the buffer.

  // Get the topmost line, one above the current position.
  top = m_pInputBuffer;
  while(cy < y - 1) {
    top = top->m_pNext;
    cy++;
  }
  //
  // Get the next line.
  cur = top;
  if (y > m_lY)
    cur = cur->m_pNext; // duplicate the top line for the first line.

  bot = cur;
  if (bot->m_pNext)
    bot = bot->m_pNext; // duplicate bottom at the last line.

  if (sx > 1)
    x--; // copy one additional pixel from the left in case we need to expand horizontally.
  VerticalFilterCore<sy>(r.ra_MinY % sy,top,cur,bot,x,buffer);
  HorizontalFilterCore<sx>(r.ra_MinX % sx,buffer);
}
///

/// VerticalFilterCore<1>
// The actual implementations: Filter vertically from the line into the 8x8 buffer
template<>
void UpsamplerBase::VerticalFilterCore<1>(int,struct Line *,struct Line *cur,struct Line *,
                                          LONG offset,LONG *target)
{
  int lines = 8;
  
  do {
    memcpy(target,cur->m_pData + offset,8 * sizeof(LONG));
    if (cur->m_pNext)
      cur = cur->m_pNext;
    target += 8;
  } while(--lines);

}
///

/// VerticalFilterCore<2>
// The actual implementations: Filter vertically from the line into the 8x8 buffer
template<>
void UpsamplerBase::VerticalFilterCore<2>(int ymod,struct Line *top,struct Line *cur,struct Line *bot,
                                          LONG offset,LONG *target)
{
  int lines = 8;
  
  do {
    LONG *out = target;
    LONG *end = target + 8;
    LONG *t   = top->m_pData + offset;
    LONG *c   = cur->m_pData + offset;
    LONG *b   = bot->m_pData + offset;
    switch(ymod) {
    case 0: // even lines
      do {
        *out++ = (*t++ + 3 * *c++ + 2) >> 2;
        *out++ = (*t++ + 3 * *c++ + 1) >> 2;
      } while(out < end);
      ymod++;
      break;
    case 1: // odd lines
      do {
        *out++ = (*b++ + 3 * *c++ + 1) >> 2;
        *out++ = (*b++ + 3 * *c++ + 2) >> 2;
      } while(out < end);
      ymod = 0;
      top  = cur;
      cur  = bot;
      if (bot->m_pNext) bot = bot->m_pNext;
      break;
    }
    target += 8; // next line.
  } while(--lines);
}
///

/// VerticalFilterCore<3>
// The actual implementations: Filter vertically from the line into the 8x8 buffer
template<>
void UpsamplerBase::VerticalFilterCore<3>(int ymod,struct Line *top,struct Line *cur,struct Line *bot,
                                          LONG offset,LONG *target)
{
  int lines = 8;
  
  // 
  // This is not exactly a bilinear filter, but it is easer
  // to implement this way. The filter coefficients would ideally
  // be (1/3,2/3), not (1/4,3/4).
  do {
    LONG *out = target;
    LONG *end = target + 8;
    LONG *t   = top->m_pData + offset;
    LONG *c   = cur->m_pData + offset;
    LONG *b   = bot->m_pData + offset;
    switch(ymod) {
    case 0: 
      do {
        *out++ = (*t++ + 3 * *c++ + 2) >> 2;
        *out++ = (*t++ + 3 * *c++ + 1) >> 2;
      } while(out < end);
      ymod++;
      break;
    case 1:
      memcpy(out,c,8 * sizeof(LONG));
      ymod++;
      break;
    case 2: 
      do {
        *out++ = (*b++ + 3 * *c++ + 1) >> 2;
        *out++ = (*b++ + 3 * *c++ + 2) >> 2;
      } while(out < end);
      ymod = 0; 
      top  = cur;
      cur  = bot;
      if (bot->m_pNext) bot = bot->m_pNext;
      break;
    }
    target += 8; // next line.
 
  } while(--lines);
}
///

/// VerticalFilterCore<4>
// The actual implementations: Filter vertically from the line into the 8x8 buffer
template<>
void UpsamplerBase::VerticalFilterCore<4>(int ymod,struct Line *top,struct Line *cur,struct Line *bot,
                           LONG offset,LONG *target)
{
  int lines = 8;
  
  // 
  do {
    LONG *out = target;
    LONG *end = target + 8;
    LONG *t   = top->m_pData + offset;
    LONG *c   = cur->m_pData + offset;
    LONG *b   = bot->m_pData + offset;
    switch(ymod) {
    case 0: 
      do {
        *out++ = (3 * *t++ + 5 * *c++ + 4) >> 3;
        *out++ = (3 * *t++ + 5 * *c++ + 3) >> 3;
      } while(out < end);
      ymod++;
      break;
    case 1:
      do {
        *out++ = (1 * *t++ + 7 * *c++ + 3) >> 3;
        *out++ = (1 * *t++ + 7 * *c++ + 4) >> 3;
      } while(out < end);
      ymod++;
      break; 
    case 2:
      do {
        *out++ = (1 * *b++ + 7 * *c++ + 4) >> 3;
        *out++ = (1 * *b++ + 7 * *c++ + 3) >> 3;
      } while(out < end);
      ymod++;
      break;
    case 3: 
      do {
        *out++ = (3 * *b++ + 5 * *c++ + 4) >> 3;
        *out++ = (3 * *b++ + 5 * *c++ + 3) >> 3;
      } while(out < end);
      ymod = 0;
      top  = cur;
      cur  = bot;
      if (bot->m_pNext) bot = bot->m_pNext;
      break;
    }
    target += 8; // next line.
  } while(--lines);
}
///

/// HorizontalFilterCore<1>
// The actual implementations: Filter horizontally from the line into the 8x8 buffer
template<>
void UpsamplerBase::HorizontalFilterCore<1>(int,LONG *)
{
  // Here the buffer is already aligned correctly. Nothing else to do.
}
///

/// HorizontalFilterCore<2>
// The actual implementations: Filter horizontally from the line into the 8x8 buffer
template<>
void UpsamplerBase::HorizontalFilterCore<2>(int xmod,LONG *target)
{
  int lines = 8;

  assert(xmod == 0); // blocks are aligned by multiples of eight

  NOREF(xmod);

  do {
    LONG *src = target + 1; // src[-1] is the pixel to the left, input is offset by one pixel.
    LONG *out = target;
    LONG t;
    
    out[7] = (src[ 4]  + 3 * src[3] + 1) >> 2;
    out[6] = (src[ 2]  + 3 * src[3] + 2) >> 2;
    out[5] = (src[ 3]  + 3 * src[2] + 1) >> 2;
    out[4] = (src[ 1]  + 3 * src[2] + 2) >> 2;
    out[3] = (src[ 2]  + 3 * src[1] + 1) >> 2;
    out[2] = (src[ 0]  + 3 * src[1] + 2) >> 2; t = src[0]; // will be overwritten as out[1]
    out[1] = (src[ 1]  + 3 * t      + 1) >> 2;
    out[0] = (src[-1]  + 3 * t      + 2) >> 2;
    
    target += 8;
  } while(--lines);
}
///

/// HorizontalFilterCore<3>
// The actual implementations: Filter horizontally from the line into the 8x8 buffer
template<>
void UpsamplerBase::HorizontalFilterCore<3>(int xmod,LONG *target)
{
  int lines = 8;

  do {
    LONG *src = target + 1; // src[-1] is the pixel to the left, input is offset by one pixel.
    LONG *out = target;
    LONG t;
    
    switch(xmod) {
    case 0:
      out[7] = src[2];
      out[6] = (src[ 1] + 3 * src[2] + 2) >> 2;
      out[5] = (src[ 2] + 3 * src[1] + 1) >> 2;
      out[4] = src[1];
      out[3] = (src[ 0] + 3 * src[1] + 2) >> 2;
      out[2] = (src[ 1] + 3 * src[0] + 1) >> 2;
      out[0] = (src[-1] + 3 * src[0] + 2) >> 2;
      out[1] = src[0]; // out[1] is src[0]
      break; 
    case 1:
      out[7] = (src[ 3] + 3 * src[ 2] + 1) >> 2;
      out[6] = src[2];
      out[5] = (src[ 1] + 3 * src[ 2] + 2) >> 2;
      out[4] = (src[ 2] + 3 * src[ 1] + 1) >> 2;
      out[3] = src[1]; t = src[0];
      out[2] = (      t + 3 * src[ 1] + 2) >> 2;
      out[1] = (src[ 1] + 3 * t       + 1) >> 2;
      out[0] = t; 
      break;
    case 2:
      out[7] = (src[ 2] + 3 * src[ 3] + 2) >> 2;
      out[6] = (src[ 3] + 3 * src[ 2] + 1) >> 2;
      out[5] = src[2];
      out[4] = (src[ 1] + 3 * src[ 2] + 2) >> 2;
      out[3] = (src[ 2] + 3 * src[ 1] + 1) >> 2;
      out[2] = src[1]; t = src[0]; // will be overwritten
      out[1] = (t       + 3 * src[1]  + 2) >> 2;
      out[0] = (src[1]  + 3 * t       + 1) >> 2;
      break;
    }

    target += 8;
  } while(--lines);
}
///

/// HorizontalFilterCore<4>
// The actual implementations: Filter horizontally from the line into the 8x8 buffer
template<>
void UpsamplerBase::HorizontalFilterCore<4>(int xmod,LONG *target)
{  
  int lines = 8;

  assert(xmod == 0); // blocks are aligned by multiples of eight
  NOREF(xmod);

  do {
    LONG *src = target + 1; // src[-1] is the pixel to the left, input is offset by one pixel.
    LONG *out = target;
    LONG t;
    
    out[7] = (3 * src[ 2] + 5 * src[1] + 1) >> 3;
    out[6] = (1 * src[ 2] + 7 * src[1] + 2) >> 3;
    out[5] = (1 * src[ 0] + 7 * src[1] + 1) >> 3;
    out[4] = (3 * src[ 0] + 5 * src[1] + 2) >> 3; t = src[0]; // is out[1]
    out[3] = (3 * src[ 1] + 5 *      t + 1) >> 3;
    out[2] = (1 * src[ 1] + 7 *      t + 2) >> 3;
    out[1] = (1 * src[-1] + 7 *      t + 1) >> 3;
    out[0] = (3 * src[-1] + 5 *      t + 2) >> 3;

    target += 8;
  } while(--lines);
}
///

/// Explicit instaciations
template class Upsampler<1,1>;
template class Upsampler<1,2>;
template class Upsampler<1,3>;
template class Upsampler<1,4>;
template class Upsampler<2,1>;
template class Upsampler<2,2>;
template class Upsampler<2,3>;
template class Upsampler<2,4>;
template class Upsampler<3,1>;
template class Upsampler<3,2>;
template class Upsampler<3,3>;
template class Upsampler<3,4>;
template class Upsampler<4,1>;
template class Upsampler<4,2>;
template class Upsampler<4,3>;
template class Upsampler<4,4>;
///
