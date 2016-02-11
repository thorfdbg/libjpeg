/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** A sequential scan, also the first scan of a progressive scan,
** Huffman coded.
**
** $Id: sequentialscan.hpp,v 1.57 2015/02/25 16:09:41 thor Exp $
**
*/

#ifndef CODESTREAM_SEQUENTIALSCAN_HPP
#define CODESTREAM_SEQUENTIALSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "io/bitstream.hpp"
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
class BlockBuffer;
class BlockCtrl;
class BufferCtrl;
class LineAdapter;
class BitmapCtrl;
///

/// class SequentialScan
// A sequential scan, also the first scan of a progressive scan,
// Huffman coded.
class SequentialScan : public EntropyParser {
  //
  // Last DC value, required for the DPCM coder.
  LONG                     m_lDC[4];
  //
  // Number of blocks still to skip over.
  // This is only used in the progressive mode.
  UWORD                    m_usSkip[4];
  //
  // The bitstream from which we read the data.
  BitStream<false>         m_Stream;
  //
protected:
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockCtrl         *m_pBlockCtrl; 
  // 
  // Scan positions.
  ULONG                    m_ulX[4];
  //
  // The huffman DC tables
  class HuffmanDecoder    *m_pDCDecoder[4];
  //
  // The huffman AC tables
  class HuffmanDecoder    *m_pACDecoder[4];
  //
  // Ditto for the encoder
  class HuffmanCoder      *m_pDCCoder[4];
  class HuffmanCoder      *m_pACCoder[4];
  //
  // Ditto for the statistics collection.
  class HuffmanStatistics *m_pDCStatistics[4];
  class HuffmanStatistics *m_pACStatistics[4];
  // Scan parameters.
  UBYTE                    m_ucScanStart;
  UBYTE                    m_ucScanStop;
  UBYTE                    m_ucLowBit;
  //
  // Measure data or encode?
  bool                     m_bMeasure;
  //
  // Encode a differential scan?
  bool                     m_bDifferential;
  //
  // Encode a residual scan?
  bool                     m_bResidual;
  //
  // Progressive mode?
  bool                     m_bProgressive;
  //
  // Large range DCT mode?
  bool                     m_bLargeRange;
  //
  // Encode a single huffman block
  void EncodeBlock(const LONG *block,
                   class HuffmanCoder *dc,class HuffmanCoder *ac,
                   LONG &prevdc,UWORD &skip);
  //
  // Decode a single huffman block.
  void DecodeBlock(LONG *block,
                   class HuffmanDecoder *dc,class HuffmanDecoder *ac,
                   LONG &prevdc,UWORD &skip);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final);
  //
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
  // Make a block statistics measurement on the source data.
  void MeasureBlock(const LONG *block,
                    class HuffmanStatistics *dc,class HuffmanStatistics *ac,
                    LONG &prevdc,UWORD &skip);
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  // Code any run of zero blocks here. This is only valid in
  // the progressive mode.
  void CodeBlockSkip(class HuffmanCoder *ac,UWORD &skip);
  //
  //
public:
  // Create a sequential scan. The highbit is always ignored as this is
  // a valid setting for progressive only
  SequentialScan(class Frame *frame,class Scan *scan,UBYTE start,UBYTE stop,
                 UBYTE lowbit,UBYTE highbit,
                 bool differential = false,bool residual = false,bool largerange = false);
  //
  ~SequentialScan(void);
  // 
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding 
  virtual void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
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
