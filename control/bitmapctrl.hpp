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
** Basic control helper for requesting and releasing bitmap data.
**
** $Id: bitmapctrl.hpp,v 1.26 2017/11/28 13:08:07 thor Exp $
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
  // The HDR bitmap or just the user bitmap for legacy JPEG
  struct ImageBitMap   **m_ppBitmap; 
  //
  // The LDR tonemapped image if the user provides one.
  // If not, this remains NULL.
  struct ImageBitMap   **m_ppLDRBitmap;
  //
  // Buffers for the color transformation.
  LONG                 **m_ppCTemp;
  LONG                  *m_pColorBuffer;
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
  void RequestUserData(class BitMapHook *bmh,const RectAngle<LONG> &region,UBYTE comp,bool alpha);
  //
  // Release the user data again through the bitmap hook.
  void ReleaseUserData(class BitMapHook *bmh,const RectAngle<LONG> &region,UBYTE comp,bool alpha);
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
  // Ensure that unused bitmaps are cleared so we do overwrite memory that is
  // not requested.
  void ResetBitmaps(void);
  //
  // Extract the region of the bitmap covering the indicated rectangle
  void ExtractBitmap(struct ImageBitMap *ibm,const RectAngle<LONG> &rect,UBYTE i);
  //
  // Ditto for the LDR bitmap.
  void ExtractLDRBitmap(struct ImageBitMap *ibm,const RectAngle<LONG> &rect,UBYTE i);
  //
  // Check whether we have a dedicated LDR image or whether we must tonemap ourselves.
  bool hasLDRImage(void) const
  {
    return m_ppLDRBitmap?true:false;
  }
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
  // First step of a region encoder: Find the region that can be pulled in the next step,
  // from a rectangle request. This potentially shrinks the rectangle, which should be
  // initialized to the full image.
  virtual void CropEncodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *rr) = 0;
  //
  // First step of a region decoder: Find the region that can be provided in the next step.
  // The region should be initialized to the region from the rectangle request before
  // calling here.
  void CropDecodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *rr);
  //
  // Request user data for encoding for the given region, potentially clip the region to the
  // data available from the user.
  virtual void RequestUserDataForEncoding(class BitMapHook *bmh,RectAngle<LONG> &region,bool alpha) = 0;
  //
  // Pull data buffers from the user data bitmap hook
  virtual void RequestUserDataForDecoding(class BitMapHook *bmh,RectAngle<LONG> &region,
                                          const struct RectangleRequest *rr,bool alpha) = 0;
  //
  // Release user data after encoding.
  void ReleaseUserDataFromEncoding(class BitMapHook *bmh,const RectAngle<LONG> &region,bool alpha);
  //
  // Release user data after decoding.
  void ReleaseUserDataFromDecoding(class BitMapHook *bmh,const struct RectangleRequest *rr,bool alpha);
  //
  // Encode a region, push it into the internal buffers and
  // prepare everything for coding.
  virtual void EncodeRegion(const RectAngle<LONG> &region) = 0;
  //
  // Reconstruct a block, or part of a block
  virtual void ReconstructRegion(const RectAngle<LONG> &region,const struct RectangleRequest *rr) = 0;
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
  //
  // Compute the subsampled rectangle in case we are not upsampling from the
  // region in the rectangle request.
  void SubsampledRegion(RectAngle<LONG> &rect,const struct RectangleRequest *rr) const;
  //
};
///

///
#endif

