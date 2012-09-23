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
** A subsequent (refinement) scan of a progressive scan.
**
** $Id: refinementscan.hpp,v 1.15 2012-09-23 14:10:12 thor Exp $
**
*/

#ifndef CODESTREAM_REFINEMENTSCAN_HPP
#define CODESTREAM_REFINEMENTSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "io/bitstream.hpp"
#include "io/memorystream.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/entropyparser.hpp"
///

/// Forwards
class HuffmanDecoder;
class HuffmanCoder;
class HuffmanStatistics;
class Tables;
class ByteStream;
class DCT;
class Frame;
struct RectangleRequest;
class BlockBitmapRequester;
class BlockBuffer;
///

/// class RefinementScan
class RefinementScan : public EntropyParser {
  //
  // The huffman AC tables
  class HuffmanDecoder    *m_pACDecoder[4];
  //
  // Ditto for the encoder
  class HuffmanCoder      *m_pACCoder[4];
  //
  // Ditto for the statistics collection.
  class HuffmanStatistics *m_pACStatistics[4];
  //
  // Number of blocks still to skip over.
  // This is only used in the progressive mode.
  UWORD                    m_usSkip[4];
  //
  // Scan positions.
  ULONG                    m_ulX[4];
  //
  // The bitstream from which we read the data.
  BitStream<false>         m_Stream;
  //
  // The AC temporary buffer containing the bits for
  // the non-zero coefficients. These will be written
  // *after* the corresponding run and need to be
  // buffered somewhere.
  class MemoryStream       m_ACBuffer;
  //
protected:
  //
  // Measure data?
  bool                     m_bMeasure;
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockBuffer          *m_pBlockCtrl; 
  //
  // Scan parameters.
  UBYTE                       m_ucScanStart;
  UBYTE                       m_ucScanStop;
  UBYTE                       m_ucLowBit; // First bit (from LSB) to code in this scan
  UBYTE                       m_ucHighBit; // First bit *NOT* to code anymore.
  //
  //
  // Encode a single huffman block
  void EncodeBlock(const LONG *block,
		   class HuffmanCoder *ac,
		   UWORD &skip);
  //
  // Decode a single huffman block.
  void DecodeBlock(LONG *block,
		   class HuffmanDecoder *ac,
		   UWORD &skip);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final);
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
private:
  //
  // Make a block statistics measurement on the source data.
  void MeasureBlock(const LONG *block,
		    class HuffmanStatistics *ac,
		    UWORD &skip);
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  // Code any run of zero blocks here. This is only valid in
  // the progressive mode.
  void CodeBlockSkip(class HuffmanCoder *ac,UWORD &skip);
  //
public:
  // Create a refinement scan. The differential flag is always ignored, so
  // is the residual flag.
  RefinementScan(class Frame *frame,class Scan *scan,UBYTE start,UBYTE stop,
		 UBYTE lowbit,UBYTE highbit,
		 bool differential = false,bool residual = false);
  //
  ~RefinementScan(void);
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
