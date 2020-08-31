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
** $Id: downsamplerbase.hpp,v 1.14 2018/07/27 06:56:43 thor Exp $
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
  // A flag indicating whether this downsampler interpolates and hence
  // requires additional lines for buffering.
  bool                m_bInterpolate;
  //
protected:
  //
  // Only subclasses can create this.
  DownsamplerBase(class Environ *env,int sx,int sy,
                  ULONG width,ULONG height,bool interpolate);
  //
public:
  //
  virtual ~DownsamplerBase(void);
  //
  // The actual downsampling process. To be implemented by the actual
  // classes inheriting from this. Coordinates are in the downsampled
  // block domain the block indices. Requires an output buffer that
  // will keep the downsampled data.
  virtual void DownsampleRegion(LONG bx,LONG by,LONG *buffer) const = 0;
  //
  // Define the region to be buffered, clipping off what has been applied
  // here before. This extends the internal buffer to hold at least
  // the regions here. The region is here given in coordinates in
  // the image (non-subsampled) domain.
  void SetBufferedRegion(const RectAngle<LONG> &region);
  //
  // Make the buffered region larger to include at least the given rectangle
  // of image coordinates. This is in canvas/coordinates, not in block indices.
  void ExtendBufferedRegion(const RectAngle<LONG> &region);
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
  void GetCollectedBlocks(RectAngle<LONG> &rect) const;
  //
  // Create an upsampler for the given upsampling factors. Currently, only
  // factors from 1x1 to 4x4 are supported.
  // If "interpolate" is set, a smarter interpolation filter (instead of the box-filter)
  // is used. This filter has, however, a larger delay.
  static class DownsamplerBase *CreateDownsampler(class Environ *env,int sx,int sy,
                                                  ULONG width,ULONG height,bool interpolate);
  //
};
///

///
#endif

