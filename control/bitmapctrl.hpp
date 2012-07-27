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
** Basic control helper for requesting and releasing bitmap data.
**
** $Id: bitmapctrl.hpp,v 1.15 2012-06-11 10:19:15 thor Exp $
**
*/

#ifndef CONTROL_BITMAPCTRL_HPP
#define CONTROL_BITMAPCTRL_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "control/bufferctrl.hpp"
///

/// Forwards
class Frame;
class BitMapHook;
class Component;
class ResidualBlockHelper;
///

/// Class BitmapCtrl
// Basic control helper for requesting and releasing bitmap data.
class BitmapCtrl : public BufferCtrl {
  //
protected:
  //
  // The frame to which this belongs.
  class Frame           *m_pFrame;
  //
  struct ImageBitMap   **m_ppBitmap;
  //
  // Dimensions in pixels.
  ULONG                  m_ulPixelWidth;
  ULONG                  m_ulPixelHeight;
  //
  // The buffered pixel type of the last request.
  UBYTE                  m_ucPixelType; 
  //
  // Number of components in count.
  UBYTE                  m_ucCount;
  //
  // One point pixel transform for higher bitdepth coding, to
  // be applied before color transformation. This creates
  // residual data potentially coded by the residual scan.
  // Or it creates loss.
  UBYTE                  m_ucPointShift;
  //
  BitmapCtrl(class Frame *frame);
  //
  // Find the components and build all the arrays
  // This is a post-initalization call that does not
  // happen in the constructor.
  void BuildCommon();
  //
  // Request data from the user through the indicated bitmap hook
  // for the given rectangle. The rectangle is first clipped to
  // range (as appropriate, if the height is already known) and
  // then the desired n'th component of the scan (not the component
  // index) is requested.
  void RequestUserData(class BitMapHook *bmh,const RectAngle<LONG> &region,UBYTE comp);
  //
  // Release the user data again through the bitmap hook.
  void ReleaseUserData(class BitMapHook *bmh,const RectAngle<LONG> &region,UBYTE comp);
  // 
  // Clip a rectangle to the image region
  void ClipToImage(RectAngle<LONG> &rect) const;
  //
  // Return the i'th image bitmap.
  const struct ImageBitMap &BitmapOf(UBYTE i) const
  {
    assert(i < m_ucCount);

    return *m_ppBitmap[i];
  }
  //
  // Extract the region of the bitmap covering the indicated rectangle
  void ExtractBitmap(struct ImageBitMap *ibm,const RectAngle<LONG> &rect,UBYTE i);
  //
public:
  // 
  //
  virtual ~BitmapCtrl(void);
  //
  // Return the pixel type of the data buffered here.
  UBYTE PixelTypeOf(void) const
  {
    return m_ucPixelType;
  }
  //
  // Encode a region, push it into the internal buffers and
  // prepare everything for coding.
  virtual void EncodeRegion(class BitMapHook *bmh,const struct RectangleRequest *rr) = 0;
  //
  // Reconstruct a block, or part of a block
  virtual void ReconstructRegion(class BitMapHook *bmh,const struct RectangleRequest *rr) = 0;
  //
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(const struct RectangleRequest *rr) const = 0;
  //
  // Return true if the next MCU line is buffered and can be pushed
  // to the encoder.
  virtual bool isNextMCULineReady(void) const = 0;
  //
  // Reset all components on the image side of the control to the
  // start of the image. Required when re-requesting the image
  // for encoding or decoding.
  virtual void ResetToStartOfImage(void) = 0;
  //
  // Return an indicator whether all of the image has been loaded into
  // the image buffer.
  virtual bool isImageComplete(void) const = 0;
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    m_ulPixelHeight = lines;
  }
};
///

///
#endif

