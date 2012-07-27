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
** This scan type decodes the coding residuals and completes an image
** into a lossless image.
**
** $Id: residualhuffmanscan.cpp,v 1.10 2012-07-27 08:08:33 thor Exp $
**
*/

/// Includes
#include "codestream/residualhuffmanscan.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "marker/residualmarker.hpp"
#include "marker/huffmantable.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
#include "codestream/rectanglerequest.hpp"
#include "std/assert.hpp"
#include "tools/traits.hpp"
#include "control/blockbuffer.hpp"
#include "control/blockbitmaprequester.hpp"
#include "coding/actemplate.hpp"
#include "marker/actable.hpp"
#include "control/blocklineadapter.hpp"
#include "io/memorystream.hpp"
///

/// ResidualHuffmanScan::m_ucCodingClass
// The mapping of Hadamard bands to coding classes.
const UBYTE ResidualHuffmanScan::m_ucCodingClass[64] = 
  { 0,1,1,1,1,1,1,1,
    1,1,1,2,2,2,3,3,
    1,1,1,2,2,2,3,3,
    1,2,2,3,3,3,3,3,
    1,2,2,3,3,3,3,3,
    1,2,2,3,3,3,3,3,
    1,3,3,3,3,3,3,3,
    1,3,3,3,3,3,3,3};
///

/// ResidualHuffmanScan::ResidualHuffmanScan
ResidualHuffmanScan::ResidualHuffmanScan(class Frame *frame,class Scan *scan)
  : EntropyParser(frame,scan), m_pTable(NULL), m_pulX(NULL), m_pBlockCtrl(NULL),
    m_pResidualBuffer(NULL), m_Helper(frame), 
    m_plDC(NULL), m_plN(NULL), m_plB(NULL), m_bHadamard(false)
{
  m_ucCount = frame->DepthOf();
  memset(m_pDecoder,0,sizeof(m_pDecoder));
  memset(m_pCoder,0,sizeof(m_pCoder));
  memset(m_pStatistics,0,sizeof(m_pCoder));
}
///

/// ResidualHuffmanScan::~ResidualHuffmanScan
ResidualHuffmanScan::~ResidualHuffmanScan(void)
{
  if (m_pulX) m_pEnviron->FreeMem(m_pulX,sizeof(ULONG) * m_ucCount);
  if (m_plDC) m_pEnviron->FreeMem(m_plDC,sizeof( LONG) * m_ucCount);
  if (m_plN)  m_pEnviron->FreeMem(m_plN ,sizeof( LONG) * m_ucCount);
  if (m_plB)  m_pEnviron->FreeMem(m_plB ,sizeof( LONG) * m_ucCount);

  delete m_pResidualBuffer;
  delete m_pTable;
}
///

/// ResidualHuffmanScan::InitStatistics
// Init the counters for the statistical measurement.
void ResidualHuffmanScan::InitStatistics(void)
{
  int i;

  m_bHadamard = m_pFrame->TablesOf()->ResidualDataOf()->isHadamardEnabled();

  if (m_pulX == NULL) m_pulX = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);

  for(i = 0;i < m_ucCount;i++) {
      m_pulX[i]         = 0;
  }

  if (m_bHadamard) {
    if (m_plDC == NULL) m_plDC = ( LONG *)m_pEnviron->AllocMem(sizeof( LONG) * m_ucCount);
    if (m_plN  == NULL) m_plN  = ( LONG *)m_pEnviron->AllocMem(sizeof( LONG) * m_ucCount);
    if (m_plB  == NULL) m_plB  = ( LONG *)m_pEnviron->AllocMem(sizeof( LONG) * m_ucCount);
    
    for(i = 0;i < m_ucCount;i++) {
      m_plDC[i]         = 0;
      m_plN[i]          = 0;
      m_plB[i]          = 0;
    }
  }
}
///

/// ResidualHuffmanScan::StartParseScan
void ResidualHuffmanScan::StartParseScan(class ByteStream *,class BufferCtrl *ctrl)
{ 
  int i;
  
  class ResidualMarker *marker = m_pFrame->TablesOf()->ResidualDataOf();

  assert(marker);
  
  if (m_pTable == NULL)
    m_pTable = new(m_pEnviron) class HuffmanTable(m_pEnviron);

  InitStatistics();
 
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(NULL); 

  {
    class ByteStream *stream = marker->StreamOf();
    LONG in = stream->GetWord();
    if (in != 0xffc4) 
      JPG_THROW(MALFORMED_STREAM,"ResidualHuffmanScan::StartParseScan",
		"expected a DHT marker in the lossless side channel");
    m_pTable->ParseMarker(stream);
    in = stream->GetWord();
    if (in != 0xffda)
      JPG_THROW(MALFORMED_STREAM,"ResidualHuffmanScan::StartParseScan",
		"expected a SOS marker in fromt of the entropy coded segment");
    //
    // The IO comes actually from the marker, not from the regular IO
    m_Stream.OpenForRead(stream);
  } 
  //
  // Build now the Huffman tables
  for(i = 0;i < 4;i++) {
    m_pDecoder[i]      = m_pTable->DCTemplateOf(i)->DecoderOf();
    m_pDecoder[i+4]    = m_pTable->ACTemplateOf(i)->DecoderOf();
    m_pCoder[i]        = NULL;
    m_pCoder[i+4]      = NULL;
    m_pStatistics[i]   = NULL;
    m_pStatistics[i+4] = NULL;
  }
}
///

/// ResidualHuffmanScan::StartWriteScan
void ResidualHuffmanScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  int i;

  if (m_pTable == NULL) {
    m_pTable = new(m_pEnviron) class HuffmanTable(m_pEnviron);
    for(i = 0;i < 4;i++) {
      m_pTable->DCTemplateOf(i)->InitDCLuminanceDefault();
      m_pTable->ACTemplateOf(i)->InitDCLuminanceDefault();
    }
  } else {
    m_pTable->AdjustToStatistics();
  }

  InitStatistics();

  for(i = 0;i < 4;i++) {
    m_pDecoder[i]      = NULL;
    m_pDecoder[i+4]    = NULL;
    m_pCoder[i]        = m_pTable->DCTemplateOf(i)->EncoderOf();
    m_pCoder[i+4]      = m_pTable->ACTemplateOf(i)->EncoderOf();
    m_pStatistics[i]   = NULL;
    m_pStatistics[i+4] = NULL;
  }
  m_bMeasure           = false;

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(NULL);

  //
  // Prepare the residual buffer for collecting the output stream data.
  assert(m_pResidualBuffer == NULL);
  m_pResidualBuffer = new(m_pEnviron) class MemoryStream(m_pEnviron,4096);
  m_pResidualBuffer->PutWord(0xffc4);
  m_pTable->WriteMarker(m_pResidualBuffer);
  m_pResidualBuffer->PutWord(0xffda);
  //
  // Where the data actually goes to.
  m_pTarget         = io;
  m_Stream.OpenForWrite(m_pResidualBuffer);
}
///

/// ResidualHuffmanScan::StartMeasureScan
// Measure scan statistics.
void ResidualHuffmanScan::StartMeasureScan(class BufferCtrl *ctrl)
{
  int i;

  if (m_pTable == NULL)
    m_pTable = new(m_pEnviron) class HuffmanTable(m_pEnviron);

  InitStatistics();

  for(i = 0;i < 4;i++) {
    m_pDecoder[i]      = NULL;
    m_pDecoder[i+4]    = NULL;
    m_pCoder[i]        = NULL;
    m_pCoder[i+4]      = NULL;
    m_pStatistics[i]   = m_pTable->DCTemplateOf(i)->StatisticsOf();
    m_pStatistics[i+4] = m_pTable->ACTemplateOf(i)->StatisticsOf();
  }

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(NULL);

  //
  // Just ignore, but let pass.
  m_bMeasure = true;
}
///

/// ResidualHuffmanScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ResidualHuffmanScan::StartMCURow(void)
{
  bool more = m_pBlockCtrl->StartMCUResidualRow();

  for(int i = 0;i < m_ucCount;i++) {
    m_pulX[i]   = 0;
  }

  return more;
}
///

/// ResidualHuffmanScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool ResidualHuffmanScan::WriteMCU(void)
{ 
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pFrame->ComponentOf(c); // Yes, really all of them!
    class QuantizedRow *r    = m_pBlockCtrl->CurrentResidualRow(c);
    UBYTE mcux               = comp->MCUWidthOf();
    UBYTE mcuy               = comp->MCUHeightOf();
    ULONG xmin               = m_pulX[c];
    ULONG xmax               = xmin + mcux;
    ULONG x,y; 
    if (xmin >= r->WidthOf()) {
      more      = false;
      continue;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *rblock,rdummy[64];
	if (r && x < r->WidthOf()) {
	  rblock = r->BlockAt(x)->m_Data;
	} else {
	  rblock = rdummy;
	  memset(rdummy,0,sizeof(rdummy));
	}
	  EncodeBlock(rblock,c);
      }
      if (r) r = r->NextOf();
    }
    // Done with this component, advance the block.
    m_pulX[c] = xmax;
  }

  return more;
}
///

/// ResidualHuffmanScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool ResidualHuffmanScan::ParseMCU(void)
{
  bool more = true;
  int c;

  assert(m_pBlockCtrl);

  for(c = 0;c < m_ucCount;c++) {
    class Component *comp    = m_pFrame->ComponentOf(c); // Yes, really all!
    class QuantizedRow *r    = m_pBlockCtrl->CurrentResidualRow(c);
    UBYTE mcux               = comp->MCUWidthOf();
    UBYTE mcuy               = comp->MCUHeightOf();
    ULONG xmin               = m_pulX[c];
    ULONG xmax               = xmin + mcux;
    ULONG x,y;
    if (xmin >= r->WidthOf()) {
      more     = false;
      continue;
    }
    for(y = 0;y < mcuy;y++) {
      for(x = xmin;x < xmax;x++) {
	LONG *rblock,dummy[64];
	if (r && x < r->WidthOf()) {
	  rblock = r->BlockAt(x)->m_Data;
	} else {
	  rblock = dummy;
	}
	DecodeBlock(rblock,c);
      }
      if (r) r = r->NextOf();
    }
    // Done with this component, advance the block.
    m_pulX[c] = xmax;
  }

  return more;
}
///

/// ResidualHuffmanScan::EncodeBlock
// Encode a single huffman block
void ResidualHuffmanScan::EncodeBlock(const LONG *residual,UBYTE comp)
{ 
  int k;

  if (m_bHadamard) {
    LONG dcnew    = residual[0] - m_plDC[comp];
    ErrorAdaption(residual[0],comp);
    
    //
    // Coding of the residual.
    // First try a very simple golomb code. Later more, probably runlength coding.
    for(k = 0;k < 64;k++) {
      LONG data = (k == 0)?(dcnew):(residual[k]);
      int     s = (m_ucCodingClass[k] << 1) + ((comp > 0)?(1):(0));
      UBYTE sym  = 0;
      LONG  dabs = (data >= 0)?(data):(-data);
      LONG  lim  = 1;
      while(dabs >= lim) {
	sym  += 2;
	lim <<= 1;
      }
      // In exceptional cases, the difference can even be larger than half the range!
      //assert(sym < 33);
      if (data > 0)
	sym--;
      if (m_bMeasure) {
	m_pStatistics[s]->Put(sym);
      } else {
	// Symbol 0 : zero
	// Symbol 1 : one
	// Symbol 2 : -one
	// Symbol 3 : two, or three
	// Symbol 4 : -two, or -three...
	m_pCoder[s]->Put(&m_Stream,sym);
	if (sym >= 3) {
	  UBYTE bits = (sym - 1) >> 1; // 3->2->1, 4->3->1
	  m_Stream.Put(bits,dabs);
	}
      }
    }
  } else { 
    for(k = 0;k < 64;k++) {
      LONG data = residual[k];
      LONG left = (k & 7 )?residual[k-1]:0;
      LONG top  = (k >> 3)?residual[k-8]:0;
      LONG ltop = ((k & 7) && (k >> 3))?residual[k-9]:0;
      LONG delta = 1;
      int s = 0;
      
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
      
      UBYTE sym  = 0;
      LONG  dabs = (data >= 0)?(data):(-data);
      LONG  lim  = 1;
      while(dabs >= lim) {
	sym  += 2;
	lim <<= 1;
      }
      // In exceptional cases, the difference can even be larger than half the range!
      assert(sym < 33);
      if (data > 0)
	sym--;
      if (m_bMeasure) {
	m_pStatistics[s]->Put(sym);
      } else {
	// Symbol 0 : zero
	// Symbol 1 : one
	// Symbol 2 : -one
	// Symbol 3 : two, or three
	// Symbol 4 : -two, or -three...
	m_pCoder[s]->Put(&m_Stream,sym);
	if (sym >= 3) {
	  UBYTE bits = (sym - 1) >> 1; // 3->2->1, 4->3->1
	  m_Stream.Put(bits,dabs);
	}
      }
    }
  }
}
///

/// ResidualHuffmanScan::DecodeBlock
// Decode a single huffman block.
void ResidualHuffmanScan::DecodeBlock(LONG *residual,UBYTE comp)
{
  int k;
  //
  if (m_bHadamard) {
    for(k = 0;k < 64;k++) {
      int     s = (m_ucCodingClass[k] << 1) + ((comp > 0)?(1):(0)); 
      UBYTE sym = m_pDecoder[s]->Get(&m_Stream);
      LONG dabs = 0;
      if (sym) {
	UBYTE bits = (sym - 1) >> 1; // 3->2->1, 4->3->1
	dabs = (1 << bits);
	if (bits > 0) {
	  dabs |= m_Stream.Get(bits);
	}
	if ((sym & 1) == 0) {
	  dabs = -dabs;
	}
      }
      residual[k] = dabs;
    } 
    
    residual[0] += m_plDC[comp]; 
    ErrorAdaption(residual[0],comp);
  } else { 
    for(k = 0;k < 64;k++) { 
      LONG left = (k & 7 )?residual[k-1]:0;
      LONG top  = (k >> 3)?residual[k-8]:0;
      LONG ltop = ((k & 7) && (k >> 3))?residual[k-9]:0;
      LONG delta = 1;
      int s = 0;
    
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
      
      UBYTE sym = m_pDecoder[s]->Get(&m_Stream);
      LONG dabs = 0;
      if (sym) {
	UBYTE bits = (sym - 1) >> 1; // 3->2->1, 4->3->1
	dabs = (1 << bits);
	if (bits > 0) {
	  dabs |= m_Stream.Get(bits);
	}
	if ((sym & 1) == 0) {
	  dabs = -dabs;
	}
      }
      residual[k] = dabs;
    }
  }
}
///

/// ResidualHuffmanScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ResidualHuffmanScan::WriteFrameType(class ByteStream *io)
{
  // This scan doesn't have a frame type as it extends all following scans,
  // so let the following scan do the work.
  assert(m_pScan && m_pScan->NextOf());

  m_pScan->NextOf()->WriteFrameType(io);
}
///

/// ResidualHuffmanScan::Flush
// Flush the remaining bits out to the stream on writing.
void ResidualHuffmanScan::Flush(void)
{  
  if (m_bMeasure == false) {
    class ResidualMarker *marker = m_pFrame->TablesOf()->ResidualDataOf();
    //
    assert(marker);
    //
    // Terminate the arithmetic coding.
    m_Stream.Flush();
    //
    // Write out the buffered data as a series of markers.
    marker->WriteMarker(m_pTarget,m_pResidualBuffer);
    //
    delete m_pResidualBuffer;m_pResidualBuffer = NULL;
  }
}
///



