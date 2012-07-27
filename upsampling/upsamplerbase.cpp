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
** Base class for all upsamplers, common for all upsampling processes
** and independent of the upsampling factors.
**
** $Id: upsamplerbase.cpp,v 1.7 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "upsampling/upsamplerbase.hpp"
#include "coding/quantizedrow.hpp"
#include "upsampling/upsampler.hpp"
#include "std/string.hpp"
///

/// UpsamplerBase::UpsamplerBase
UpsamplerBase::UpsamplerBase(class Environ *env,int sx,int sy,ULONG pixelwidth,ULONG pixelheight)
  : JKeeper(env), m_lY(0), m_lHeight(0), m_ucSubX(sx), m_ucSubY(sy),
    m_pInputBuffer(NULL), m_pLastRow(NULL), m_pFree(NULL)
{
  // Compute the width in input buffer (subsampled) pixels.
  m_ulWidth     = (pixelwidth  + sx - 1) / sx;
  m_lTotalLines = (pixelheight + sy - 1) / sy; 
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

/// UpsamplerBase::SetBufferedRegion
// Define the region to be buffered, clipping off what has been buffered
// here before. Modifies the rectangle to contain only what is needed
// in addition to what is here already. The rectangle is in block indices.
void UpsamplerBase::SetBufferedRegion(RectAngle<LONG> &region)
{
  struct Line *row;
  LONG maxy;

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
class UpsamplerBase *UpsamplerBase::CreateUpsampler(class Environ *env,int sx,int sy,ULONG width,ULONG height)
{
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

  return NULL;
}
///


