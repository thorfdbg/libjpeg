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
** This codes the differential scan for DCT-based huffman differential
** frames in the hierarchical process.
**
** $Id: differentialscan.cpp,v 1.8 2012-07-20 23:49:08 thor Exp $
**
*/

/// Includes
#include "codestream/differentialscan.hpp"
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
///

/// DifferentialScan::DifferentialScan
DifferentialScan::DifferentialScan(class Frame *frame,class Scan *scan,
				   UBYTE start,UBYTE stop,UBYTE lowbit)
  : EntropyParser(frame,scan), m_pBlockCtrl(NULL),
    m_ucScanStart(start), m_ucScanStop(stop), m_ucLowBit(lowbit)
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

/// DifferentialScan::~DifferentialScan
DifferentialScan::~DifferentialScan(void)
{
}
///

/// DifferentialScan::StartParseScan
void DifferentialScan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
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
    m_ulX[i]           = 0;
    m_usSkip[i]        = 0;
  }

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);

  m_Stream.OpenForRead(io);
}
///

/// DifferentialScan::StartWriteScan
void DifferentialScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
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
    m_ulX[i]           = 0;
    m_usSkip[i]        = 0;
  }
  m_bMeasure = false;

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
  
  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io);

}
///

/// DifferentialScan::StartMeasureScan
// Measure scan statistics.
void DifferentialScan::StartMeasureScan(class BufferCtrl *ctrl)
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
    m_ulX[i]             = 0;
    m_usSkip[i]          = 0;
  }
  m_bMeasure = true;

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
}
///

/// DifferentialScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool DifferentialScan::StartMCURow(void)
{
  bool more = m_pBlockCtrl->StartMCUQuantizerRow(m_pScan);

  for(int i = 0;i < m_ucCount;i++) {
    m_ulX[i]   = 0;
  }

  return more;
}
///

/// DifferentialScan::Flush
// Flush the remaining bits out to the stream on writing.
void DifferentialScan::Flush(void)
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
  if (!m_bMeasure)
    m_Stream.Flush();
} 
///

/// DifferentialScan::Restart
// Restart the parser at the next restart interval
void DifferentialScan::Restart(void)
{
  for(int i = 0; i < m_ucCount;i++) {
    m_usSkip[i]        = 0;
  }

  m_Stream.OpenForRead(m_Stream.ByteStreamOf());
}
///

/// DifferentialScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool DifferentialScan::WriteMCU(void)
{ 
  bool more = true;
  int c;

  assert(m_pBlockCtrl);
  
  BeginWriteMCU(m_Stream.ByteStreamOf());

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp           = m_pComponent[c];
    class QuantizedRow *q           = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    class HuffmanCoder *dc          = m_pDCCoder[c];
    class HuffmanCoder *ac          = m_pACCoder[c];
    class HuffmanStatistics *dcstat = m_pDCStatistics[c];
    class HuffmanStatistics *acstat = m_pACStatistics[c];
    UWORD &skip                     = m_usSkip[c];
    UBYTE mcux                      = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    UBYTE mcuy                      = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG xmin                      = m_ulX[c];
    ULONG xmax                      = xmin + mcux;
    ULONG x,y; 
    if (xmax >= q->WidthOf()) {
      more     = false;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *block,dummy[64];
	if (q && x < q->WidthOf()) {
	  block  = q->BlockAt(x)->m_Data;
	} else {
	  block  = dummy;
	  memset(dummy ,0,sizeof(dummy));
	}
#if HIERARCHICAL_HACK
	// A nice hack for the hierarchical scan: If this is not the last frame
	// in the hierarchy, remove all coefficients below the diagonal to allow a
	// fast "EOB", they can be encoded by the level above.
	if (m_pFrame->NextOf()) {
	  LONG i,j;
	  for(j = 0;j < 8;j++) {
	    for(i = 0;i < 8;i++) {
	      if (i+j > 4) {
		block[i + (j << 3)] = 0;
	      }
	    }
	  }
	}
#endif
	if (m_bMeasure) {
	  MeasureBlock(block,dcstat,acstat,skip);
	} else {
	  EncodeBlock(block,dc,ac,skip);
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

/// DifferentialScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool DifferentialScan::ParseMCU(void)
{
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  bool valid = BeginReadMCU(m_Stream.ByteStreamOf());
  
  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pComponent[c];
    class QuantizedRow *q    = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    class HuffmanDecoder *dc = m_pDCDecoder[c];
    class HuffmanDecoder *ac = m_pACDecoder[c];
    UWORD &skip              = m_usSkip[c];
    UBYTE mcux               = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    UBYTE mcuy               = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG xmin               = m_ulX[c];
    ULONG xmax               = xmin + mcux;
    ULONG x,y;
    if (xmax >= q->WidthOf()) {
      more     = false;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *block,dummy[64];
	if (q && x < q->WidthOf()) {
	  block  = q->BlockAt(x)->m_Data;
	} else {
	  block  = dummy;
	}
	if (valid) {
	  DecodeBlock(block,dc,ac,skip);
	} else {
	  for(UBYTE i = m_ucScanStart;i <= m_ucScanStop;i++) {
	    block[i] = 0;
	  }
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

/// DifferentialScan::MeasureBlock
// Make a block statistics measurement on the source data.
void DifferentialScan::MeasureBlock(const LONG *block,
				    class HuffmanStatistics *dc,class HuffmanStatistics *ac,
				    UWORD &skip)
{ 
  // DC coding
  if (m_ucScanStart == 0) {
    LONG  diff;
    UBYTE symbol = 0;
    // DPCM coding of the DC coefficient: Not for differential, this uses direct coding.
    diff   = block[0] >> m_ucLowBit; // Actually, only correct for two's complement machines...

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


/// DifferentialScan::CodeBlockSkip
// Code any run of zero blocks here. This is only valid in
// the progressive mode.
void DifferentialScan::CodeBlockSkip(class HuffmanCoder *ac,UWORD &skip)
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

/// DifferentialScan::EncodeBlock
// Encode a single huffman block
void DifferentialScan::EncodeBlock(const LONG *block,
				   class HuffmanCoder *dc,class HuffmanCoder *ac,
				   UWORD &skip)
{
  // DC coding
  if (m_ucScanStart == 0) {
    UBYTE symbol = 0;
    LONG diff;
    //
    // DPCM coding of the DC coefficient. Actually not, coded directly.
    diff   = block[0] >> m_ucLowBit; // Actually, only correct for two's complement machines...

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

/// DifferentialScan::DecodeBlock
// Decode a single huffman block.
void DifferentialScan::DecodeBlock(LONG *block,
				   class HuffmanDecoder *dc,class HuffmanDecoder *ac,
				   UWORD &skip)
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
    // No DPCM coding, just direct coding.
    block[0] = diff << m_ucLowBit; // point transformation
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
	    JPG_THROW(MALFORMED_STREAM,"DifferentialScan::DecodeBlock",
		      "AC coefficient decoding out of sync");
	  block[DCT::ScanOrder[k]] = diff << m_ucLowBit; // Point transformation.
	  k++;
	}
      } while(k <= m_ucScanStop);
    }
  }
}
///

/// DifferentialScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void DifferentialScan::WriteFrameType(class ByteStream *io)
{
  if (m_ucScanStart > 0 || m_ucScanStop < 63 || m_ucLowBit) {
    // is differential progressive.
    io->PutWord(0xffc6);
  } else {
    io->PutWord(0xffc5); // differential sequential
  }
}
///

