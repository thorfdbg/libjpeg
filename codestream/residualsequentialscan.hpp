/*************************************************************************
** Copyright (c) 2003-2011 Accusoft/Pegasus                             **
** THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE                          **
**                                                                      **
** Written by Thomas Richter (THOR Software) for Accusoft/Pegasus       **
** All Rights Reserved                                                  **
*************************************************************************/
/*
** A sequential scan, also the first scan of a progressive scan,
** Huffman coded.
**
** $Id: sequentialscan.hpp,v 1.38 2012-02-03 08:28:07 thor Exp $
**
*/

#ifndef CODESTREAM_RESIDUALSEQUENTIALSCAN_HPP
#define CODESTREAM_RESIDUALSEQUENTIALSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "io/bitstream.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/entropyparser.hpp"
#include "coding/qmcoder.hpp"
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
class BufferCtrl;
class LineAdapter;
class BitmapCtrl;
///

/// class ResidualSequentialScan
// A sequential scan, also the first scan of a progressive scan,
// Huffman coded.
class ResidualSequentialScan : public EntropyParser {
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
  //
  // Last DC value, required for the DPCM coder.
  LONG                     m_lDC[4];
  //
  // Number of blocks still to skip over.
  // This is only used in the progressive mode.
  UWORD                    m_usSkip[4];
  //
  // Scan positions.
  ULONG                    m_ulX[4];
  //
  // The bitstream from which we read the data.
  class BitStream          m_Stream;
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockBuffer          *m_pBlockCtrl; 
  //
  // Scan parameters.
  UBYTE                       m_ucScanStart;
  UBYTE                       m_ucScanStop;
  UBYTE                       m_ucLowBit;
  //
  // Measure data or encode?
  bool                        m_bMeasure;
  //
  // Contexts for the residual coder.
  struct QMContextSet {
    //
    // Magnitude context. Three are sufficient.
    QMContext M[17];
    QMContext X[17];
    QMContext S0,SP;
    QMContext SS[7];
    QMContext Uniform,SZ;
    //
    void Init(void)
    {
      for(int i = 0;i < 17;i++) {
        M[i].Init();
        X[i].Init();
      }
      S0.Init();
      SP.Init();
      for(int k = 0;k < 7;k++) {
        SS[k].Init();
      }
      SZ.Init();
      Uniform.Init(QMCoder::Uniform_State);
    }
  }                           m_Context;
  //
  // Entropy encoder for the residual data.
  class QMCoder               m_Coder;
  //
  // Maximum tolerable error. Currently zero.
  UBYTE                       m_ucMaxError;
  //
  // Encode the block residuals.
  void EncodeResidualBlock(const LONG *residual,ByteStream *target);
  //
  // Decode block residuals.
  void DecodeResidualBlock(LONG *residual,ByteStream *source);
  //
  // Inject residuals into a block.
  void InjectResidual(LONG *block,const LONG *rblock);
  //
  // Extract again the residuals from a block
  void ExtractResidual(LONG *block,LONG *rblock);
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
  virtual void Flush(void);
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
public:
  ResidualSequentialScan(class Frame *frame,class Scan *scan,
			 UBYTE start,UBYTE stop,UBYTE lowbit);
  //
  ~ResidualSequentialScan(void);
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
