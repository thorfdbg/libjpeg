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
** This class represents a single frame and the frame dimensions.
**
** $Id: frame.hpp,v 1.68 2017/08/17 13:24:01 thor Exp $
**
*/


#ifndef MARKER_FRAME_HPP
#define MARKER_FRAME_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
#include "boxes/databox.hpp"
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
class Checksum;
class ChecksumAdapter;
class DCT;
///

/// class Frame
// This class represents a single frame and the frame dimensions.
class Frame : public JKeeper {
  // 
  // The image of this frame
  class Image           *m_pParent;
  //
  // In case this frame is part of a sequence of
  // hierarchical frames, this is the next larger
  // frame required to compose the full image.
  class Frame           *m_pNext;
  //
  // The tables of this frame, i.e. huffman and quantization tables.
  class Tables          *m_pTables;
  //
  // The scan pattern.
  class Scan            *m_pScan;
  //
  // The last scan.
  class Scan            *m_pLast;
  //
  // The currently active scan.
  class Scan            *m_pCurrent;
  //
  // The buffer this frame.
  class BufferCtrl      *m_pImage;
  //
  // Computes the residual data.
  class ResidualBlockHelper *m_pBlockHelper;
  //
  // The type of the frame encoding.
  ScanType               m_Type;
  //
  // width of the image in pixels.
  ULONG                  m_ulWidth; 
  //
  // height of the image in pixels.
  ULONG                  m_ulHeight;
  //
  // Sample precision in bits.
  UBYTE                  m_ucPrecision;
  //
  // Number of components.
  UBYTE                  m_ucDepth;
  //
  // Maximum MCU width and height. This data is required to compute
  // the subsampling factors.
  UBYTE                  m_ucMaxMCUWidth;
  UBYTE                  m_ucMaxMCUHeight;
  //
  // The definition of the components, the component array.
  class Component      **m_ppComponent;
  //
  // Currently active refinement data box.
  class DataBox         *m_pCurrentRefinement;
  //
  // The current adapter for updating the checksum over the
  // encoded data.
  class ChecksumAdapter *m_pAdapter;
  //
  // Indicate the height by the DNL marker?
  bool                   m_bWriteDNL;
  //
  // State flags for parsing. Make the next scan a refinement
  // scan even though there is no more data in the IO stream?
  bool                   m_bBuildRefinement;
  bool                   m_bCreatedRefinement;
  bool                   m_bEndOfFrame;
  bool                   m_bStartedTables;
  //
  // Counts the refinement scans.
  UWORD                  m_usRefinementCount;
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
  // Attach a new scan to the frame, return the scan
  // and make this the current scan.
  class Scan *AttachScan(void);
  //
  // Helper function to create a regular scan from the tags.
  // There are no scan tags here, instead all components are included.
  // If breakup is set, then each component gets its own scan, otherwise
  // groups of four components get into one scan.
  void CreateSequentialScanParameters(bool breakup,ULONG tagoffset,const struct JPG_TagItem *tags);
  //
  // Helper function to create progressive scans. These need to be broken
  // up over several components. A progressive scan cannot contain more
  // than one component if it includes AC parameters.
  void CreateProgressiveScanParameters(bool breakup,ULONG tagoffset,const struct JPG_TagItem *tags,
                                       const struct JPG_TagItem *scantags);
  //
public:
  // This requires a type identifier.
  Frame(class Image *parent,class Tables *tables,ScanType t);
  //
  ~Frame(void);
  //
  // Return the image this frame is part of.
  class Image *ImageOf(void) const
  {
    return m_pParent;
  }
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
  void SetImageBuffer(class BufferCtrl *img)
  {
    m_pImage = img;
  }
  //
  // Return an indicator whether the end of a frame was reached.
  bool isEndOfFrame(void) const
  {
    return m_bEndOfFrame;
  }
  //
  // Extend the image by a merging process, and install it
  // here.
  void ExtendImageBuffer(class BufferCtrl *img,class Frame *residual);
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
  // limits of JPEG. This is the R_b value from the specs.
  UBYTE PointPreShiftOf(void) const;
  //
  // Define default scan parameters. Returns the scan for further refinement if required.
  // tagoffset is an offset added to the tags - used to read from the residual scan types
  // rather the regular ones if this is a residual frame.
  class Scan *InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,UBYTE precision,
                                       bool writednl,
                                       const UBYTE *subx,const UBYTE *suby,
                                       ULONG tagoffset,
                                       const struct JPG_TagItem *tags);
  //
  // Start parsing a single scan. Could also create a checksum
  // in case the APP markers come late.
  class Scan *StartParseScan(class ByteStream *io,class Checksum *chk);
  //
  // Start writing a single scan. Scan parameters must have been installed before.
  class Scan *StartWriteScan(class ByteStream *io,class Checksum *chk);
  //
  // Start a measurement scan that can be added upfront to optimize the huffman
  // coder
  class Scan *StartMeasureScan(void);
  //
  // Start an optimization scan for the R/D optimizer.
  class Scan *StartOptimizeScan(void);
  //
  // End parsing the current scan.
  void EndParseScan(void);
  //
  // End writing the current scan
  void EndWriteScan(void);
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
  // Return an indicator whether this is a DCT-based frame type.
  // This seems like an obvious choice given the scan type, but
  // it is not for hieararchical as this may mix lossless
  // differential with DCT-based modes.
  bool isDCTBased(void) const;
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
  class BitmapCtrl *BuildImageBuffer(void);
  //
  // Write the scan trailer of this frame. This is only the
  // DNL marker if it is enabled.
  void WriteTrailer(class ByteStream *io);
  //
  // Complete the current refinement scan if there is one.
  void CompleteRefimentScan(class ByteStream *io);
  //
  // Define the image size if it is not yet known here. This is
  // called whenever the DNL marker is parsed in.
  void PostImageHeight(ULONG height);
  //
  // Optimize a single DCT block through all scans of this frame for
  // ideal R/D performance.
  void OptimizeDCTBlock(LONG bx,LONG by,UBYTE compidx,class DCT *dct,LONG block[64]);
  //
};
///

///
#endif
