/*
**
** Represents the scan including the scan header for the
** arithmetic coding procedure for follow-up refinement
** scans.
**
** $Id: acrefinementscan.hpp,v 1.24 2015/10/13 16:56:50 thor Exp $
**
*/

#ifndef CODESTREAM_ACREFINEMENTSCAN_HPP
#define CODESTREAM_ACREFINEMENTSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "coding/qmcoder.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/entropyparser.hpp"
///

/// Forwards
class Tables;
class ByteStream;
class DCT;
class Frame;
struct RectangleRequest;
class BufferCtrl;
class BlockCtrl;
///

/// class ACRefinementScan
class ACRefinementScan : public EntropyParser {
  //
#if ACCUSOFT_CODE
  //
  // The QM coder doing the main work here.
  class QMCoder            m_Coder;
  //
  // Scan positions.
  ULONG                    m_ulX[4];
  //
  // Context information
  struct QMContextSet {
    //
    // The AC Coding context set.
    struct DCContextZeroSet {
      QMContext S0,SE,SC;
      //
      // Initialize
#ifdef DEBUG_QMCODER  
      void Init(int i)
      {
	char string[5] = "s000";
	string[2] = (i / 10) + '0';
	string[3] = (i % 10) + '0';
	S0.Init(string);
	string[1] = 'e';
	SE.Init(string);
	string[2] = 'c';
	SC.Init(string);
      }
#else
      void Init(void)
      {
	S0.Init();
	SE.Init();
	SC.Init();
      }
#endif
      // Entry #0 is not used by regular JPEG coding,
      // only by residual refinement.
    } ACZero[64];
    //
    // The uniform context.
    QMContext Uniform;
    //
    // Initialize the full beast.
    void Init(void)
    {
      for(int i = 0;i < 64;i++) {
#ifdef DEBUG_QMCODER
	ACZero[i].Init(i);
#else
	ACZero[i].Init();
#endif
      }
#ifdef DEBUG_QMCODER
      Uniform.Init(QMCoder::Uniform_State,"uni ");
#else
      Uniform.Init(QMCoder::Uniform_State);
#endif
    }
  } m_Context;
  //
  // 
protected:
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockCtrl            *m_pBlockCtrl;
  //
  // Scan parameters.
  UBYTE                       m_ucScanStart;
  UBYTE                       m_ucScanStop;
  UBYTE                       m_ucLowBit;
  UBYTE                       m_ucHighBit;
  //  
  // Only here for the hidden scan to look at.
  // Always false.
  bool                        m_bMeasure;
  // 
  // Encode a residual scan?
  bool                        m_bResidual;
  //
  // Encode a single block
  void EncodeBlock(const LONG *block);
  //
  // Decode a single block.
  void DecodeBlock(LONG *block);
  //
#endif
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final);
  //
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
private:
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  //
public:
  // Create an AC coded refinement scan. The differential flag is always
  // ignored, so is the residual flag.
  ACRefinementScan(class Frame *frame,class Scan *scan,UBYTE start,UBYTE stop,
		   UBYTE lowbit,UBYTE highbit,
		   bool differential = false,bool residual = false);
  //
  ~ACRefinementScan(void);
  // 
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding 
  virtual void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Measure scan statistics. Not implemented here since it is not
  // required. The AC coder is adaptive.
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
