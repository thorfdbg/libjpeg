/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** $Id: upsamplerbase.hpp,v 1.16 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef UPSAMPLING_UPSAMPLERBASE_HPP
#define UPSAMPLING_UPSAMPLERBASE_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/line.hpp"
///

/// Class UpsamplerBase
// This class provides the infrastructure for the upsampling process
// common to all implementations regardless of the upsampling factors.
class UpsamplerBase : public JKeeper {
  //
protected:
  //
  // The width of the upsampled data in subsampled pixels.
  ULONG               m_ulWidth;
  //
  // The height of the upsampled data in subsampled lines.
  LONG                m_lTotalLines;
  //
  // The top Y line of the buffer we hold.
  LONG                m_lY;
  //
  // The number of lines buffered here.
  LONG                m_lHeight;
  //
  // Dimensions in pixels.
  ULONG               m_ulPixelWidth;
  ULONG               m_ulPixelHeight;
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
  // The actual implementations: Filter horizontally from the line into the 8x8 buffer
  template<int>
  static void HorizontalFilterCore(int xmod,LONG *target);
  //
  template<int>
  static void VerticalFilterCore(int ymod,struct Line *top,struct Line *cur,struct Line *bot,
                                 LONG offset,LONG *target);
  //
private:
  //
  // The last row of the buffer.
  struct Line *m_pLastRow;
  //
  // Lines currently not in use.
  struct Line *m_pFree;
  //
public:
  UpsamplerBase(class Environ *env,int sx,int sy,ULONG width,ULONG height);
  //
  virtual ~UpsamplerBase(void);
  //
  // The actual upsampling process. To be implemented by the actual
  // classes inheriting from this.
  virtual void UpsampleRegion(const RectAngle<LONG> &r,LONG *buffer) const = 0;
  //
  // Define the region to be buffered, clipping off what has been buffered
  // here before. Modifies the rectangle to contain only what is needed
  // in addition to what is here already. The rectangle is in block indices.
  void SetBufferedRegion(RectAngle<LONG> &region);
  //
  // Set the buffered region given in image regions, not in
  // block regions. Returns the updated rectangle in blocks
  void SetBufferedImageRegion(RectAngle<LONG> &region);
  //
  // Make the buffered region larger to include at least the given rectangle.
  // The rectangle is given in block indices, not in canvas coordinates.
  void ExtendBufferedRegion(const RectAngle<LONG> &region);
  //
  // Define the region to contain the given data, copy it to the line buffers
  // for later upsampling. Coordinates are in blocks.
  void DefineRegion(LONG bx,LONG by,const LONG *data);
  //
  // Remove the blocks of the given block line, given in image
  // block coordinates.
  void RemoveBlocks(ULONG by);
  //
  // Return a rectangle of block coordinates in the image domain
  // that is ready for output.
  void GetCollectedBlocks(RectAngle<LONG> &rect) const;
  //
  // Create an upsampler for the given upsampling factors. Currently, only
  // factors from 1x1 to 4x4 are supported.
  static class UpsamplerBase *CreateUpsampler(class Environ *env,int sx,int sy,ULONG width,ULONG height);
};
///

///
#endif

