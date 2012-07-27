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
** This class represents the image as a whole, consisting either of a single
** or multiple frames.
**
** $Id: image.hpp,v 1.10 2012-06-11 10:19:15 thor Exp $
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
///

/// class Image
// This class represents the image as a whole, consisting either of a single
// or multiple frames.
class Image : public JKeeper {
  // 
  // The tables of this frame, i.e. huffman and quantization tables.
  class Tables        *m_pTables;
  //
  // This frame marker contains the image characteristics. In case the image
  // is non-hierachical, this is the frame representing the image itself.
  // Otherwise, it is just what the DHP marker represents, and the frames
  // start below which are then combined into the image.
  class Frame         *m_pDimensions;
  //
  // The first (smallest) frame of a hierarchical image, or NULL if this is
  // a standard image.
  class Frame         *m_pSmallest;
  //
  // The last frame.
  class Frame         *m_pLast;
  //
  // The currently active scan.
  class Frame         *m_pCurrent;
  //
  // The overall image as seen from the user. Depending on the
  // type of the image, this may consist of various classes...
  class BitmapCtrl    *m_pImage;
  //
public:
  //
  Image(class Tables *tables);
  //
  ~Image(void);
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
  // for the regular "flat" mode.
  void InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,
				UBYTE precision,ScanType type,UBYTE levels,bool scale,
				bool writednl,const UBYTE *subx,const UBYTE *suby,
				const struct JPG_TagItem *tags);
  //
  // Start parsing a single frame.
  class Frame *StartParseFrame(class ByteStream *io);
  //
  // Start writing a single scan. Scan parameters must have been installed before.
  class Frame *StartWriteFrame(class ByteStream *io);
  //
  // Start a measurement scan that can be added upfront to optimize the huffman
  // coder
  class Frame *StartMeasureFrame(void);
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
  // Return the settings tables of this frame.
  class Tables *TablesOf(void) const
  {
    return m_pTables;
  }
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
  void WriteTrailer(class ByteStream *io) const;
  //
  // Parse off the EOI marker at the end of the image. Return false
  // if there are no more scans in the file, true otherwise.
  bool ParseTrailer(class ByteStream *io) const;
  //
};
///

///
#endif
