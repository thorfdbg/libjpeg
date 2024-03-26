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
** This class represents the image as a whole, consisting either of a single
** or multiple frames.
**
** $Id: image.hpp,v 1.35 2024/03/26 10:04:47 thor Exp $
**
*/

#ifndef MARKER_CODESTREAM_IMAGE_HPP
#define MARKER_CODESTREAM_IMAGE_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
#include "marker/frame.hpp"
///

/// Forwards
class ByteStream;
class Component;
class Tables;
class Scan;
class BitmapCtrl;
class Frame;
class ResidualBuffer;
class Checksum;
class MemoryStream;
class ChecksumAdapter;
class Box;
///

/// class Image
// This class represents the image as a whole, consisting either of a single
// or multiple frames.
class Image : public JKeeper {
  // 
  // In case this image has a residual image: Here it goes. This residual
  // image contains additional information to refine the source and make an
  // HDR image available.
  class Image           *m_pResidual;
  //
  // In case this image contains an alpha channel, here it is. The alpha channel
  // may again have a residual.
  class Image           *m_pAlphaChannel;
  //
  // In case this image is a residual image, here is the legacy image.
  class Image           *m_pParent;
  //
  // In case this is an alpha channel, the color/image data is here.
  class Image           *m_pMaster;
  //
  // The tables of this frame, i.e. huffman and quantization tables.
  class Tables          *m_pTables;
  //
  // The pointer that controls the life time of the tables. Not
  // used for anything else. Residual tables and alpha tables are
  // controlled in the table itself.
  class Tables          *m_pTableOwner;
  //
  // This frame marker contains the image characteristics. In case the image
  // is non-hierachical, this is the frame representing the image itself.
  // Otherwise, it is just what the DHP marker represents, and the frames
  // start below which are then combined into the image.
  class Frame           *m_pDimensions;
  //
  // The first (smallest) frame of a hierarchical image, or NULL if this is
  // a standard image.
  class Frame           *m_pSmallest;
  //
  // The last frame.
  class Frame           *m_pLast;
  //
  // The currently active scan.
  class Frame           *m_pCurrent;
  //
  // The overall image as seen from the user. Depending on the
  // type of the image, this may consist of various classes...
  // Note that the bitmap control only exists once, i.e. alpha and image
  // (and alpha residual and image residual) share a single bitmap ctrl.
  class BitmapCtrl      *m_pImageBuffer;
  //
  // In case we have a residual image, keep it here. This is only available
  // for blocky modes right now.
  class ResidualBuffer  *m_pResidualImage;
  //
  // In case a checksum is required: The checksum is kept here.
  class Checksum        *m_pChecksum;
  //
  // Buffers the legacy stream until the checksum is computed.
  class MemoryStream    *m_pLegacyStream;
  //
  // Adapter between this and the checksum.
  class ChecksumAdapter *m_pAdapter;
  //
  // When writing images, this contains the checksum box
  // and nothing else.
  class Box             *m_pBoxList;
  //
  // If this flag is set, then the frame header has already been removed
  // and does not require removal again. This is part of the
  // decoding state machine and due to the fact that the residual and
  // alpha stream frame headers are parsed during ParseTrailer during testing
  // whether there is another frame.
  bool                   m_bReceivedFrameHeader;
  //
  // Create the buffer providing an access path to the residuals, if available.
  // This works only for block based modes, line based modes do not create 
  // residuals.
  class BufferCtrl *CreateResidualBuffer(class BufferCtrl *img);
  //
  // Parse off the residual stream. Returns the residual frame if it exists, or NULL
  // in case it does not or there are no more scans in the file.
  class Frame *ParseResidualStream(class DataBox *residual);
  //
  // Parse off the alpha channel. Returns the alpha frame if it is exists, or NULL
  // in case it does not or there are no more scans in this frame.
  class Frame *ParseAlphaChannel(class DataBox *box);
  //
  // Convert a frame marker to a scan type, return it.
  ScanType FrameMarkerToScanType(LONG marker) const;
  //
  // Check whether a scan type is a differential scan
  // and hence can only be used in a hierarchical JPEG.
  static bool isDifferentialType(ScanType type);
  //
  // Select the first frame to write to, return it.
  class Frame *FindFirstWriteFrame(void) const;
  //
  // Write the image header belonging to the given frame to the given stream.
  void WriteImageAndFrameHeader(class Frame *frame,class ByteStream *target) const;
  //
  // Given an image, get the target buffer where the data goes when writing, or the
  // regular stream passed in in case data can be written directly.
  class DataBox *OutputBufferOf(void);
  //
  // Complete the side channel, i.e. complete it and flush it out.
  void FlushSideChannel(class ByteStream *target);
  //
  // Parse off the frame header and construct the frame, then return it.
  class Frame *ParseFrameHeader(class ByteStream *io);
  //
  // Create the frame, or frame hierarchy, from the given type
  // This probably builds the hierarchical buffer if there is one.
  // eh and ev are the horizontal and vertical expansion flags that
  // indicate the frame expansion for hierarchical JPEG.
  class Frame *CreateFrameBuffer(class ByteStream *io,ScanType type);
  //
public:
  //
  Image(class Environ *env);
  //
  ~Image(void);
  //
  //
  // Create a residual image and install it here.
  class Image *CreateResidualImage(void);
  //
  // Attach an alpha channel to this image and return it.
  class Image *CreateAlphaChannel(void);
  //
  // Return the side information of this image or create it.
  class Tables *TablesOf(void);
  //
  // Return an indicator whether this is possibly a hierarchical scan
  bool isHierarchical(void) const
  {
    if (m_pSmallest)
      return true;
    return false;
  }
  //
  // Return the alpha channel if we have one.
  class Image *AlphaChannelOf(void) const
  {
    return m_pAlphaChannel;
  }
  //
  // Return the width of the frame in pixels.
  ULONG WidthOf(void) const
  {
    if (m_pDimensions == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Image::WidthOf","no image created or loaded");
    return m_pDimensions->WidthOf();
  }
  //
  // Return the height of the frame in pixels, or zero if it is still undefined.
  ULONG HeightOf(void) const
  {
    ULONG height;
    
    if (m_pDimensions == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Image::HeightOf","no image created or loaded");
    
    height = m_pDimensions->HeightOf();
    //
    // If the DNL marker is used, it might be that this is zero. In this case,
    // take from the largest scale.
    if (height == 0 && m_pLast) {
      height = m_pLast->HeightOf();
    }
    return height;
  }
  //
  // Return the number of components.
  UBYTE DepthOf(void) const
  {
     if (m_pDimensions == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Image::DepthOf","no image created or loaded");
   return m_pDimensions->DepthOf();
  }
  //
  // Return the precision in bits per sample.
  UBYTE PrecisionOf(void) const
  {
     if (m_pDimensions == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Image::PrecisionOf","no image created or loaded");
     return m_pDimensions->PrecisionOf() + m_pDimensions->PointPreShiftOf();
  }
  //
  // Define default scan parameters. Returns the frame smallest frame or the only frame.
  // Levels is the number of decomposition levels for the hierarchical mode. It is zero
  // for the regular "flat" mode. Tag offset is an offset added to the tags for
  // defining the residual image.
  void InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,
                                UBYTE precision,ScanType type,UBYTE levels,bool scale,
                                bool writednl,const UBYTE *subx,const UBYTE *suby,
                                ULONG tagoffset,const struct JPG_TagItem *tags);
  //
  // Start parsing a single frame. If the second argument is set, the scan
  // type is a residual scan type.
  class Frame *StartParseFrame(class ByteStream *io);
  //
  // Start writing a single scan. Scan parameters must have been installed before.
  class Frame *StartWriteFrame(class ByteStream *io);
  //
  // Start a measurement scan that can be added upfront to optimize the huffman
  // coder
  class Frame *StartMeasureFrame(void);
  //
  // Start an optimization scan that can be added upfront the measurement to 
  // improve the R/D performance.
  class Frame *StartOptimizeFrame(void);
  //
  // Return the scan.
  class Frame *FirstFrameOf(void) const
  {
    return m_pSmallest?m_pSmallest:m_pDimensions;
  }
  //
  // Return the currently active scan.
  class Frame *CurrentFrameOf(void) const
  {
    return m_pCurrent;
  }
  //
  // Advance to the next frame, deliver it or NULL if there is no next frame.
  class Frame *NextFrame(void);
  //
  // Reset the scan to the first in the image
  void ResetToFirstFrame(void);
  //
  // Return the input stream data should come from. This might be the
  // residual stream if the current frame is the residual frame.
  // It is otherwise the unmodified input.
  class ByteStream *InputStreamOf(class ByteStream *legacy) const;
  //
  // Return the output stream data should go to. This might be the
  // residual stream if the current frame is the residual frame.
  // It is otherwise the unmodified input.
  class ByteStream *OutputStreamOf(class ByteStream *legacy) const;
  //
  // Return the settings tables of this frame.
  class Tables *TablesOf(void) const
  {
    return m_pTables;
  }
  //
  // Return the checksum so far if we need to keep one.
  class Checksum *ChecksumOf(void) const;
  //
  // Create a checksum when this is the main image, and a checksum
  // in the tables is needed.
  class Checksum *CreateChecksumWhenNeeded(class Checksum *chk);
  //
  // Write the header and header tables up to the SOS marker.
  void WriteHeader(class ByteStream *io) const;
  //
  // Control interface - direct forwarding to the bitmap control
  // Reconstruct a rectangle of coefficients.
  void ReconstructRegion(class BitMapHook *bmh,const struct RectangleRequest *rr);
  //
  // Encode the next region in the scan from the user bitmap. The requested region
  // is indicated in the tags going to the user bitmap hook.
  void EncodeRegion(class BitMapHook *bmh,const struct RectangleRequest *rr);
  //
  // Return the number of lines available for reconstruction from this scan.
  ULONG BufferedLines(const struct RectangleRequest *rr) const;
  //
  // Return true if the next MCU line is buffered and can be pushed
  // to the encoder.
  bool isNextMCULineReady(void) const;
  //
  // Return an indicator whether all of the image has been loaded into
  // the image buffer.
  bool isImageComplete(void) const;
  //
  // Write the trailing data of the trailer, namely the EOI
  void WriteTrailer(class ByteStream *io);
  //
  // Parse off the EOI marker at the end of the image. Return false
  // if there are no more scans in the file, true otherwise.
  bool ParseTrailer(class ByteStream *io);
};
///

///
#endif
