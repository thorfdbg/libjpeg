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
** This scan type decodes the coding residuals and completes an image
** into a lossless image.
**
** $Id: residualhuffmanscan.hpp,v 1.8 2012-07-27 08:08:33 thor Exp $
**
*/

#ifndef CODESTREAM_RESIDUALHUFFMANSCAN_HPP
#define CODESTREAM_RESIDUALHUFFMANSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "io/bitstream.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/entropyparser.hpp"
#include "coding/qmcoder.hpp"
#include "control/residualblockhelper.hpp"
#include "io/bitstream.hpp"
///

/// Forwards
class Tables;
class ByteStream;
class DCT;
class Frame;
struct RectangleRequest;
class BlockBitmapRequester;
class BlockBuffer;
class MemoryStream;
class HuffmanDecoder;
class HuffmanCoder;
class HuffmanStatistics;
class HuffmanTable;
///

/// class ResidualHuffmanScan
class ResidualHuffmanScan : public EntropyParser {
  //
  // The huffman tables for this scan.
  class HuffmanTable         *m_pTable;
  //
  // The huffman DC tables: Tables are used in a somewhat
  // different way.
  class HuffmanDecoder       *m_pDecoder[8];
  //
  // Ditto for the encoder
  class HuffmanCoder         *m_pCoder[8];
  //
  // Ditto for the statistics collection.
  class HuffmanStatistics    *m_pStatistics[8];
  //
  // Scan positions.
  ULONG                      *m_pulX;
  ULONG                      *m_pulY;
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockBuffer          *m_pBlockCtrl; 
  //
  // Buffers the output before it is split into APP4 markers.
  class MemoryStream         *m_pResidualBuffer;
  //
  // Where the data finally ends up.
  class ByteStream           *m_pTarget;
  //
  class ResidualBlockHelper   m_Helper;
  //
  // Where the huffman coder puts and gets data.
  BitStream<false>            m_Stream;
  //
  // Measure the scan? If so, nothing happens. As this just extends
  // scans, even huffman scans, nothing will happen then, but no error
  // can be generated.
  bool                        m_bMeasure;
  //
  // The mapping of Hadamard bands to coding classes.
  static const UBYTE          m_ucCodingClass[64];
  //
  // Cancelation of the DC gain due to gamma mapping.
  LONG                       *m_plDC;
  // Event counter
  LONG                       *m_plN;
  // Average error counter
  LONG                       *m_plB;
  //  
  // Enable or disable the Hadamard transformation.
  bool                        m_bHadamard;
  //
  // Update the predicted DC gain.
  void ErrorAdaption(LONG dc,UBYTE comp)
  {
    m_plB[comp] += dc;
    m_plN[comp]++;
    m_plDC[comp] = m_plB[comp] / m_plN[comp];

    if (m_plN[comp] > 64) {
      m_plN[comp] >>= 1;
      if (m_plB[comp] >= 0) {
	m_plB[comp] = (m_plB[comp]) >> 1;
      } else {
	m_plB[comp] = -((-m_plB[comp]) >> 1);
      }
    }
  }
  //
  // Init the counters for the statistical measurement.
  void InitStatistics(void);
  //
  // Encode a single residual block
  void EncodeBlock(const LONG *rblock,UBYTE c);
  //
  // Decode a single residual block.
  void DecodeBlock(LONG *block,UBYTE c);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(void);
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void)
  {
    // As this never writes restart markers but rather relies on
    // the APP9 marker segment for restart, if any, this should
    // never be called.
    assert(!"residual streams do not write restart markers");
  }
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  //
public:
  ResidualHuffmanScan(class Frame *frame,class Scan *scan);
  //
  ~ResidualHuffmanScan(void);
  // 
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding 
  virtual void StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Measure scan statistics.
  virtual void StartMeasureScan(class BufferCtrl *ctrl);
  //
  // Start a MCU scan. Returns true if there are more rows. False otherwise.
  virtual bool StartMCURow(void);
  //
  // Parse a single MCU in this scan. Return true if there are more
  // MCUs in this row.
  virtual bool ParseMCU(void);  
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void); 
};
///


///
#endif
