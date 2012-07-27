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
** $Id: residualscan.cpp,v 1.18 2012-07-27 08:08:33 thor Exp $
**
*/

/// Includes
#include "codestream/residualscan.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "marker/residualmarker.hpp"
#include "coding/quantizedrow.hpp"
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

/// Statistics
//#define PLOT_STATISTICS
#ifdef PLOT_STATISTICS
#include "std/stdio.hpp"
#define OFFSET (1 << 17)
static ULONG count[1 << 18][64][2];
#endif
///

/// ResidualScan::m_ucCodingClass
// The mapping of Hadamard bands to coding classes.
const UBYTE ResidualScan::m_ucCodingClass[64] = 
   {0,1,1,1,1,1,1,1,
    1,1,1,2,2,2,3,3,
    1,1,1,2,2,2,3,3,
    1,2,2,3,3,3,3,3,
    1,2,2,3,3,3,3,3,
    1,2,2,3,3,3,3,3,
    1,3,3,3,3,3,3,3,
    1,3,3,3,3,3,3,3};
const UBYTE ResidualScan::m_ucFineClass[64] = 
  { 0, 1, 1, 2, 2, 2, 3, 3,
    4, 7, 7, 8, 8, 8,11,11,
    4, 7, 7, 8, 8, 8,11,11,
    5, 9, 9,10,10,10,11,11,
    5, 9, 9,10,10,10,11,11,
    5, 9, 9,10,10,10,11,11,
    6,12,12,12,12,12,13,13,
    6,12,12,12,12,12,13,13};
///

/// ResidualScan::ResidualScan
ResidualScan::ResidualScan(class Frame *frame,class Scan *scan)
  : EntropyParser(frame,scan), m_pulX(NULL), m_pBlockCtrl(NULL),
    m_pResidualBuffer(NULL), m_Helper(frame), 
    m_plDC(NULL), m_plN(NULL), m_plB(NULL), m_bHadamard(false)
{
  m_ucCount = frame->DepthOf();
}
///

/// ResidualScan::~ResidualScan
ResidualScan::~ResidualScan(void)
{
  if (m_pulX) m_pEnviron->FreeMem(m_pulX,sizeof(ULONG) * m_ucCount);
  if (m_plDC) m_pEnviron->FreeMem(m_plDC,sizeof( LONG) * m_ucCount);
  if (m_plN)  m_pEnviron->FreeMem(m_plN ,sizeof( LONG) * m_ucCount);
  if (m_plB)  m_pEnviron->FreeMem(m_plB ,sizeof( LONG) * m_ucCount);

  delete m_pResidualBuffer;

#ifdef PLOT_STATISTICS
  for(int c = 0;c < 2;c++) {
    for(int k = 0;k < 64;k++) {
      char name[64];
      sprintf(name,"statistics_%d_%d.plot",k,c);
      FILE *file = fopen(name,"w");
      int first,last;
      for(first = 0;first < (1 << 18);first++)
	if (count[first][k][c]) break;
      for(last = (1 << 18)-1;last >= 0;last--)
	if (count[last][k][c]) break;
      for(int i = first;i <= last;i++) {
	fprintf(file,"%d\t%d\n",i - OFFSET,count[i][k][c]);
      }
      fclose(file);
    }
  }
#endif
}
///

/// ResidualScan::InitStatistics
// Init the counters for the statistical measurement.
void ResidualScan::InitStatistics(void)
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

  m_Context.Init();
}
///

/// ResidualScan::StartParseScan
void ResidualScan::StartParseScan(class ByteStream *,class BufferCtrl *ctrl)
{ 
  class ResidualMarker *marker = m_pFrame->TablesOf()->ResidualDataOf();

  assert(marker);

  InitStatistics();
  
  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(NULL); 

  // The IO comes actually from the marker, not from the regular IO
  m_Coder.OpenForRead(marker->StreamOf());
}
///

/// ResidualScan::StartWriteScan
void ResidualScan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{ 
  InitStatistics();
  m_bMeasure           = false;

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(NULL);

  //
  // Prepare the residual buffer for collecting the output stream data.
  assert(m_pResidualBuffer == NULL);
  m_pResidualBuffer = new(m_pEnviron) class MemoryStream(m_pEnviron,4096);

  //
  // Where the data actually goes to.
  m_pTarget         = io;
  m_Coder.OpenForWrite(m_pResidualBuffer);
}
///

/// ResidualScan::StartMeasureScan
// Measure scan statistics.
void ResidualScan::StartMeasureScan(class BufferCtrl *ctrl)
{   
  InitStatistics();

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockBuffer *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(NULL);

  //
  // Just ignore, but let pass.
  m_bMeasure = true;
}
///

/// ResidualScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool ResidualScan::StartMCURow(void)
{
  bool more = m_pBlockCtrl->StartMCUResidualRow();

  for(int i = 0;i < m_ucCount;i++) {
    m_pulX[i]   = 0;
  }

  return more;
}
///

/// ResidualScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool ResidualScan::WriteMCU(void)
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
    if (m_bMeasure == false) {
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
    }
    // Done with this component, advance the block.
    m_pulX[c] = xmax;
  }

  return more;
}
///

/// ResidualScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool ResidualScan::ParseMCU(void)
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

/// ResidualScan::EncodeBlock
// Encode a single huffman block
void ResidualScan::EncodeBlock(const LONG *residual,UBYTE comp)
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
      int f     = m_ucCodingClass[k];
      int p     = m_ucFineClass[k];
      int c     = (comp > 0)?(1):(0); // component coding class.
      int l     = 0;
      
#ifdef PLOT_STATISTICS
      count[data + OFFSET][k][c]++;
#endif

      if (data) {
	LONG sz;
	m_Coder.Put(m_Context.S0[p][c],true);
	// Sign coding.
	if (data < 0) {
	  m_Coder.Put(m_Context.SS[p],true);
	  sz = -data - 1;
	} else {
	  m_Coder.Put(m_Context.SS[p],false);
	  sz = data  - 1;
	}
	//
	// Code the magnitude.
	if (sz >= 1) {
	  int  i = 0;
	  LONG m = 2;
	  m_Coder.Put(m_Context.SP[p],true);
	  //
	  // Magnitude category coding.
	  while(sz >= m) {
	    m_Coder.Put(m_Context.X[f][i],true);
	    m <<= 1;
	    i++;
	  } 
	  // Terminate magnitude cathegory coding.
	  m_Coder.Put(m_Context.X[f][i],false);
	  //
	  // Get the MSB to code.
	  m >>= 1;
	  // Refinement bits: Depend on the magnitude category.
	  while((m >>= 1)) {
	    m_Coder.Put(m_Context.M[f][i][l],(m & sz)?(true):(false));
	    if (l < 2) l++;
	  }
	} else {
	  m_Coder.Put(m_Context.SP[p],false);
	}
      } else {
	m_Coder.Put(m_Context.S0[p][c],false);
      }
    }
  } else {
    //
    // Coding of the residual.
    for(k = 0;k < 64;k++) {
      LONG data = residual[k];
      LONG left = (k & 7 )?residual[k-1]:0;
      LONG top  = (k >> 3)?residual[k-8]:0;
      LONG ltop = ((k & 7) && (k >> 3))?residual[k-9]:0;
      LONG delta = 1;
      int s = 0;
      int l = 0;
      int c = (comp > 0)?(1):(0); // component coding class.
      
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
	m_Coder.Put(m_Context.S0[0][c],true);
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
	  m_Coder.Put(m_Context.SP[c],true);
	  //
	  // Magnitude category coding.
	  while(sz >= m) {
	    m_Coder.Put(m_Context.X[c][i],true);
	    m <<= 1;
	    i++;
	  } 
	  // Terminate magnitude cathegory coding.
	  m_Coder.Put(m_Context.X[c][i],false);
	  //
	  // Get the MSB to code.
	  m >>= 1;
	  // Refinement bits: Depend on the magnitude category.
	  while((m >>= 1)) {
	    m_Coder.Put(m_Context.M[c][i][l],(m & sz)?(true):(false));
	    if (l < 2) l++;
	  }
	} else {
	  m_Coder.Put(m_Context.SP[c],false);
	}
      } else {
	m_Coder.Put(m_Context.S0[0][c],false);
      }
    }
  }
}
///

/// ResidualScan::DecodeBlock
// Decode a single huffman block.
void ResidualScan::DecodeBlock(LONG *residual,UBYTE comp)
{
  int k;
  
  if (m_bHadamard) {
    for(k = 0;k < 64;k++) { 
      LONG data; 
      int f     = m_ucCodingClass[k];
      int p     = m_ucFineClass[k];
      int c     = (comp > 0)?(1):(0); // component coding class.
      int l     = 0;
      
      if (m_Coder.Get(m_Context.S0[p][c])) {
	LONG sz;
	bool sign = m_Coder.Get(m_Context.SS[p]); // true if negative.
	//
	if (m_Coder.Get(m_Context.SP[p])) {
	  int  i = 0;
	  LONG m = 2;
	  
	  while(m_Coder.Get(m_Context.X[f][i])) {
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
	    if (m_Coder.Get(m_Context.M[f][i][l])) {
	      sz |= m;
	    }
	    if (l < 2) l++;
	  }
	} else {
	  sz = 0;
	}
	
	if (sign) {
	  data = -sz - 1;
	} else {
	  data =  sz + 1;
	}
      } else {
	data = 0;
      } 
      residual[k] = data;
    }
    
    residual[0] += m_plDC[comp]; 
    ErrorAdaption(residual[0],comp);
  } else {
    for(k = 0;k < 64;k++) { 
      LONG data;
      LONG left = (k & 7 )?residual[k-1]:0;
      LONG top  = (k >> 3)?residual[k-8]:0;
      LONG ltop = ((k & 7) && (k >> 3))?residual[k-9]:0;
      LONG delta = 1;
      int s = 0;
      int l = 0;
      int c = (comp > 0)?(1):(0); // component coding class.
    
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
      
      if (m_Coder.Get(m_Context.S0[0][c])) {
	LONG sz;
	bool sign = m_Coder.Get(m_Context.SS[s]); // true if negative.
	//
	if (m_Coder.Get(m_Context.SP[c])) {
	  int  i = 0;
	  LONG m = 2;
	  
	  while(m_Coder.Get(m_Context.X[c][i])) {
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
	    if (m_Coder.Get(m_Context.M[c][i][l])) {
	      sz |= m;
	    }
	    if (l < 2) l++;
	  }
	} else {
	  sz = 0;
	}

	if (sign) {
	  data = (-sz - 1);
	} else {
	  data = ( sz + 1);
	}
      } else {
	data = 0;
      } 
      residual[k] = data;
    }
  }
}
///

/// ResidualScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void ResidualScan::WriteFrameType(class ByteStream *io)
{
  // This scan doesn't have a frame type as it extends all following scans,
  // so let the following scan do the work.
  assert(m_pScan && m_pScan->NextOf());

  m_pScan->NextOf()->WriteFrameType(io);
}
///

/// ResidualScan::Flush
// Flush the remaining bits out to the stream on writing.
void ResidualScan::Flush(void)
{  
  if (m_bMeasure == false) {
    class ResidualMarker *marker = m_pFrame->TablesOf()->ResidualDataOf();
    //
    assert(marker);
    //
    // Terminate the arithmetic coding.
    m_Coder.Flush();
    //
    // Write out the buffered data as a series of markers.
    marker->WriteMarker(m_pTarget,m_pResidualBuffer);
    //
    delete m_pResidualBuffer;m_pResidualBuffer = NULL;
  }
}
///



