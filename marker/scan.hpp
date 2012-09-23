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
** Represents all data in a single scan, and hence is the SOS marker.
**
** $Id: scan.hpp,v 1.49 2012-09-23 14:10:13 thor Exp $
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
  // All settings are now stored, prepare the parser
  void CompleteSettings(void);
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
  void InstallDefaults(UBYTE depth,const struct JPG_TagItem *tags);
  //
  // Make this scan not a default scan, but a residual scan. This
  // creates the AC part of the scan (actually, the scan for the remaining positions)
  void MakeResidualScan(class Component *comp);
  //
  // Make this scan a hidden refinement scan starting at the indicated
  // bit position in the indicated component label.
  void MakeHiddenRefinementACScan(UBYTE bitposition,class Component *comp);
  //
  // Make this scan a hidden refinement scan starting at the indicated
  // bit position for the DC part. This is interleaved.
  void MakeHiddenRefinementDCScan(UBYTE bitposition);
  //
  // Parse off a hidden refinement scan from the given position.
  void StartParseHiddenRefinementScan(class BufferCtrl *ctrl);
  //
 // Parse off a residual scan from the given position.
  void StartParseResidualScan(class BufferCtrl *ctrl);
  //
  // Fill in the decoding tables required.
  void StartParseScan(class ByteStream *io,class BufferCtrl *ctrl); 
  //
  // Fill in the encoding tables.
  void StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl); 
  //
  // Start making a measurement run to optimize the
  // huffman tables.
  void StartMeasureScan(class BufferCtrl *ctrl);
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
};
///

///
#endif
