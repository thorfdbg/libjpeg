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
** This class represents a single frame and the frame dimensions.
**
** $Id: frame.hpp,v 1.46 2012-09-23 12:58:39 thor Exp $
**
*/


#ifndef MARKER_FRAME_HPP
#define MARKER_FRAME_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Forwards
class ByteStream;
class Component;
class Tables;
class Scan;
class BitmapCtrl;
class LineAdapter;
class BufferCtrl;
class ResidualBlockHelper;
///

/// class Frame
// This class represents a single frame and the frame dimensions.
class Frame : public JKeeper {
  // 
  // In case this frame is part of a sequence of
  // hierarchical frames, this is the next larger
  // frame required to compose the full image.
  class Frame         *m_pNext;
  //
  // The tables of this frame, i.e. huffman and quantization tables.
  class Tables        *m_pTables;
  //
  // The scan pattern.
  class Scan          *m_pScan;
  //
  // The last scan.
  class Scan          *m_pLast;
  //
  // The currently active scan.
  class Scan          *m_pCurrent;
  //
  // The buffer this frame.
  class BufferCtrl    *m_pImage;
  //
  // Computes the residual data.
  class ResidualBlockHelper *m_pBlockHelper;
  //
  // The type of the frame encoding.
  ScanType             m_Type;
  //
  // width of the image in pixels.
  ULONG                m_ulWidth; 
  //
  // height of the image in pixels.
  ULONG                m_ulHeight;
  //
  // Sample precision in bits.
  UBYTE                m_ucPrecision;
  //
  // Number of components.
  UBYTE                m_ucDepth;
  //
  // Maximum MCU width and height. This data is required to compute
  // the subsampling factors.
  UBYTE                m_ucMaxMCUWidth;
  UBYTE                m_ucMaxMCUHeight;
  //
  // The definition of the components, the component array.
  class Component    **m_ppComponent;
  //
  // Indicate the height by the DNL marker?
  bool                 m_bWriteDNL;
  //
  // State flags for parsing. Make the next scan a residual
  // scan even though there is no more data in the IO stream?
  bool                 m_bBuildResidual;
  //
  // Has the residual scan already been created? 
  bool                 m_bCreatedResidual;
  //
  // The same logic for hidden DCT refinement scans.
  bool                 m_bBuildRefinement;
  bool                 m_bCreatedRefinement;
  //
  // Compute the largest common denominator of a and b.
  static int gcd(int a,int b)
  {
    while(b) {
      int t  = a % b;
      a  = b;
      b  = t;
    }

    return a;
  }
  //
  // Define a component for writing. Must be called exactly once per component for encoding.
  // idx is the component index (not its label, which is generated automatically), and
  // the component subsampling factors. Must be called after installing precision and depth.
  class Component *DefineComponent(UBYTE idx,UBYTE subx = 1,UBYTE suby = 1);
  //
  // Compute the MCU sizes of the components from the subsampling values
  void ComputeMCUSizes(void);
  //
  // Start parsing a scan. Returns true if the scan start is found and there is another hidden
  // scan. Returns false otherwise.
  bool ScanForScanHeader(class ByteStream *stream);
  //
public:
  // This requires a type identifier.
  Frame(class Tables *tables,ScanType t);
  //
  ~Frame(void);
  //
  // Next frame in a sequence of hierarchical frames.
  class Frame *NextOf(void) const
  {
    return m_pNext;
  }
  //
  // Tag on a frame
  void TagOn(class Frame *next)
  {
    assert(m_pNext == NULL);
    m_pNext = next;
  }
  //
  // Set the image the frame data goes into. Required before the
  // user can call StartParse|Write|MeasureScan.
  void SetImage(class BufferCtrl *img)
  {
    m_pImage = img;
  }
  //
  // Parse off a frame header
  void ParseMarker(class ByteStream *io);
  //
  // Write the frame header
  void WriteMarker(class ByteStream *io);
  //
  // Find a component by a component identifier. Throws if the component does not exist.
  class Component *FindComponent(UBYTE id) const;
  //
  // Return the width of the frame in pixels.
  ULONG WidthOf(void) const
  {
    return m_ulWidth;
  }
  //
  // Return the height of the frame in pixels, or zero if it is still undefined.
  ULONG HeightOf(void) const
  {
    return m_ulHeight;
  }
  //
  // Return the number of components.
  UBYTE DepthOf(void) const
  {
    return m_ucDepth;
  }
  //
  // Return the precision in bits per sample.
  UBYTE PrecisionOf(void) const
  {
    return m_ucPrecision;
  }
  //
  // Return the precision including the hidden bits.
  UBYTE HiddenPrecisionOf(void) const;
  //
  // Return the point preshift, the adjustment of the
  // input samples by a shift that moves them into the
  // limits of JPEG.
  UBYTE PointPreShiftOf(void) const;
  //
  // Define default scan parameters. Returns the scan for further refinement if required.
  class Scan *InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,UBYTE precision,
				       bool writednl,
				       const UBYTE *subx,const UBYTE *suby,
				       const struct JPG_TagItem *tags);
  //
  // Start parsing a single scan.
  class Scan *StartParseScan(class ByteStream *io);
  //
  // Start writing a single scan. Scan parameters must have been installed before.
  class Scan *StartWriteScan(class ByteStream *io);
  //
  // Start a measurement scan that can be added upfront to optimize the huffman
  // coder
  class Scan *StartMeasureScan(void);
  //
  // Return the scan.
  class Scan *FirstScanOf(void) const
  {
    return m_pScan;
  }
  //
  // Return the currently active scan.
  class Scan *CurrentScanOf(void) const
  {
    return m_pCurrent;
  }
  //
  // Advance the current frame to the next one,
  // returns it if there is a next one, NULL
  // otherwise if all scans are written.
  class Scan *NextScan(void);
  //
  // Reset the scan to the first in the image
  void ResetToFirstScan(void)
  {
    m_pCurrent = m_pScan;
  }
  //
  // The scan type of this frame, or rather the frame type.
  ScanType ScanTypeOf(void) const
  {
    return m_Type;
  }
  //
  // Return the settings tables of this frame.
  class Tables *TablesOf(void) const
  {
    return m_pTables;
  }
  //
  // Return the i'th component. Note that the argument
  // is here the component in the order they are defined
  // in the frame, not in the scan. The argument is not
  // a component ID but its relative index
  class Component *ComponentOf(UBYTE idx) const
  {
    assert(idx < m_ucDepth);
    return m_ppComponent[idx];
  }
  //
  // Write the marker that identifies this type of frame, and all the scans within it.
  void WriteFrameType(class ByteStream *io) const;
  //
  // Parse off the EOI marker at the end of the image. Return false
  // if there are no more scans in the file, true otherwise.
  bool ParseTrailer(class ByteStream *io);
  //
  // Build the line adapter fitting to the frame type.
  class LineAdapter *BuildLineAdapter(void);
  //
  // Build the image buffer type fitting to the frame type.
  class BitmapCtrl *BuildImage(void);
  //
  // Write the scan trailer of this frame. This is only the
  // DNL marker if it is enabled.
  void WriteTrailer(class ByteStream *io);
  //
  // Define the image size if it is not yet known here. This is
  // called whenever the DNL marker is parsed in.
  void PostImageHeight(ULONG height);
};
///

///
#endif
