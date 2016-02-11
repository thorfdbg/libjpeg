/*
**
** Represents the lossless scan - lines are coded directly with predictive
** coding.
**
** $Id: losslessscan.hpp,v 1.33 2014/11/16 15:49:58 thor Exp $
**
*/

#ifndef CODESTREAM_LOSSLESSSCAN_HPP
#define CODESTREAM_LOSSLESSSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bitstream.hpp"
#include "codestream/predictivescan.hpp"
#include "tools/line.hpp"
///

/// Forwards
class Frame;
class LineCtrl;
class ByteStream;
class HuffmanCoder;
class HuffmanDecoder;
class HuffmanStatistics;
class LineBitmapRequester;
class LineBuffer;
class LineAdapter;
class Scan;
class PredictorBase;
///

/// class LosslessScan
// A lossless scan creator.
class LosslessScan : public PredictiveScan {
  //
#if ACCUSOFT_CODE
  class HuffmanDecoder      *m_pDCDecoder[4];
  //
  // Ditto for the encoder
  class HuffmanCoder        *m_pDCCoder[4];
  //
  // And for measuring the statistics.
  class HuffmanStatistics   *m_pDCStatistics[4];
  //
  // The bitstream for bit-IO
  BitStream<false>           m_Stream;
  // 
  // Only measuring the statistics.
  bool                       m_bMeasure;
  //
#endif
  // This is actually the true MCU-parser, not the interface that reads
  // a full line.
  void ParseMCU(struct Line **prev,struct Line **top);
  //
  // The actual MCU-writer, write a single group of pixels to the stream,
  // or measure their statistics.
  void WriteMCU(struct Line **prev,struct Line **top);
  //
  // The actual MCU-writer, write a single group of pixels to the stream,
  // or measure their statistics.
  void MeasureMCU(struct Line **prev,struct Line **top);
   // 
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final); 
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
public:
  LosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,
	       bool differential = false);
  //
  virtual ~LosslessScan(void);
  // 
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding
  virtual void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Start the measurement run for the optimized
  // huffman encoder.
  virtual void StartMeasureScan(class BufferCtrl *ctrl);
  //
  // Start a MCU scan. Returns true if there are more rows. False otherwise.
  // Note that we emulate here that MCUs are multiples of eight lines high
  // even though from a JPEG perspective a MCU is a single pixel in the
  // lossless coding case.
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
