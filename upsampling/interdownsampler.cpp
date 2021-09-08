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
** The actual downsampling implementation.
** This is an interpolating downsampler with an
** additional 1-line delay. This works if there
** is no residual image.
**
** $Id: interdownsampler.cpp,v 1.2 2020/04/08 10:05:42 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "upsampling/downsamplerbase.hpp"
#include "upsampling/interdownsampler.hpp"
#include "std/string.hpp"
///

/// InterDownsampler::InterDownsampler
template<int sx,int sy>
InterDownsampler<sx,sy>::InterDownsampler(class Environ *env,ULONG width,ULONG height)
  : DownsamplerBase(env,sx,sy,width,height,true)
{
}
///

/// InterDownsampler::~InterDownsampler
template<int sx,int sy>
InterDownsampler<sx,sy>::~InterDownsampler(void)
{
}
///

/// InterDownsampler::DownsampleRegion
// The actual downsampling process. To be implemented by the actual
// classes inheriting from this. Coordinates are in the downsampled
// block domain the block indices. Requires an output buffer that
// will keep the downsampled data.
template<int sx,int sy>
void InterDownsampler<sx,sy>::DownsampleRegion(LONG bx,LONG by,LONG *buffer) const
{
  LONG topbuffer[8];
  LONG bottombuffer[8];
  LONG ofs = (bx * sx) << 3; // first pixel in the buffer.
  LONG yfs = (by * sy) << 3; // first line.
  int lines = 0; // number of lines already managed to be summed up.
  int cnt   = 8; // number of output lines to go.
  struct Line *line = m_pInputBuffer;
  struct Line *top  = line,*bot;  // the line above and below the current line.
  LONG y    = m_lY;

  assert(yfs >= m_lY && yfs < m_lY + m_lHeight);

  //
  // Get the line.
  while(y < yfs) {
    top  = line;
    line = line->m_pNext;
    y++;
  }
  assert(line);
  bot = line->m_pNext;
  if (bot == NULL)
    bot = line;
  assert(yfs == 0 || line != top);

  do {
    //
    // Start of a new line clear the entire output buffer.
    if (lines == 0) {
      // If there are no longer any lines to pull from, just replicate the last buffered line
      assert(line);
      memset(buffer,0,8 * sizeof(LONG));
      memset(topbuffer,0,8 * sizeof(LONG));
      memset(bottombuffer,0,8 * sizeof(LONG));
    }
    //
    // Still something in the image?
    {
      LONG *src  = line->m_pData + ofs + 1;  // Current input buffer position.
      LONG *srct = top->m_pData  + ofs + 1;  // Current input buffer position.
      LONG *srcb = bot->m_pData  + ofs + 1;  // Current input buffer position.
      LONG *bp   = buffer;
      LONG *bpt  = topbuffer;
      LONG *bpb  = bottombuffer;
      int i      = 8; // pixel in the line

      do {
        switch(sx) { // actually this will be unrolled because it is a template
        case 2:
          *bpt += (srct[-1] + 3 * srct[0] + 3 * srct[1] + srct[2] + 2) >> 2;
          *bp  += (src [-1] + 3 * src [0] + 3 * src [1] + src [2] + 2) >> 2;
          *bpb += (srcb[-1] + 3 * srcb[0] + 3 * srcb[1] + srcb[2] + 2) >> 2;
          break;
        case 4:
          *bpt += srct[3];
          *bp  += src[3];
          *bpb += srcb[3];
          // fall through
        case 3:
          *bpt += srct[2];
          *bp  += src[2];
          *bpb += srcb[2];
          *bpt += srct[1];
          *bp  += src[1];
          *bpb += srcb[1];
          // fall through
        case 1:
          *bpt += srct[0];
          *bp  += src[0];
          *bpb += srcb[0];
        }
        srct += sx;
        src  += sx;
        srcb += sx;
        bpt++;
        bp++;
        bpb++;
      } while(--i);
      //
      // Now continue with the next line if there is one, count the number of lines summed up.
      if (top->m_pNext)
        top = top->m_pNext;
      if (line->m_pNext) // pixel replication at the boundary.
        line = line->m_pNext;
      if (bot->m_pNext)
        bot = bot->m_pNext;
      lines++;
    }
    //
    // If we're done with this block, or if there are no more lines, normalize this
    // block and continue with the next.
    if (lines >= sy) {
      WORD norm = lines * sx;
      // Only if there is actually anything in the buffer, otherwise just leave it empty.
      assert(lines);
      if (sy == 2) {
        assert(lines == 2);
        norm *= 4;
        for(int i = 0;i < 8;i++)
          buffer[i] = (2 * buffer[i] + topbuffer[i] + bottombuffer[i]) / norm;
      } else {
        if (norm > 1) {
          // Normalize the summed pixels.
          buffer[0] /= norm;buffer[1] /= norm;buffer[2] /= norm;buffer[3] /= norm;
          buffer[4] /= norm;buffer[5] /= norm;buffer[6] /= norm;buffer[7] /= norm;
        }
      }
      // Start the next buffer line.
      buffer    += 8;
      cnt--;
      lines      = 0;
    }
  } while(cnt);
}
///


/// Explicit instaciations
template class InterDownsampler<1,1>;
template class InterDownsampler<1,2>;
template class InterDownsampler<1,3>;
template class InterDownsampler<1,4>;
template class InterDownsampler<2,1>;
template class InterDownsampler<2,2>;
template class InterDownsampler<2,3>;
template class InterDownsampler<2,4>;
template class InterDownsampler<3,1>;
template class InterDownsampler<3,2>;
template class InterDownsampler<3,3>;
template class InterDownsampler<3,4>;
template class InterDownsampler<4,1>;
template class InterDownsampler<4,2>;
template class InterDownsampler<4,3>;
template class InterDownsampler<4,4>;
///
