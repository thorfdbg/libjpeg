/*
** A subsequent (refinement) scan of a progressive scan.
**
** $Id: refinementscan.hpp,v 1.24 2014/09/30 08:33:15 thor Exp $
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
class BlockCtrl;
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
  class BlockCtrl         *m_pBlockCtrl; 
  //
  // Scan parameters.
  UBYTE                    m_ucScanStart;
  UBYTE                    m_ucScanStop;
  UBYTE                    m_ucLowBit; // First bit (from LSB) to code in this scan
  UBYTE                    m_ucHighBit; // First bit *NOT* to code anymore.
  //
  // Encode a residual scan?
  bool                     m_bResidual;
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
