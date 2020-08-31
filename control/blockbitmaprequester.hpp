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
** This class pulls blocks from the frame and reconstructs from those
** quantized block lines or encodes from them.
**
** $Id: blockbitmaprequester.hpp,v 1.30 2017/11/28 13:08:07 thor Exp $
**
*/

#ifndef CONTROL_BLOCKBITMAPREQUESTER_HPP
#define CONTROL_BLOCKBITMAPREQUESTER_HPP

/// Includes
#include "control/bitmapctrl.hpp"
#include "control/blockbuffer.hpp"
///

/// Forwards
class DCT;
class UpsamplerBase;
class DownsamplerBase;
class ColorTrafo;
class QuantizedRow;
class ResidualBlockHelper;
class DeRinger;
///

/// class BlockBitmapRequester
// This class pulls blocks from the frame and reconstructs from those
// quantized block lines or encodes from them.
class BlockBitmapRequester : public BlockBuffer, public BitmapCtrl {
  //
  class Environ             *m_pEnviron;
  class Frame               *m_pFrame;
  //
  // Dimensions of the frame.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Number of components in the frame.
  UBYTE                      m_ucCount;
  //
  // Number of lines already in the input buffer on encoding.
  ULONG                     *m_pulReadyLines;
  //
  // Temporary for decoding how many MCUs are ready on the next
  // iteration.can be pulled next.
  ULONG                      m_ulMaxMCU;
  // 
  // Downsampling operator.
  class DownsamplerBase    **m_ppDownsampler;
  //
  // The downsampler for the residual image.
  class DownsamplerBase    **m_ppResidualDownsampler;
  //
  // And the inverse, if required.
  class UpsamplerBase      **m_ppUpsampler;
  //
  // The upsampler for the residual image.
  class UpsamplerBase      **m_ppResidualUpsampler;
  //
  // The original image buffered in a dummy 1,1 downsampler
  class DownsamplerBase    **m_ppOriginalImage;
  //
  // Temporary bitmaps
  struct ImageBitMap       **m_ppTempIBM;
  //
  struct ImageBitMap       **m_ppOriginalIBM;
  //
  // Temporary data pointers for the residual computation.
  LONG                     **m_ppQTemp;
  LONG                     **m_ppRTemp;
  //
  // Temporary output buffer for the residual
  LONG                     **m_ppDTemp;
  //
  // The output color buffer.
  LONG                      *m_plResidualColorBuffer;
  //
  // The buffer for the original data.
  LONG                      *m_plOriginalColorBuffer;
  //
  // Current position in reconstruction or encoding,
  // going through the color transformation.
  // On decoding, the line in here has the Y-coordinate 
  // in m_ulReadyLines.
  class QuantizedRow      ***m_pppQImage;
  //
  // Current position for the residual image.
  class QuantizedRow      ***m_pppRImage;
  //
  // A helper class that encodes the residual.
  class ResidualBlockHelper *m_pResidualHelper;
  //
  // Deblocking filter (if any)
  class DeRinger           **m_ppDeRinger;
  //
  // True if subsampling is required.
  bool                       m_bSubsampling;
  //
  // True if this is an openloop encoder, i.e. we do not
  // use the reconstructed DCT samples.
  bool                       m_bOpenLoop;
  //
  // If this is true, the post-DCT R/D optimizer is on.
  bool                       m_bOptimize;
  //
  // If this is true, run the deblocking filter as well.
  bool                       m_bDeRing;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
  // Create the next row of the image such that m_pppImage[i] is valid.
  class QuantizedRow *BuildImageRow(class QuantizedRow **qrow,class Frame *frame,int i);
  //
  // Forward the state machine for the quantized rows by one image-8-block line
  void AdvanceQRows(void);
  //
  // Compute the residual data and move that into the R-output buffers.
  void AdvanceRRows(const RectAngle<LONG> &region,class ColorTrafo *ctrafo);
  //
  // Get the source data from the source image(s)
  // and place them into the downsampler and the original
  // image buffer.
  void PullSourceData(const RectAngle<LONG> &region,class ColorTrafo *ctrafo);
  //
  // The encoding procedure without subsampling, which is the much simpler case.
  void EncodeUnsampled(const RectAngle<LONG> &region,class ColorTrafo *ctrafo);
  //
  // Reconstruct a region not using any subsampling.
  void ReconstructUnsampled(const struct RectangleRequest *rr,const RectAngle<LONG> &region,
                            ULONG maxmcu,class ColorTrafo *ctrafo);
  //
  // Pull the quantized data into the upsampler if there is one.
  void PullQData(const struct RectangleRequest *rr,const RectAngle<LONG> &region);
  //
  // Get the residual data and potentially move it into the
  // residual upsampler
  void PullRData(const struct RectangleRequest *rr,const RectAngle<LONG> &region);
  //
  // Generate the final output of the reconstructed data.
  void PushReconstructedData(const struct RectangleRequest *rr,const RectAngle<LONG> &region,
                             ULONG maxmcu,class ColorTrafo *ctrafo);
  //
public:
  //
  BlockBitmapRequester(class Frame *frame);
  //
  virtual ~BlockBitmapRequester(void); 
  //
  class Environ *EnvironOf(void) const
  {
    return m_pEnviron;
  }
  //
  // First time usage: Collect all the information for encoding.
  // May throw on out of memory situations
  virtual void PrepareForEncoding(void);
  //
  // First time usage: Collect all the information for decoding.
  // May throw on out of memory situations.
  virtual void PrepareForDecoding(void);
  //
  // Return the color transformer responsible for this scan.
  class ColorTrafo *ColorTrafoOf(bool encoding,bool disabletorgb);
  //
  // First step of a region encoder: Find the region that can be pulled in the next step,
  // from a rectangle request. This potentially shrinks the rectangle, which should be
  // initialized to the full image.
  virtual void CropEncodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *rr);
  //
  // Request user data for encoding for the given region, potentially clip the region to the
  // data available from the user.
  virtual void RequestUserDataForEncoding(class BitMapHook *bmh,RectAngle<LONG> &region,bool alpha);
  //
  // Pull data buffers from the user data bitmap hook
  virtual void RequestUserDataForDecoding(class BitMapHook *bmh,RectAngle<LONG> &region,
                                          const struct RectangleRequest *rr,bool alpha); 
  //
  // Encode a region, push it into the internal buffers and
  // prepare everything for coding.
  virtual void EncodeRegion(const RectAngle<LONG> &region);
  //
  // Reconstruct a block, or part of a block
  virtual void ReconstructRegion(const RectAngle<LONG> &region,const struct RectangleRequest *rr);
  //
  // Return true if the next MCU line is buffered and can be pushed
  // to the encoder.
  virtual bool isNextMCULineReady(void) const;
  //
  // Reset all components on the image side of the control to the
  // start of the image. Required when re-requesting the image
  // for encoding or decoding.
  virtual void ResetToStartOfImage(void); 
  //
  // Return an indicator whether all of the image has been loaded into
  // the image buffer.
  virtual bool isImageComplete(void) const;
  //
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const
  {
    return false;
  }  
  //
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(const struct RectangleRequest *rr) const
  {
    return BlockBuffer::BufferedLines(rr);
  }
  //
  // Install a block helper.
  void SetBlockHelper(class ResidualBlockHelper *helper);
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    BitmapCtrl::PostImageHeight(lines);
    BlockBuffer::PostImageHeight(lines);
    
    m_ulPixelHeight = lines;
  }
};
///

///
#endif
