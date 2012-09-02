/*************************************************************************
** Copyright (c) 2003-2011 Accusoft/Pegasus                             **
** THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE                          **
**                                                                      **
** Written by Thomas Richter (THOR Software) for Accusoft/Pegasus       **
** All Rights Reserved                                                  **
*************************************************************************/
/*
**
** A sequential scan, also the first scan of a progressive scan,
** Huffman coded.
**
** $Id: sequentialscan.cpp,v 1.55 2012-02-06 16:53:54 thor Exp $
**
*/

/// Includes
#include "codestream/residualsequentialscan.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/rectanglerequest.hpp"
#include "dct/idct.hpp"
#include "dct/fdct.hpp"
#include "dct/sermsdct.hpp"
#include "std/assert.hpp"
#include "interface/bitmaphook.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
#include "tools/traits.hpp"
#include "control/blockbuffer.hpp"
#include "control/blockbitmaprequester.hpp"
#include "control/blocklineadapter.hpp"
#include "io/staticstream.hpp"
///

/// ResidualSequentialScan::ResidualSequentialScan
ResidualSequentialScan::ResidualSequentialScan(class Frame *frame,class Scan *scan,
			       UBYTE start,UBYTE stop,UBYTE lowbit)
  : EntropyParser(frame,scan), m_pBlockCtrl(NULL),
    m_ucScanStart(start), m_ucScanStop(stop), m_ucLowBit(lowbit), m_ucMaxError(0)
{
  m_ucCount = scan->ComponentsInScan();
  
  for(int i = 0;i < 4;i++) {
    m_pDCDecoder[i]    = NULL;
    m_pACDecoder[i]    = NULL;
    m_pDCCoder[i]      = NULL;
    m_pACCoder[i]      = NULL;
    m_pDCStatistics[i] = NULL;
    m_pACStatistics[i] = NULL;
  }
}
///

/// ResidualSequentialScan::~ResidualSequentialScan
ResidualSequentialScan::~ResidualSequentialScan(void)
{
}
///

/// ResidualSequentialScan::StartParseScan
void ResidualSequentialScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) {
    if (m_ucScanStart == 0) {
      m_pDCDecoder[i]  = m_pScan->DCHuffmanDecoderOf(i);
    } else {
      m_pDCDecoder[i]  = NULL; // not required, is AC only.
    }
    if (m_ucScanStop) {
      m_pACDecoder[i]  = m_pScan->ACHuffmanDecoderOf(i);
    } else {
      m_pACDecoder[i]  = NULL; // not required, is DC only.
    }
    m_lDC[i]           = 0; 
    m_ulX[i]           = 0;
    m_usSkip[i]        = 0;
  }

  m_Context.Init();
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);

  m_Stream.OpenForRead(io);
}
///

/// ResidualSequentialScan::StartWriteScan
void ResidualSequentialScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) {
    if (m_ucScanStart == 0) {
      m_pDCCoder[i]    = m_pScan->DCHuffmanCoderOf(i);
    } else {
      m_pDCCoder[i]    = NULL;
    }
    if (m_ucScanStop) {
      m_pACCoder[i]    = m_pScan->ACHuffmanCoderOf(i);
    } else {
      m_pACCoder[i]    = NULL;
    }
    m_pDCStatistics[i] = NULL;
    m_pACStatistics[i] = NULL;
    m_lDC[i]           = 0;
    m_ulX[i]           = 0;
    m_usSkip[i]        = 0;
  }
  m_bMeasure = false;

  m_Context.Init();
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
  
  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io);

}
///

/// ResidualSequentialScan::StartMeasureScan
// Measure scan statistics.
void ResidualSequentialScan::StartMeasureScan(class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) { 
    m_pDCCoder[i]        = NULL;
    m_pACCoder[i]        = NULL;
    if (m_ucScanStart == 0) {
      m_pDCStatistics[i] = m_pScan->DCHuffmanStatisticsOf(i);
    } else {
      m_pDCStatistics[i] = NULL;
    }
    if (m_ucScanStop) {
      m_pACStatistics[i] = m_pScan->ACHuffmanStatisticsOf(i);
    } else {
      m_pACStatistics[i] = NULL;
    }
    m_lDC[i]             = 0;
    m_ulX[i]             = 0;
    m_usSkip[i]          = 0;
  }
  m_bMeasure = true;

  m_Context.Init();
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
}
///

/// ResidualSequentialScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ResidualSequentialScan::StartMCURow(void)
{
  bool more = m_pBlockCtrl->StartMCUQuantizerRow(m_pScan);

  m_pBlockCtrl->StartMCUResidualRow(m_pScan);

  for(int i = 0;i < m_ucCount;i++) {
    m_ulX[i]   = 0;
  }

  return more;
}
///

/// ResidualSequentialScan::Flush
// Flush the remaining bits out to the stream on writing.
void ResidualSequentialScan::Flush(void)
{
  if (m_ucScanStart) {
    // Progressive, AC band. It looks wierd to code the remaining
    // block skips right here. However, AC bands in spectral selection
    // are always coded in isolated scans, thus only one component
    // per scan and no interleaving. Hence, no problem.
    assert(m_ucCount == 1);
    if (m_usSkip[0]) {
      // Flush out any pending block
      if (m_pACStatistics[0]) { // only count.
	UBYTE symbol = 0;
	while(m_usSkip[0] >= (1L << symbol))
	  symbol++;
	m_pACStatistics[0]->Put((symbol - 1) << 4);
	m_usSkip[0] = 0;
      } else {
	CodeBlockSkip(m_pACCoder[0],m_usSkip[0]);
      }
    }
  }
  m_Stream.Flush();
} 
///

/// ResidualSequentialScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool ResidualSequentialScan::WriteMCU(void)
{ 
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp           = m_pComponent[c];
    class QuantizedRow *q           = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    class QuantizedRow *r           = m_pBlockCtrl->CurrentResidualRow(comp->IndexOf());
    class HuffmanCoder *dc          = m_pDCCoder[c];
    class HuffmanCoder *ac          = m_pACCoder[c];
    class HuffmanStatistics *dcstat = m_pDCStatistics[c];
    class HuffmanStatistics *acstat = m_pACStatistics[c];
    LONG &prevdc                    = m_lDC[c];
    UWORD &skip                     = m_usSkip[c];
    UBYTE mcux                      = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    UBYTE mcuy                      = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG xmin                      = m_ulX[c];
    ULONG xmax                      = xmin + mcux;
    ULONG x,y; 
    if (xmin >= q->WidthOf()) {
      more     = false;
      continue;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *block,*rblock,dummy[64],rdummy[64];
	if (q && x < q->WidthOf()) {
	  assert(r);
	  block  = q->BlockAt(x)->m_Data;
	  rblock = r->BlockAt(x)->m_Data;
	} else {
	  block  = dummy;
	  rblock = rdummy;
	  memset(dummy ,0,sizeof(dummy) );
	  memset(rdummy,0,sizeof(rdummy));
	  block[0] = prevdc;
	}
	InjectResidual(block,rblock);
	if (m_bMeasure) {
	  MeasureBlock(block,dcstat,acstat,prevdc,skip);
	} else {
	  EncodeBlock(block,ac,dc,prevdc,skip);
	}
      }
      if (q) q = q->NextOf();
    }
    // Done with this component, advance the block.
    m_ulX[c] = xmax;
  }

  return more;
}
///

/// ResidualSequentialScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool ResidualSequentialScan::ParseMCU(void)
{
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pComponent[c];
    class QuantizedRow *q    = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    class QuantizedRow *r    = m_pBlockCtrl->CurrentResidualRow(comp->IndexOf());
    class HuffmanDecoder *dc = m_pDCDecoder[c];
    class HuffmanDecoder *ac = m_pACDecoder[c];
    UWORD &skip              = m_usSkip[c];
    LONG &prevdc             = m_lDC[c];
    UBYTE mcux               = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    UBYTE mcuy               = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG xmin               = m_ulX[c];
    ULONG xmax               = xmin + mcux;
    ULONG x,y;
    if (xmin >= q->WidthOf()) {
      more     = false;
      continue;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *block,*rblock,dummy[64];
	if (q && x < q->WidthOf()) {
	  assert(r);
	  block  = q->BlockAt(x)->m_Data;
	  rblock = r->BlockAt(x)->m_Data;
	} else {
	  block  = dummy;
	  rblock = dummy;
	}
	DecodeBlock(block,dc,ac,prevdc,skip);
	ExtractResidual(block,rblock);
      }
      if (q) q = q->NextOf();
    }
    // Done with this component, advance the block.
    m_ulX[c] = xmax;
  }

  return more;
}
///

/// ResidualSequentialScan::MeasureBlock
// Make a block statistics measurement on the source data.
void ResidualSequentialScan::MeasureBlock(const LONG *block,
				  class HuffmanStatistics *dc,class HuffmanStatistics *ac,
				  LONG &prevdc,UWORD &skip)
{ 
  // DC coding
  if (m_ucScanStart == 0) {
    LONG  diff;
    UBYTE symbol = 0;
    // DPCM coding of the DC coefficient.
    diff   = block[0] >> m_ucLowBit; // Actually, only correct for two's complement machines...
    diff  -= prevdc;
    prevdc = block[0] >> m_ucLowBit;

    if (diff) {
      do {
	symbol++;
	if (diff > -(1L << symbol) && diff < (1L << symbol)) {
	  dc->Put(symbol);
	  break;
	}
      } while(true);
    } else {
      dc->Put(0);
    }
  }
  
  // AC coding
  if (m_ucScanStop) {
    UBYTE symbol,run = 0;
    int k = (m_ucScanStart)?(m_ucScanStart):(1);
    
    do {
      LONG data = block[DCT::ScanOrder[k]]; 
      // Implement the point transformation. This is here a division, not
      // a shift (rounding is different for negative numbers).
      data = (data >= 0)?(data >> m_ucLowBit):(-((-data) >> m_ucLowBit));
      if (data == 0) {
	run++;
      } else {
	// Code (or compute the length of) any missing zero run.
	if (skip) {
	  UBYTE sksymbol = 0;
	  while(skip >= (1L << sksymbol))
	    sksymbol++;
	  ac->Put((sksymbol - 1) << 4);
	  skip = 0;
	}
	// First ensure that the run is at most 15, the largest cathegory.
	while(run > 15) {
	  ac->Put(0xf0); // r = 15 and s = 0
	  run -= 16;
	}
	symbol = 0;
	do {
	  symbol++;
	  if (data > -(1L << symbol) && data < (1L << symbol)) {
	    // Cathegory symbol, run length run
	    ac->Put(symbol | (run << 4));
	    break;
	  }
	} while(true);
	//
	// Run is over.
	run = 0;
      }
    } while(++k <= m_ucScanStop);
    
    // Is there still an open run? If so, code an EOB.
    if (run) {
      // In the progressive mode, absorb into the skip
      if (m_ucScanStart) {
	skip++;
	if (skip == MAX_WORD) {
	  ac->Put(0xe0); // symbol for maximum length
	  skip = 0;
	}
      } else {
	ac->Put(0x00);
      }
    }
  }
}
///


/// ResidualSequentialScan::CodeBlockSkip
// Code any run of zero blocks here. This is only valid in
// the progressive mode.
void ResidualSequentialScan::CodeBlockSkip(class HuffmanCoder *ac,UWORD &skip)
{  
  if (skip) {
    UBYTE symbol = 0;
    do {
      symbol++;
      if (skip < (1L << symbol)) {
	symbol--;
	assert(symbol <= 14);
	ac->Put(&m_Stream,symbol << 4);
	if (symbol)
	  m_Stream.Put(symbol,skip);
	skip = 0;
	return;
      }
    } while(true);
  }
}
///

/// ResidualSequentialScan::EncodeBlock
// Encode a single huffman block
void ResidualSequentialScan::EncodeBlock(const LONG *block,
					 class HuffmanCoder *dc,class HuffmanCoder *ac,
					 LONG &prevdc,UWORD &skip)
{
  // DC coding
  if (m_ucScanStart == 0) {
    UBYTE symbol = 0;
    LONG diff;
    //
    // DPCM coding of the DC coefficient.
    diff   = block[0] >> m_ucLowBit; // Actually, only correct for two's complement machines...
    diff  -= prevdc;
    prevdc = block[0] >> m_ucLowBit;

    if (diff) {
      do {
	symbol++;
	if (diff > -(1L << symbol) && diff < (1L << symbol)) {
	  dc->Put(&m_Stream,symbol);
	  if (diff >= 0) {
	    m_Stream.Put(symbol,diff);
	  } else {
	    m_Stream.Put(symbol,diff - 1);
	  }
	  break;
	}
      } while(true);
    } else {
      dc->Put(&m_Stream,0);
    }
  }
  
  // AC coding
  if (m_ucScanStop) {
    UBYTE symbol,run = 0;
    int k = (m_ucScanStart)?(m_ucScanStart):(1);

    do {
      LONG data = block[DCT::ScanOrder[k]];
      // Implement the point transformation. This is here a division, not
      // a shift (rounding is different for negative numbers).
      data = (data >= 0)?(data >> m_ucLowBit):(-((-data) >> m_ucLowBit));
      if (data == 0) {
	run++;
      } else {
	// Are there any skipped blocks we still need to code? Since this
	// block is none of them.
	if (skip)
	  CodeBlockSkip(ac,skip);
	//
	// First ensure that the run is at most 15, the largest cathegory.
	while(run > 15) {
	  ac->Put(&m_Stream,0xf0); // r = 15 and s = 0
	  run -= 16;
	}
	symbol = 0;
	do {
	  symbol++;
	  if (data > -(1L << symbol) && data < (1L << symbol)) {
	    // Cathegory symbol, run length run
	    ac->Put(&m_Stream,symbol | (run << 4));
	    if (data >= 0) {
	      m_Stream.Put(symbol,data);
	    } else {
	      m_Stream.Put(symbol,data - 1);
	    }
	    break;
	  }
	} while(true);
	//
	// Run is over.
	run = 0;
      }
    } while(++k <= m_ucScanStop);

    // Is there still an open run? If so, code an EOB in the regular mode.
    // If this is part of the (isolated) AC scan of the progressive JPEG,
    // check whether we could potentially accumulate this into a run of
    // zero blocks.
    if (run) {
      // Include in a block skip (or try to, rather).
      if (m_ucScanStart) {
	skip++;
	if (skip == MAX_WORD) // avoid an overflow, code now
	  CodeBlockSkip(ac,skip);
      } else {
	// In sequential mode, encode as EOB.
	ac->Put(&m_Stream,0x00);
      }
    }
  }
}
///

/// ResidualSequentialScan::DecodeBlock
// Decode a single huffman block.
void ResidualSequentialScan::DecodeBlock(LONG *block,
					 class HuffmanDecoder *dc,class HuffmanDecoder *ac,
					 LONG &prevdc,UWORD &skip)
{
  if (m_ucScanStart == 0) {
    // First DC level coding. If it is in the spectral selection.
    LONG diff   = 0;
    UBYTE value = dc->Get(&m_Stream);
    if (value > 0) {
      LONG v = 1 << (value - 1);
      diff   = m_Stream.Get(value);
      if (diff < v) {
	diff += (-1L << value) + 1;
      }
    }
    prevdc  += diff;
    block[0] = prevdc << m_ucLowBit; // point transformation
  }

  if (m_ucScanStop) {
    // AC coding. 
    if (skip > 0) {
      skip--; // Still blocks to skip
    } else {
      int k = (m_ucScanStart)?(m_ucScanStart):(1);

      do {
	UBYTE rs = ac->Get(&m_Stream);
	UBYTE r  = rs >> 4;
	UBYTE s  = rs & 0x0f;
	
	if (s == 0) {
	  if (r == 15) {
	    k += 16;
	  } else {
	    // A progressive EOB run.
	    skip  = 1 << r;
	    if (r) skip |= m_Stream.Get(r);
	    skip--; // this block is included in the count.
	    break;
	  }
	} else {
	  LONG diff;
	  LONG v = 1 << (s - 1);
	  k     += r;
	  diff   = m_Stream.Get(s);
	  if (diff < v) {
	    diff += (-1L << s) + 1;
	  }
	  if (k >= 64)
	    JPG_THROW(MALFORMED_STREAM,"ResidualSequentialScan::DecodeBlock",
		      "AC coefficient decoding out of sync");
	  block[DCT::ScanOrder[k]] = diff << m_ucLowBit; // Point transformation.
	  k++;
	}
      } while(k <= m_ucScanStop);
    }
  }
}
///

/// ResidualSequentialScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ResidualSequentialScan::WriteFrameType(class ByteStream *io)
{
  if (m_ucScanStart > 0 || m_ucScanStop < 63 || m_ucLowBit) {
    // is progressive.
    io->PutWord(0xffc2);
  } else {
    io->PutWord(0xffc1); // not baseline, but sequential. Could check that...
  }
}
///


/// ResidualResidualScan::EncodeResidualBlock
void ResidualSequentialScan::EncodeResidualBlock(const LONG *residual,ByteStream *target)
{ 
  int k;
  
  m_Coder.OpenForWrite(target);
  //
  // Coding of the residual.
  // First try a very simple golomb code. Later more, probably runlength coding.
  for(k = 0;k < 64;k++) {
    LONG data = residual[k];
    LONG left = (k & 7 )?residual[k-1]:0;
    LONG top  = (k >> 3)?residual[k-8]:0;
    LONG ltop = ((k & 7) && (k >> 3))?residual[k-9]:0;
    LONG delta = 1;
    int s = 0;
    
    /* NOTE: must implement round to zero */
    data /= m_ucMaxError + 1;
    left /= m_ucMaxError + 1;
    top  /= m_ucMaxError + 1;
    ltop /= m_ucMaxError + 1;
    
    if ((left > delta && ltop > delta && top < -delta)) {
      s = 1;
    } else if ((left < -delta && ltop < -delta && top > delta)) {
      s = 2;
    } else if ((left > delta && ltop < -delta && top < -delta)) {
      s = 3;
    } else if ((left < -delta && ltop > delta && top > delta)) {
      s = 4;
    } else if ((left > delta && top > delta && ltop > delta)) {
      s = 5;
    } else if ((left < -delta && top <-delta && ltop < -delta)) {
      s = 6;
    }
    
    if (data) {
      LONG sz;
      m_Coder.Put(m_Context.S0,true);
      // Sign coding.
      if (data < 0) {
	m_Coder.Put(m_Context.SS[s],true);
	sz = -data - 1;
      } else {
	m_Coder.Put(m_Context.SS[s],false);
	sz = data  - 1;
      }
      //
      // Code the magnitude.
      if (sz >= 1) {
	int  i = 0;
	LONG m = 2;
	m_Coder.Put(m_Context.SP,true);
	//
	// Magnitude category coding.
	while(sz >= m) {
	  m_Coder.Put(m_Context.X[i],true);
	  m <<= 1;
	  i++;
	} 
	// Terminate magnitude cathegory coding.
	m_Coder.Put(m_Context.X[i],false);
	//
	// Get the MSB to code.
	m >>= 1;
	// Refinement bits: Depend on the magnitude category.
	while((m >>= 1)) {
	  m_Coder.Put(m_Context.M[i],(m & sz)?(true):(false));
	}
      } else {
	m_Coder.Put(m_Context.SP,false);
      }
    } else {
      m_Coder.Put(m_Context.S0,false);
    }
  }

  m_Coder.Flush();
}
///

/// ResidualSequentialScan::DecodeResidualBlock
// Decode a single huffman block.
void ResidualSequentialScan::DecodeResidualBlock(LONG *residual,class ByteStream *source)
{
  int k;
  
  m_Coder.OpenForRead(source);

  //
  for(k = 0;k < 64;k++) { 
    LONG data;
    LONG left = (k & 7 )?residual[k-1]:0;
    LONG top  = (k >> 3)?residual[k-8]:0;
    LONG ltop = ((k & 7) && (k >> 3))?residual[k-9]:0;
    LONG delta = 1;
    int s = 0;
    
    /* NOTE: must implement round to zero */
    data /= m_ucMaxError + 1;
    left /= m_ucMaxError + 1;
    top  /= m_ucMaxError + 1;
    ltop /= m_ucMaxError + 1;
    
    if ((left > delta && ltop > delta && top < -delta)) {
      s = 1;
    } else if ((left < -delta && ltop < -delta && top > delta)) {
      s = 2;
    } else if ((left > delta && ltop < -delta && top < -delta)) {
      s = 3;
    } else if ((left < -delta && ltop > delta && top > delta)) {
      s = 4;
    } else if ((left > delta && top > delta && ltop > delta)) {
      s = 5;
    } else if ((left < -delta && top <-delta && ltop < -delta)) {
      s = 6;
    }
    
    if (m_Coder.Get(m_Context.S0)) {
      LONG sz;
      bool sign = m_Coder.Get(m_Context.SS[s]); // true if negative.
      //
      if (m_Coder.Get(m_Context.SP)) {
	int  i = 0;
	LONG m = 2;
	
	while(m_Coder.Get(m_Context.X[i])) {
	  m <<= 1;
	  i++;
	  if (m == 0) 
	    JPG_THROW(MALFORMED_STREAM,"ResidualScan::DecodeBlock",
		      "QMDecoder is out of sync");
	}
	//
	// Get the MSB to decode.
	m >>= 1;
	sz  = m;
	//
	// Refinement coding of remaining bits.
	while((m >>= 1)) {
	  if (m_Coder.Get(m_Context.M[i])) {
	    sz |= m;
	  }
	}
      } else {
	sz = 0;
      }

      if (sign) {
	data = (-sz - 1) * (m_ucMaxError + 1);
      } else {
	data = ( sz + 1) * (m_ucMaxError + 1);
      }
    } else {
      data = 0;
    }
    
    residual[k] = data;
  }
}
///

/// ResidualSequentialStream::InjectResidual
// Inject residuals into a block.
void ResidualSequentialScan::InjectResidual(LONG *block,const LONG *rblock)
{
  UBYTE buffer[16],*b;
  int k,s;
  class StaticStream side(m_pEnviron,buffer,sizeof(buffer));

  EncodeResidualBlock(rblock,&side);

  for(k = 0,s = 8,b = buffer;k < 64;k++) {
    s -= 2;
    block[k] = (block[k] << 2) | ((*b >> s) & 3);
    if (s == 0) {
      s = 8;b++;
    }
  }
}
///

/// ResidualSequentialStream::ExtractResidual
// Extract again the residuals from a block
void ResidualSequentialScan::ExtractResidual(LONG *block,LONG *rblock)
{
  UBYTE buffer[8],*b;
  int k,s;
  class StaticStream side(m_pEnviron,buffer,sizeof(buffer));

  for(k = 0,s = 8,b = buffer,*b = 0;k < 64;k++) {
    s -= 2;
    *b |= (block[k] & 3) << s;
    block[k] >>= 2;
    if (s == 0) {
      s = 8;
      b++;
      *b = 0;
    }
  }

  DecodeResidualBlock(rblock,&side);
}
///
