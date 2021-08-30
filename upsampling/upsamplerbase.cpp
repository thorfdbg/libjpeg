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
** $Id: upsamplerbase.cpp,v 1.22 2020/08/31 07:50:44 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "upsampling/upsamplerbase.hpp"
#include "coding/quantizedrow.hpp"
#include "upsampling/upsampler.hpp"
#include "upsampling/cositedupsampler.hpp"
#include "std/string.hpp"
///

/// UpsamplerBase::UpsamplerBase
UpsamplerBase::UpsamplerBase(class Environ *env,int sx,int sy,ULONG pixelwidth,ULONG pixelheight)
  : JKeeper(env), m_lY(0), m_lHeight(0), m_ucSubX(sx), m_ucSubY(sy),
    m_pInputBuffer(NULL), m_pLastRow(NULL), m_pFree(NULL)
{
  // Check whether a DNL marker is present.
  if (pixelheight == 0)
    pixelheight   = ~0U >> 1;
  // Compute the width in input buffer (subsampled) pixels.
  m_ulWidth       = (pixelwidth  + sx - 1) / sx;
  m_lTotalLines   = (pixelheight + sy - 1) / sy; 
  //
  // Also store the dimension in pixels.
  m_ulPixelWidth  = pixelwidth;
  m_ulPixelHeight = pixelheight;

}
///

/// UpsamplerBase::~UpsamplerBase
UpsamplerBase::~UpsamplerBase(void)
{
  struct Line *row;

  while((row = m_pInputBuffer)) {
    m_pInputBuffer = row->m_pNext;
    if (row->m_pData)
      m_pEnviron->FreeMem(row->m_pData,(m_ulWidth + 2 + 8) * sizeof(LONG));
    delete row;
  } 

  while((row = m_pFree)) {
    m_pFree = row->m_pNext;
    m_pEnviron->FreeMem(row->m_pData,(m_ulWidth + 2 + 8) * sizeof(LONG));
    delete row;
  }
}
///

/// UpsamplerBase::GetCollectedBlocks
// Return a rectangle of block coordinates in the image domain
// that is ready for output.
void UpsamplerBase::GetCollectedBlocks(RectAngle<LONG> &rect) const
{
  rect.ra_MinX = 0;
  rect.ra_MaxX = (m_ulWidth * m_ucSubX - 1) >> 3;
  //
  // We need one line above the first line we can upsample
  // except for the top of the image.
  if (m_ucSubY > 1) {
    if (m_lY) {
      rect.ra_MinY = ((m_lY + 1) * m_ucSubY)  >> 3;
    } else {
      rect.ra_MinY = 0;
    }
  } else {
    rect.ra_MinY = m_lY >> 3;
  }
  //
  // All of the image?
  if (m_lY + m_lHeight >= m_lTotalLines) {
    // Yes, return all of it.
    rect.ra_MaxY = (m_lTotalLines * m_ucSubY - 1) >> 3;
  } else {
    // m_lY + m_lHeight - 1 is the last line buffered, hence
    // (m_lY + m_lHeight - 1) * m_ucSubY is the last line we can reconstruct
    // since the next output line would require the following line.
    // this plus one is the *first* line we cannot compute.
    // Of this line, we compute the block, then subtract one
    // to get the last block we surely have.
    rect.ra_MaxY = (((m_lY + m_lHeight - 1) * m_ucSubY + 1) >> 3) - 1;
  }
}
///

/// UpsamplerBase::SetBufferedImageRegion
// Set the buffered region given in image regions, not in
// block regions. Returns the updated rectangle in blocks
void UpsamplerBase::SetBufferedImageRegion(RectAngle<LONG> &region)
{
  LONG bwidth           = ((m_ulPixelWidth  + m_ucSubX - 1) / m_ucSubX + 7) >> 3;
  LONG bheight          = ((m_ulPixelHeight + m_ucSubY - 1) / m_ucSubY + 7) >> 3;
  LONG rx               = (m_ucSubX > 1)?(1):(0);
  LONG ry               = (m_ucSubY > 1)?(1):(0);
  // The +/-1 include additional lines required for subsampling expansion
  region.ra_MinX        = ((region.ra_MinX / m_ucSubX - rx) >> 3);
  region.ra_MaxX        = ((region.ra_MaxX / m_ucSubX + rx) >> 3);
  region.ra_MinY        = ((region.ra_MinY / m_ucSubY - ry) >> 3);
  region.ra_MaxY        = ((region.ra_MaxY / m_ucSubY + ry) >> 3);
  // Clip.
  if (region.ra_MinX < 0)        region.ra_MinX = 0;
  if (region.ra_MaxX >= bwidth)  region.ra_MaxX = bwidth - 1;
  if (region.ra_MinY < 0)        region.ra_MinY = 0;
  if (region.ra_MaxY >= bheight) region.ra_MaxY = bheight - 1;
  //
  SetBufferedRegion(region); // also removes the rectangle of blocks already buffered.
}
///

/// UpsamplerBase::SetBufferedRegion
// Define the region to be buffered, clipping off what has been buffered
// here before. Modifies the rectangle to contain only what is needed
// in addition to what is here already. The rectangle is in block indices.
void UpsamplerBase::SetBufferedRegion(RectAngle<LONG> &region)
{
  struct Line *row;
  // First check whether we can clip off anything at the top.
  // Drop off anything above the region.
  while(m_lY < (region.ra_MinY << 3)) {
    // The current Y line is no longer required, drop it.
    // If it is there.
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
  //
  // Here the first row is now either the first row of the region, or it is NULL
  // if it is not yet present. We also have that m_lY >= region.ra_MinY, i.e.
  // the buffered region starts 
  assert(m_lY >= (region.ra_MinY << 3));
  //
  // If part of the buffered region is below our top line, just dispose the buffer.
  if (m_lY > (region.ra_MinY << 3)) {
    if (m_pInputBuffer) {
      assert(m_pLastRow);
      m_pInputBuffer->m_pNext = m_pFree;
      m_pFree                 = m_pInputBuffer;
      m_lHeight               = 0;
      m_pInputBuffer          = NULL;
      m_pLastRow              = NULL;
    }
    m_lY = region.ra_MinY << 3;
  }
  // 
  assert(m_lY == (region.ra_MinY << 3));
  //
  // Now remove all the lines from the region that are in the buffer.
  region.ra_MinY = (m_lY + m_lHeight + 7) >> 3;
  //
  // The region to request is now non-empty.
  ExtendBufferedRegion(region);
}
///

/// UpsamplerBase::ExtendBufferedRegion
// Make the buffered region larger to include at least the given rectangle.
// The rectangle is given in block indices, not canvas coordinates.
void UpsamplerBase::ExtendBufferedRegion(const RectAngle<LONG> &region)
{ 
  LONG maxy;
  // All lines from minY to maxY now need to be allocated as the caller
  // need to fill them in. This includes that if
  // region.ra_MinY > region.ra_MaxY. This is because 
  // region.ra_MinY == m_lY + m_lHeight by the above
  // assignment.
  maxy = (1 + region.ra_MaxY) << 3; // +1: conversion to exclusive.
  if (maxy > m_lTotalLines)
    maxy = m_lTotalLines;
  //
  while(m_lY + m_lHeight < maxy) {
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
      alloc->m_pData = (LONG *)m_pEnviron->AllocMem((m_ulWidth + 2 + 8) * sizeof(LONG));
    }
    m_lHeight++;
  }
}
///

/// UpsamplerBase::RemoveBlocks
// Remove the blocks of the given block line, given in image 
// block coordinates.
void UpsamplerBase::RemoveBlocks(ULONG by)
{ 
  LONG firstkeep = (by + 1) << 3; // The first line that has to be kept.
  //

  if (m_ucSubY > 1) {
    // However, lines are here in subsampled coordinates, and one
    // extra line is needed.
    firstkeep = (firstkeep / m_ucSubY) - 1;
  }

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

/// UpsamplerBase::DefineRegion
// Define the region to contain the given data, copy it to the line buffers
// for later upsampling. Coordinates are in blocks.
void UpsamplerBase::DefineRegion(LONG bx,LONG by,const LONG *data)
{
  struct Line *line = m_pInputBuffer;
  LONG cnt = 8,y = m_lY;

  bx <<= 3;
  by <<= 3; // to pixel coordinates.

  assert(by >= m_lY && by < m_lY + m_lHeight);

  // Find the target line.
  while(y < by) {
    line = line->m_pNext;
    y++;
  }
  assert(line);
  
  do {
    LONG *dest = line->m_pData + 1;
    memcpy(dest + bx,data,8 * sizeof(LONG));
    // Just copy the edges. Simpler just to do than to 
    // make a test whether it is necessary.
    dest[-1]        = dest[0];
    dest[m_ulWidth] = dest[m_ulWidth - 1];
    line            = line->m_pNext;
    data           += 8;
  } while(--cnt && line);
}
///

/// UpsamplerBase::CreateUpsampler
// Create an upsampler for the given upsampling factors. Currently, only
// factors from 1x1 to 4x4 are supported.
class UpsamplerBase *UpsamplerBase::CreateUpsampler(class Environ *env,int sx,int sy,
                                                    ULONG width,ULONG height,bool centered)
{

  if (centered) {
    switch(sy) {
    case 1:
      switch(sx) {
      case 1:
        return new(env) Upsampler<1,1>(env,width,height);
        break;
      case 2:
        return new(env) Upsampler<2,1>(env,width,height);
        break;
      case 3:
        return new(env) Upsampler<3,1>(env,width,height);
        break;
      case 4:
        return new(env) Upsampler<4,1>(env,width,height);
        break;
      }
      break;
    case 2:
      switch(sx) {
      case 1:
        return new(env) Upsampler<1,2>(env,width,height);
        break;
      case 2:
        return new(env) Upsampler<2,2>(env,width,height);
        break;
      case 3:
        return new(env) Upsampler<3,2>(env,width,height);
        break;
      case 4:
        return new(env) Upsampler<4,2>(env,width,height);
        break;
      }
      break;
    case 3:
      switch(sx) {
      case 1:
        return new(env) Upsampler<1,3>(env,width,height);
        break;
      case 2:
        return new(env) Upsampler<2,3>(env,width,height);
        break;
      case 3:
        return new(env) Upsampler<3,3>(env,width,height);
        break;
      case 4:
        return new(env) Upsampler<4,3>(env,width,height);
        break;
      }
      break;
    case 4:
      switch(sx) {
      case 1:
        return new(env) Upsampler<1,4>(env,width,height);
        break;
      case 2:
        return new(env) Upsampler<2,4>(env,width,height);
        break;
      case 3:
        return new(env) Upsampler<3,4>(env,width,height);
        break;
      case 4:
        return new(env) Upsampler<4,4>(env,width,height);
        break;
      }
      break;
    }
  } else {
    switch(sy) {
    case 1:
      switch(sx) {
      case 1:
        return new(env) CositedUpsampler<1,1>(env,width,height);
        break;
      case 2:
        return new(env) CositedUpsampler<2,1>(env,width,height);
        break;
      case 3:
        return new(env) CositedUpsampler<3,1>(env,width,height);
        break;
      case 4:
        return new(env) CositedUpsampler<4,1>(env,width,height);
        break;
      }
      break;
    case 2:
      switch(sx) {
      case 1:
        return new(env) CositedUpsampler<1,2>(env,width,height);
        break;
      case 2:
        return new(env) CositedUpsampler<2,2>(env,width,height);
        break;
      case 3:
        return new(env) CositedUpsampler<3,2>(env,width,height);
        break;
      case 4:
        return new(env) CositedUpsampler<4,2>(env,width,height);
        break;
      }
      break;
    case 3:
      switch(sx) {
      case 1:
        return new(env) CositedUpsampler<1,3>(env,width,height);
        break;
      case 2:
        return new(env) CositedUpsampler<2,3>(env,width,height);
        break;
      case 3:
        return new(env) CositedUpsampler<3,3>(env,width,height);
        break;
      case 4:
        return new(env) CositedUpsampler<4,3>(env,width,height);
        break;
      }
      break;
    case 4:
      switch(sx) {
      case 1:
        return new(env) CositedUpsampler<1,4>(env,width,height);
        break;
      case 2:
        return new(env) CositedUpsampler<2,4>(env,width,height);
        break;
      case 3:
        return new(env) CositedUpsampler<3,4>(env,width,height);
        break;
      case 4:
        return new(env) CositedUpsampler<4,4>(env,width,height);
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


