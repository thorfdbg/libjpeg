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
** Represents all data in a single scan, and hence is the SOS marker.
**
** $Id: scan.hpp,v 1.66 2017/06/06 10:51:41 thor Exp $
**
*/


#ifndef MARKER_SCAN_HPP
#define MARKER_SCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "marker/scantypes.hpp"
#include "std/assert.hpp"
///

/// Forwards
class ByteStream;
class Component;
class Frame;
class Tables;
class QuantizedRow;
struct ImageBitMap;
class BitMapHook;
struct RectangleRequest;
class UpsamplerBase;
class DownsamplerBase;
class BitmapCtrl;
class BufferCtrl;
class LineAdapter;
class EntropyParser;
class Checksum;
///

/// class Scan
// This class implements the scan header.
class Scan : public JKeeper {
  //
  // Next scan in line, potentially covering more
  // components.
  class Scan            *m_pNext;
  //
  // Frame this scan is part of.
  class Frame           *m_pFrame;
  //
  // The codestream parser that interprets the entropy coded
  // data. Not done here.
  class EntropyParser   *m_pParser;
  // 
  // Scans may have private AC coding tables that adapt
  // to the statistics of the components within. If so,
  // such tables are here. These are not used on decoding
  // where tables come from the global "tables".
  //
  // The huffman table.
  class HuffmanTable    *m_pHuffman;
  //
  // The AC table.
  class ACTable         *m_pConditioner;
  //
  // Number of the components in the scan.
  UBYTE                  m_ucCount;
  //
  // Index of the scan. This is just for housekeeping and not
  // part of the JPEG syntax.
  UBYTE                  m_ucScanIndex;
  //
  // Components selected for the scan, there are as many as
  // indicated above.
  UBYTE                  m_ucComponent[4];
  //
  // The DC coding table selector.
  UBYTE                  m_ucDCTable[4];
  //
  // The AC coding table selector.
  UBYTE                  m_ucACTable[4];
  //
  // Spectral coding selector, start of scan.
  // Also the NEAR value for JPEG-LS
  UBYTE                  m_ucScanStart;
  //
  // Spectral coding selector, end of scan.
  // Also the interleaving value for JPEG-LS.
  UBYTE                  m_ucScanStop;
  //
  // Start approximation high bit position.
  UBYTE                  m_ucHighBit;
  //
  // End of approximation low bit position.
  // Also the point transformation.
  UBYTE                  m_ucLowBit;
  //
  // Number of hidden bits not included in the low bit count.
  UBYTE                  m_ucHiddenBits;
  //
  // Set if this scan is a hidden scan and goes into a 
  // side channel.
  bool                   m_bHidden;
  //
  // Mapping table selector for JPEG_LS
  UBYTE                  m_ucMappingTable[4];
  //
  // Component pointers
  class Component       *m_pComponent[4];
  // 
  // Create a suitable parser given the scan type as indicated in the
  // header and the contents of the marker. The parser is kept
  // here as it is local to the scan.
  void CreateParser(void);
  //
public:
  //
  Scan(class Frame *frame);
  //
  virtual ~Scan(void);
  //
  // Flush the remaining bits out to the stream on writing.
  void Flush(void);
  //
  // Return the next scan found here.
  class Scan *NextOf(void) const
  {
    return m_pNext;
  }
  //
  // Tag on a next scan to this scan.
  void TagOn(class Scan *next)
  {
    assert(m_pNext == NULL);
    
    m_pNext = next;
    next->m_ucScanIndex = m_ucScanIndex + 1;
  }
  //
  // Return the i'th component of the scan.
  class Component *ComponentOf(UBYTE i);
  //
  // Return the number of the components in the scan.
  UBYTE ComponentsInScan(void) const
  {
    return m_ucCount;
  }
  //
  // Check whether this scan is in a side channel and hidden
  // in an extra box included in an APP11 marker.
  bool isHidden(void) const
  {
    return m_bHidden;
  }
  //
  // Find the DC huffman table of the indicated index.
  class HuffmanTemplate *FindDCHuffmanTable(UBYTE idx) const;
  //
  // Find the AC huffman table of the indicated index.
  class HuffmanTemplate *FindACHuffmanTable(UBYTE idx) const;
  //
  // Find the AC conditioner table for the indicated index
  // and the DC band.
  class ACTemplate *FindDCConditioner(UBYTE idx) const;
  //
  // The same for the AC band.
  class ACTemplate *FindACConditioner(UBYTE idx) const;
  //
  // Find the thresholds of the JPEG LS scan.
  class Thresholds *FindThresholds(void) const;
  //
  // Write the scan type marker at the beginning of the
  // file.
  void WriteFrameType(class ByteStream *io);
  //
  // Parse the marker contents. The scan type comes from
  // the frame type.
  void ParseMarker(class ByteStream *io);
  //
  // Parse the marker contents where the scan type
  // comes from an additional parameter.
  void ParseMarker(class ByteStream *io,ScanType type);
  //
  // Write the marker to the stream. Note that this should
  // be called indirectly by the implementing interface of
  // the entropy parser and not called here from toplevel.
  void WriteMarker(class ByteStream *io);
  //
  // Install the defaults for a given scan type 
  // containing the given number of components.
  // The tag offset is added to the tag to offset them for the
  // residual coding tags.
  void InstallDefaults(UBYTE depth,ULONG tagoffset,const struct JPG_TagItem *tags);
  //
  // Make this scan a hidden refinement scan starting at the indicated
  // bit position in the indicated component label. If start and stop are
  // both zero to indicate a DC scan, all components are included and comp
  // may be NULL.
  void MakeHiddenRefinementScan(UBYTE bitposition,class Component *comp,UBYTE sstart,UBYTE sstop);
  //
  // Parse off a hidden refinement scan from the given position.
  void StartParseHiddenRefinementScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Fill in the decoding tables required.
  void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl); 
  //
  // Fill in the encoding tables.
  void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl); 
  //
  // Start making a measurement run to optimize the
  // huffman tables.
  void StartMeasureScan(class BufferCtrl *ctrl);
  //
  // Start a rate/distortion optimization for scan on the given buffer.
  void StartOptimizeScan(class BufferCtrl *ctrl);
  //
  // Start a MCU scan.
  bool StartMCURow(void);  
  //
  // Parse a single MCU in this scan.
  bool ParseMCU(void);
  //
  // Write a single MCU in this scan.
  bool WriteMCU(void);
  //
  // Return the huffman decoder of the DC value for the
  // indicated component.
  class HuffmanDecoder *DCHuffmanDecoderOf(UBYTE idx) const;
  //
  // Return the huffman decoder of the DC value for the
  // indicated component.
  class HuffmanDecoder *ACHuffmanDecoderOf(UBYTE idx) const;
  //
  // Find the Huffman decoder of the indicated index.
  class HuffmanCoder *DCHuffmanCoderOf(UBYTE idx) const;
  //
  // Find the Huffman decoder of the indicated index.
  class HuffmanCoder *ACHuffmanCoderOf(UBYTE idx) const;
  //
  // Find the Huffman decoder of the indicated index.
  class HuffmanStatistics *DCHuffmanStatisticsOf(UBYTE idx) const;
  //
  // Find the Huffman decoder of the indicated index.
  class HuffmanStatistics *ACHuffmanStatisticsOf(UBYTE idx) const;
  //
  // Find the arithmetic coding conditioner table for the indicated
  // component and the DC band.
  class ACTemplate *DCConditionerOf(UBYTE idx) const;
  //
  // The same for the AC band.
  class ACTemplate *ACConditionerOf(UBYTE idx) const;
  //
  // Return the DC table of the conditioner.
  UBYTE DCTableIndexOf(UBYTE idx) const
  {
    assert(idx < 4);
    
    return m_ucDCTable[idx];
  } 
  //
  // Return the AC table of the conditioner.
  UBYTE ACTableIndexOf(UBYTE idx) const
  {
    assert(idx < 4);
    
    return m_ucACTable[idx];
  }
  //
  // Optimize the given DCT block for ideal rate-distortion performance. The
  // input parameters are the component this applies to, the critical R/D slope,
  // the original transformed but unquantized DCT data and the quantized DCT
  // block.
  void OptimizeDCTBlock(LONG bx,LONG by,UBYTE compidx,DOUBLE lambda,
                        class DCT *dct,LONG quantized[64]);
  //
  // Run a joint optimization of the R/D performance of all DC coefficients
  // within this scan. This requires a separate joint efford as DC coefficients
  // are encoded dependently.
  void OptimizeDC(void);
};
///

///
#endif
