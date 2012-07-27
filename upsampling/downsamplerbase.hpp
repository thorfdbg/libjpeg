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
** $Id: downsamplerbase.hpp,v 1.3 2012-06-02 10:27:14 thor Exp $
**
*/

#ifndef UPSAMPLING_DOWNSAMPLERBASE_HPP
#define UPSAMPLING_DOWNSAMPLERBASE_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/line.hpp"
///

/// Class DownsamplerBase
// This class provides the infrastructure for the downsampling process
// common to all implementations regardless of the downsampling factors.
class DownsamplerBase : public JKeeper {
  //
protected:
  //
  // The width of the upsampled data in image pixels.
  ULONG               m_ulWidth;
  //
  // The height of the upsampled data in image lines.
  LONG                m_lTotalLines;
  //
  // The top Y line of the buffer we hold.
  LONG                m_lY;
  //
  // The number of lines buffered here.
  LONG                m_lHeight;
  //
  // Subsampling factors.
  UBYTE               m_ucSubX;
  //
  UBYTE               m_ucSubY;
  //
  // The reconstructed data itself. This is the input buffer
  // of the upsampling filter.
  struct Line        *m_pInputBuffer;
  //
private:
  //
  // The last row of the buffer.
  struct Line        *m_pLastRow;
  //
  // Lines currently not in use.
  struct Line        *m_pFree;
  //
public:
  DownsamplerBase(class Environ *env,int sx,int sy,ULONG width,ULONG height);
  //
  virtual ~DownsamplerBase(void);
  //
  // The actual downsampling process. To be implemented by the actual
  // classes inheriting from this. Coordinates are in the downsampled
  // block domain the block indices. Requires an output buffer that
  // will keep the downsampled data.
  virtual void DownsampleRegion(LONG bx,LONG by,LONG *buffer) = 0;
  //
  // Define the region to be buffered, clipping off what has been applied
  // here before. This extends the internal buffer to hold at least
  // the regions here.
  void SetBufferedRegion(const RectAngle<LONG> &region);
  //
  // Define the region to contain the given data, copy it to the line buffers
  // for later downsampling. Coordinates are in 8x8 blocks.
  void DefineRegion(LONG x,LONG y,const LONG *data);
  //
  // Remove the blocks of the given block line, given in downsampled
  // block coordinates.
  void RemoveBlocks(ULONG by);
  //
  // Return a rectangle of block coordinates in the downsampled domain
  // that is ready for output.
  void GetCollectedBlocks(RectAngle<LONG> &rect);
  //
  // Create an upsampler for the given upsampling factors. Currently, only
  // factors from 1x1 to 4x4 are supported.
  static class DownsamplerBase *CreateDownsampler(class Environ *env,int sx,int sy,ULONG width,ULONG height);
  //
};
///

///
#endif

