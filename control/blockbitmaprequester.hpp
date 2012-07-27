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
** This class pulls blocks from the frame and reconstructs from those
** quantized block lines or encodes from them.
**
** $Id: blockbitmaprequester.hpp,v 1.15 2012-07-14 12:07:35 thor Exp $
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
  // Downsampling operator.
  class DownsamplerBase    **m_ppDownsampler;
  //
  // And the inverse, if required.
  class UpsamplerBase      **m_ppUpsampler;
  //
  // Temporary bitmaps
  struct ImageBitMap       **m_ppTempIBM;
  //
  // Temporary data pointers for the residual computation.
  LONG                     **m_ppQTemp;
  LONG                     **m_ppRTemp;
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
  // True if subsampling is required.
  bool                       m_bSubsampling;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
  // Create the next row of the image such that m_pppImage[i] is valid.
  class QuantizedRow *BuildImageRow(class QuantizedRow **qrow,int i);
  //
  //
public:
  //
  BlockBitmapRequester(class Frame *frame);
  //
  virtual ~BlockBitmapRequester(void); 
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
  class ColorTrafo *ColorTrafoOf(bool encoding);
  // 
  // Encode a region, push it into the internal buffers and
  // prepare everything for coding.
  virtual void EncodeRegion(class BitMapHook *bmh,const struct RectangleRequest *rr);
  //
  // Reconstruct a block, or part of a block
  virtual void ReconstructRegion(class BitMapHook *bmh,const struct RectangleRequest *rr);
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
  void SetBlockHelper(class ResidualBlockHelper *helper)
  {
    m_pResidualHelper = helper;
  }
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
