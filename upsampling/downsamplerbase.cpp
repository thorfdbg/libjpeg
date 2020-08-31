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
** Base class for all upsamplers, common for all upsampling processes
** and independent of the upsampling factors.
**
** $Id: downsamplerbase.cpp,v 1.16 2020/08/31 07:50:44 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "upsampling/downsamplerbase.hpp"
#include "upsampling/downsampler.hpp"
#include "upsampling/interdownsampler.hpp"
#include "std/string.hpp"
///

/// DownsamplerBase::DownsamplerBase
DownsamplerBase::DownsamplerBase(class Environ *env,int sx,int sy,
                                 ULONG width,ULONG height,bool interpolate)
  : JKeeper(env), m_ulWidth(width), m_lTotalLines(height), m_lY(0), m_lHeight(0),
    m_ucSubX(sx), m_ucSubY(sy), m_pInputBuffer(NULL), m_pLastRow(NULL), m_pFree(NULL),
    m_bInterpolate(interpolate)
{
}
///

/// DownsamplerBase::~DownsamplerBase
DownsamplerBase::~DownsamplerBase(void)
{ 
  struct Line *row;

  while((row = m_pInputBuffer)) {
    m_pInputBuffer = row->m_pNext;
    if (row->m_pData)
      m_pEnviron->FreeMem(row->m_pData,(m_ulWidth + 2 + (m_ucSubX << 3)) * sizeof(LONG));
    delete row;
  } 

  while((row = m_pFree)) {
    m_pFree = row->m_pNext;
    m_pEnviron->FreeMem(row->m_pData,(m_ulWidth + 2 + (m_ucSubX << 3)) * sizeof(LONG));
    delete row;
  }
}
///

/// DownsamplerBase::SetBufferedRegion
// Define the region to be buffered, clipping off what has been applied
// here before. This extends the internal buffer to hold at least
// the regions here.
void DownsamplerBase::SetBufferedRegion(const RectAngle<LONG> &region)
{
  //
  // Does just the same right now...
  ExtendBufferedRegion(region);
}
///

/// DownsamplerBase::ExtendBufferedRegion
// Make the buffered region larger to include at least the given rectangle.
// The rectangle is given in image/canvas coordinates.
void DownsamplerBase::ExtendBufferedRegion(const RectAngle<LONG> &region)
{ 
  // Create all lines between the current last line, m_lY+m_lHeight-1 and
  // the last line of the rectangle, region.ra_MaxY
  while(m_lY + m_lHeight < region.ra_MaxY + 1) {
    struct Line *qrow,*alloc = NULL;
    //
    // Get a new pixel row, either from the buffered
    // rows or from the heap.
    if (m_pFree) {
      qrow          = m_pFree;
      m_pFree       = qrow->m_pNext;
      qrow->m_pNext = NULL;
    } else {
      alloc         = new(m_pEnviron) struct Line;
      qrow          = alloc;
    }
    //
    // 
    if (m_pLastRow) {
      m_pLastRow->m_pNext = qrow;
      m_pLastRow = qrow;
    } else {
      assert(m_pInputBuffer == NULL);
      m_pLastRow = m_pInputBuffer = qrow;
    }
    //
    // Allocate the memory for it.
    if (alloc) {
      alloc->m_pData = (LONG *)m_pEnviron->AllocMem((m_ulWidth + 2 + (m_ucSubX << 3)) * sizeof(LONG));
    }
    m_lHeight++;
  }
}
///

/// DownsamplerBase::DefineRegion
// Define the region to contain the given data, copy it to the line buffers
// for later downsampling. Coordinates are in 8x8 blocks.
void DownsamplerBase::DefineRegion(LONG x,LONG y,const LONG *data)
{
  struct Line *line = m_pInputBuffer;
  LONG topy = y << 3;
  LONG yf   = m_lY;
  LONG ofs  = x << 3;
  LONG cnt  = 8;
  LONG ovl  = (m_ucSubX << 3) - ((m_bInterpolate)?0:1); // number of pixels extended to the right.

  assert(topy >= m_lY && topy < m_lY + m_lHeight);

  while(yf < topy) {
    line = line->m_pNext;
    yf++;
  }

  assert(line);
  
  do {
    LONG *dst = line->m_pData + ((m_bInterpolate)?(1):(0));
    UBYTE i;
    // In case of interpolation being turned on, fill the data with the
    // offset +1 to simplify the access to neighbouring samples in the
    // actual downsampler.
    //
    // Overlab at the boundary, extend to the right to keep the downsampling simple
    memcpy(dst + ofs,data,8 * sizeof(LONG));
    // Mirror-extend to the right.
    if (ofs + 8 >= LONG(m_ulWidth)) {
      // Actually, any type of extension is suitable as long as the
      // mean is sensible.
      for(i = 0; i < ovl;i++) {
        dst[m_ulWidth + i] = dst[(m_ulWidth > i)?(m_ulWidth - 1 - i):0];
      }
    }
    // Mirror-extend to the left.
    if (ofs == 0 && m_bInterpolate) {
      dst[-1] = dst[(m_ulWidth > 1)?1:0];
    }
    line  = line->m_pNext;
    data += 8;
  } while(--cnt && line);
}
///

/// DownsamplerBase::RemoveBlocks
// Remove the blocks of the given block line, given in downsampled
// block coordinates.
void DownsamplerBase::RemoveBlocks(ULONG by)
{
  LONG firstkeep = ((by + 1) << 3) * m_ucSubY; // The first line that has to be kept.
  // If interpolation is enabled, one additional line has to be kept.
  if (m_bInterpolate)
    firstkeep--;

  while(m_lY < firstkeep) {
    struct Line *row;
    // The current Y line is no longer required, drop it. If it is there.
    row = m_pInputBuffer;
    if (row) {
      m_pInputBuffer = row->m_pNext;
      if (m_pInputBuffer == NULL) {
        assert(row == m_pLastRow); 
        assert(m_lHeight == 1);
        // it hopefully is as it has no following line
        m_pLastRow = NULL;
      }
      row->m_pNext = m_pFree;
      m_pFree      = row;
      m_lHeight--;
    }
    m_lY++;
  }
}
///

/// DownsamplerBase::GetCollectedBlocks
// Return a rectangle of block coordinates in the downsampled domain
// that is ready for output.
void DownsamplerBase::GetCollectedBlocks(RectAngle<LONG> &rect) const
{
  LONG ymin = m_lY;
  LONG ymax = m_lY;
  //
  // One less line 
  if (m_bInterpolate) {
    ymin++;
    ymax--;
  }
  //
  // Everything in horizontal direction.
  rect.ra_MinX = 0;
  rect.ra_MaxX = (((m_ulWidth + m_ucSubX - 1) / m_ucSubX + 7) >> 3) - 1;
  // In vertical direction, start at the upper edge of the first buffered line,
  // but use the first complete block.
  rect.ra_MinY = ((ymin / m_ucSubY) + 7) >> 3;
  // Find the first block that is not buffered, remove that block.
  // If we are at the end of the image, just return the last block and
  // complete even if not all lines are ready.
  if (m_lY + m_lHeight >= m_lTotalLines) {
    rect.ra_MaxY = (((m_lTotalLines + m_ucSubY - 1) / m_ucSubY + 7) >> 3) - 1;
  } else {
    rect.ra_MaxY = (((ymax + m_lHeight) / m_ucSubY) >> 3) - 1;
  }
}
///

/// DownsamplerBase::CreateDownsampler
// Create an upsampler for the given upsampling factors. Currently, only
// factors from 1x1 to 4x4 are supported.
class DownsamplerBase *DownsamplerBase::CreateDownsampler(class Environ *env,int sx,int sy,
                                                          ULONG width,ULONG height,bool interpolate)
{
  if (interpolate) {
    switch(sy) {
    case 1:
      switch(sx) {
      case 1:
        return new(env) InterDownsampler<1,1>(env,width,height);
        break;
      case 2:
        return new(env) InterDownsampler<2,1>(env,width,height);
        break;
      case 3:
        return new(env) InterDownsampler<3,1>(env,width,height);
        break;
      case 4:
        return new(env) InterDownsampler<4,1>(env,width,height);
        break;
      }
      break;
    case 2:
      switch(sx) {
      case 1:
        return new(env) InterDownsampler<1,2>(env,width,height);
        break;
      case 2:
        return new(env) InterDownsampler<2,2>(env,width,height);
        break;
      case 3:
        return new(env) InterDownsampler<3,2>(env,width,height);
        break;
      case 4:
        return new(env) InterDownsampler<4,2>(env,width,height);
        break;
      }
      break;
    case 3:
      switch(sx) {
      case 1:
        return new(env) InterDownsampler<1,3>(env,width,height);
        break;
      case 2:
        return new(env) InterDownsampler<2,3>(env,width,height);
        break;
      case 3:
        return new(env) InterDownsampler<3,3>(env,width,height);
        break;
      case 4:
        return new(env) InterDownsampler<4,3>(env,width,height);
        break;
      }
      break;
    case 4:
      switch(sx) {
      case 1:
        return new(env) InterDownsampler<1,4>(env,width,height);
        break;
      case 2:
        return new(env) InterDownsampler<2,4>(env,width,height);
        break;
      case 3:
        return new(env) InterDownsampler<3,4>(env,width,height);
        break;
      case 4:
        return new(env) InterDownsampler<4,4>(env,width,height);
        break;
      }
      break;
    }
  } else {
    switch(sy) {
    case 1:
      switch(sx) {
      case 1:
        return new(env) Downsampler<1,1>(env,width,height);
        break;
      case 2:
        return new(env) Downsampler<2,1>(env,width,height);
        break;
      case 3:
        return new(env) Downsampler<3,1>(env,width,height);
        break;
      case 4:
        return new(env) Downsampler<4,1>(env,width,height);
        break;
      }
      break;
    case 2:
      switch(sx) {
      case 1:
        return new(env) Downsampler<1,2>(env,width,height);
        break;
      case 2:
        return new(env) Downsampler<2,2>(env,width,height);
        break;
      case 3:
        return new(env) Downsampler<3,2>(env,width,height);
        break;
      case 4:
        return new(env) Downsampler<4,2>(env,width,height);
        break;
      }
      break;
    case 3:
      switch(sx) {
      case 1:
        return new(env) Downsampler<1,3>(env,width,height);
        break;
      case 2:
        return new(env) Downsampler<2,3>(env,width,height);
        break;
      case 3:
        return new(env) Downsampler<3,3>(env,width,height);
        break;
      case 4:
        return new(env) Downsampler<4,3>(env,width,height);
        break;
      }
      break;
    case 4:
      switch(sx) {
      case 1:
        return new(env) Downsampler<1,4>(env,width,height);
        break;
      case 2:
        return new(env) Downsampler<2,4>(env,width,height);
        break;
      case 3:
        return new(env) Downsampler<3,4>(env,width,height);
        break;
      case 4:
        return new(env) Downsampler<4,4>(env,width,height);
        break;
      }
      break;
    }
  }

  {
    class Environ *m_pEnviron = env;
    JPG_THROW(NOT_IMPLEMENTED,"DownsamplerBase::CreateUpsampler",
              "subsampling factors larger than 4x4 are not supported, sorry");
  }
  
  return NULL;
}
///


