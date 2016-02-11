/*
**
** Represents the scan including the scan header.
**
** $Id: losslessscan.cpp,v 1.50 2015/03/25 08:45:43 thor Exp $
**
*/

/// Includes
#include "codestream/losslessscan.hpp"
#include "io/bytestream.hpp"
#include "control/linebuffer.hpp"
#include "control/linebitmaprequester.hpp"
#include "control/lineadapter.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "io/bitstream.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
#include "codestream/tables.hpp"
#include "codestream/predictorbase.hpp"
#include "tools/line.hpp"
///

/// LosslessScan::LosslessScan
LosslessScan::LosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,bool differential)
  : PredictiveScan(frame,scan,predictor,lowbit,differential)
{ 
#if ACCUSOFT_CODE
  for(int i = 0;i < 4;i++) {
    m_pDCDecoder[i]    = NULL;
    m_pDCCoder[i]      = NULL;
    m_pDCStatistics[i] = NULL;
  }
#endif
}
///

/// LosslessScan::~LosslessScan
LosslessScan::~LosslessScan(void)
{
}
///

/// LosslessScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void LosslessScan::WriteFrameType(class ByteStream *io)
{
#if ACCUSOFT_CODE
  if (m_bDifferential) {
    io->PutWord(0xffc7); // differential lossless sequential
  } else {
    io->PutWord(0xffc3); // lossless sequential
  }
#else
  NOREF(io);
#endif
}
///

/// LosslessScan::StartParseScan
void LosslessScan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCDecoder[i]       = m_pScan->DCHuffmanDecoderOf(i);
  }
  
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  m_Stream.OpenForRead(io,chk);
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"LosslessScan::StartParseScan",
	    "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// LosslessScan::StartWriteScan
void LosslessScan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCCoder[i]       = m_pScan->DCHuffmanCoderOf(i);
    m_pDCStatistics[i]  = NULL;
  }
  
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan); 

  EntropyParser::StartWriteScan(io,chk,ctrl);
  
  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io,chk); 

  m_bMeasure = false;
#else
  NOREF(io);
  NOREF(chk);
  NOREF(ctrl);
  JPG_THROW(NOT_IMPLEMENTED,"LosslessScan::StartWriteScan",
	    "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// LosslessScan::StartMeasureScan
void LosslessScan::StartMeasureScan(class BufferCtrl *ctrl)
{
#if ACCUSOFT_CODE
  int i;

  FindComponentDimensions();
  
  for(i = 0;i < m_ucCount;i++) {
    m_pDCCoder[i]       = NULL;
    m_pDCStatistics[i]  = m_pScan->DCHuffmanStatisticsOf(i);
  }
 
  assert(ctrl->isLineBased());
  m_pLineCtrl = dynamic_cast<LineBuffer *>(ctrl);
  m_pLineCtrl->ResetToStartOfScan(m_pScan);
  
  m_Stream.OpenForWrite(NULL,NULL);
  
  m_bMeasure = true;
#else
  NOREF(ctrl);
#endif
}
///

/// LosslessScan::WriteMCU
// Write a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool LosslessScan::WriteMCU(void)
{
#if ACCUSOFT_CODE
  int i;
  struct Line *top[4],*prev[4];
  int lines      = 8; // total number of MCU lines processed.
  
  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    prev[i]         = m_pLineCtrl->PreviousLineOf(idx);
    m_ulX[i]        = 0;
    m_ulY[i]        = m_pLineCtrl->CurrentYOf(idx);
  }
  
  // Loop over lines and columns
  do {
    do {
      BeginWriteMCU(m_Stream.ByteStreamOf());    
      //
      if (m_bMeasure) {
	MeasureMCU(prev,top);
      } else {
	WriteMCU(prev,top);
      }
    } while(AdvanceToTheRight());
    //
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);
#endif
  return false;
}
///

/// LosslessScan::WriteMCU
// The actual MCU-writer, write a single group of pixels to the stream,
// or measure their statistics.
void LosslessScan::WriteMCU(struct Line **prev,struct Line **top)
{
#if ACCUSOFT_CODE
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanCoder *dc = m_pDCCoder[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    class PredictorBase *mcupred = m_pPredict[i];
    UBYTE ym = m_ucMCUHeight[i];
    LONG *lp = line->m_pData + m_ulX[i];
    LONG *pp = (pline)?(pline->m_pData + m_ulX[i]):(NULL);
    //
    // Write MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[i];
      do {
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v = pred->EncodeSample(lp,pp);
	//
	if (v == 0) {
	  dc->Put(&m_Stream,0);
	} else if (v == MIN_WORD) {
	  dc->Put(&m_Stream,16); // Do not append bits
	} else {
	  UBYTE symbol = 0;
	  do {
	    symbol++;
	    if (v > -(1 << symbol) && v < (1 << symbol)) {
	      dc->Put(&m_Stream,symbol);
	      if (v >= 0) {
		m_Stream.Put(symbol,v);
	      } else {
		m_Stream.Put(symbol,v - 1);
	      }
	      break;
	    }
	  } while(true);
	}
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
      } while(--xm && (lp++,pp++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + m_ulX[i],line = (line->m_pNext)?(line->m_pNext):(line),
		     lp = line->m_pData + m_ulX[i],mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}
///

/// LosslessScan::MeasureMCU
// The actual MCU-writer, write a single group of pixels to the stream,
// or measure their statistics. This here only measures the statistics
// to design an optimal Huffman table
void LosslessScan::MeasureMCU(struct Line **prev,struct Line **top)
{
#if ACCUSOFT_CODE
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanStatistics *dcstat = m_pDCStatistics[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    class PredictorBase *mcupred = m_pPredict[i];
    UBYTE ym = m_ucMCUHeight[i];
    LONG *lp = line->m_pData + m_ulX[i];
    LONG *pp = (pline)?(pline->m_pData + m_ulX[i]):(NULL);
    //
    //
    // Write MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[i];
      do {
	// Decode now the difference between the predicted value and
	// the real value.
	LONG v = pred->EncodeSample(lp,pp);
	//
	if (v == 0) {
	  dcstat->Put(0);
	} else if (v == -32768) {
	  dcstat->Put(16); // Do not append bits
	} else {
	  UBYTE symbol = 0;
	  do {
	    symbol++;
	    if (v > -(1 << symbol) && v < (1 << symbol)) {
	      dcstat->Put(symbol);
	      break;
	    }
	  } while(true);
	}
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
      } while(--xm && (lp++,pp++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + m_ulX[i],line = (line->m_pNext)?(line->m_pNext):(line),
		     lp = line->m_pData + m_ulX[i],mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}
///

/// LosslessScan::ParseMCU
// This is actually the true MCU-parser, not the interface that reads
// a full line.
void LosslessScan::ParseMCU(struct Line **prev,struct Line **top)
{ 
#if ACCUSOFT_CODE
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanDecoder *dc = m_pDCDecoder[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    UBYTE ym = m_ucMCUHeight[i];
    class PredictorBase *mcupred = m_pPredict[i];
    LONG *lp = line->m_pData + m_ulX[i];
    LONG *pp = (pline)?(pline->m_pData + m_ulX[i]):(NULL);
    //
    // Parse MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[i];
      do {
	LONG v;
	UBYTE symbol = dc->Get(&m_Stream);
	
	if (symbol == 0) {
	  v = 0;
	} else if (symbol == 16) {
	  v = -32768;
	} else {
	  LONG thre = 1L << (symbol - 1);
	  LONG diff = m_Stream.Get(symbol); // get the number of bits 
	  if (diff < thre) {
	    diff += (-1L << symbol) + 1;
	  }
	  v = diff;
	}
	//
	// Set the current pixel, do the inverse pointwise transformation.
	lp[0] = pred->DecodeSample(v,lp,pp);
	//
	// One pixel done. Proceed to the next in the MCU. Note that
	// the lines have been extended such that always a complete MCU is present.
      } while(--xm && (lp++,pp++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + m_ulX[i],line = (line->m_pNext)?(line->m_pNext):(line),
		     lp = line->m_pData + m_ulX[i],mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}
///

/// LosslessScan::ParseMCU
// Parse a single MCU in this scan. Actually, this is not quite true,
// as we write an entire group of eight lines of pixels, as a MCU is
// here a group of pixels. But it is more practical this way.
bool LosslessScan::ParseMCU(void)
{
#if ACCUSOFT_CODE
  int i;
  struct Line *top[4],*prev[4];
  int lines      = 8; // total number of MCU lines processed.
 
  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE idx       = comp->IndexOf();
    top[i]          = m_pLineCtrl->CurrentLineOf(idx);
    prev[i]         = m_pLineCtrl->PreviousLineOf(idx);
    m_ulX[i]        = 0;
    m_ulY[i]        = m_pLineCtrl->CurrentYOf(idx);
  }

  // Loop over lines and columns
  do {
    bool startofline = true;
    do {
      if (BeginReadMCU(m_Stream.ByteStreamOf())) {
	ParseMCU(prev,top);
      } else {
	// Only if this is not due to a DNL marker that has been detected.
	if (m_ulPixelHeight != 0 && !hasFoundDNL()) {
	  ClearMCU(top);
	} else if (!startofline) {
	  // The problem is here that the DNL marker might have been detected, even though decoding
	  // is not yet done completely. This may be because there are still just enough bits in the
	  // bitream present to run a single decode. Big Outch! Just continue decoding in this case.
	  ParseMCU(prev,top);
	} else break;
      }
      startofline = false;
    } while(AdvanceToTheRight());
    //
    // Advance to the next line.
  } while(AdvanceToTheNextLine(prev,top) && --lines);
#endif  
  return false; // no further blocks here.
}
///

/// LosslessScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool LosslessScan::StartMCURow(void)
{
#if ACCUSOFT_CODE
  return m_pLineCtrl->StartMCUQuantizerRow(m_pScan);
#else
  return false;
#endif
}
///

/// LosslessScan::Flush
// Flush the remaining bits out to the stream on writing.
void LosslessScan::Flush(bool)
{  
#if ACCUSOFT_CODE
  if (!m_bMeasure)
    m_Stream.Flush();

  PredictiveScan::FlushOnMarker();
#endif
}
///

/// LosslessScan::Restart
// Restart the parser at the next restart interval
void LosslessScan::Restart(void)
{ 
#if ACCUSOFT_CODE
  m_Stream.OpenForRead(m_Stream.ByteStreamOf(),m_Stream.ChecksumOf());

  PredictiveScan::RestartOnMarker();
#endif
}
///
