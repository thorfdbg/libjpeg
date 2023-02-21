/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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
** A sequential scan, also the first scan of a progressive scan,
** Huffman coded.
**
** $Id: sequentialscan.cpp,v 1.94 2023/02/21 10:17:41 thor Exp $
**
*/

/// Includes
#include "codestream/sequentialscan.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/rectanglerequest.hpp"
#include "dct/dct.hpp"
#include "std/assert.hpp"
#include "interface/bitmaphook.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
#include "tools/traits.hpp"
#include "control/blockbuffer.hpp"
#include "control/blockbitmaprequester.hpp"
#include "control/blocklineadapter.hpp"
///

/// SequentialScan::SequentialScan
SequentialScan::SequentialScan(class Frame *frame,class Scan *scan,
                               UBYTE start,UBYTE stop,UBYTE lowbit,UBYTE,
                               bool differential,bool residual,bool large,bool baseline)
  : EntropyParser(frame,scan), m_pBlockCtrl(NULL), 
    m_ucScanStart(start), m_ucScanStop(stop), m_ucLowBit(lowbit),
    m_bDifferential(differential), m_bResidual(residual), m_bLargeRange(large), m_bBaseline(baseline)
{  
  UBYTE hidden = m_pFrame->TablesOf()->HiddenDCTBitsOf();
  m_ucCount    = scan->ComponentsInScan();
  
  if (m_ucScanStart > 0 || m_ucScanStop < 63 || m_ucLowBit > hidden)
    m_bProgressive = true;
  else 
    m_bProgressive = false;

  for(int i = 0;i < 4;i++) {
    m_pDCDecoder[i]    = NULL;
    m_pACDecoder[i]    = NULL;
    m_pDCCoder[i]      = NULL;
    m_pACCoder[i]      = NULL;
    m_pDCStatistics[i] = NULL;
    m_pACStatistics[i] = NULL;
    m_plDCBuffer[i]    = NULL;
  }
}
///

/// SequentialScan::~SequentialScan
SequentialScan::~SequentialScan(void)
{
  for(int i = 0;i < 4;i++) {
    if (m_plDCBuffer[i])
      m_pEnviron->FreeMem(m_plDCBuffer[i],sizeof(LONG) * m_ulBlockWidth[i] * m_ulBlockHeight[i]);
  }
}
///

/// SequentialScan::StartParseScan
void SequentialScan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) {
    if (m_ucScanStart == 0) {
      m_pDCDecoder[i]  = m_pScan->DCHuffmanDecoderOf(i);
      if (m_pDCDecoder[i] == NULL)
        JPG_THROW(MALFORMED_STREAM,"SequentialScan::StartParseScan",
                  "Huffman decoder not specified for all components included in scan");
    } else {
      m_pDCDecoder[i]  = NULL; // not required, is AC only.
    }
    if (m_ucScanStop) {
      m_pACDecoder[i]  = m_pScan->ACHuffmanDecoderOf(i);
      if (m_pACDecoder[i] == NULL)
        JPG_THROW(MALFORMED_STREAM,"SequentialScan::StartParseScan",
                  "Huffman decoder not specified for all components included in scan");
   } else {
      m_pACDecoder[i]  = NULL; // not required, is DC only.
    }
    m_lDC[i]           = 0; 
    m_ulX[i]           = 0;
    m_usSkip[i]        = 0;
  }

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);

  m_Stream.OpenForRead(io,chk);
}
///

/// SequentialScan::StartWriteScan
void SequentialScan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) {
    if (m_bResidual == false && m_ucScanStart == 0) {
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

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);
 
  EntropyParser::StartWriteScan(io,chk,ctrl);
  
  m_pScan->WriteMarker(io);
  m_Stream.OpenForWrite(io,chk);

}
///

/// SequentialScan::StartMeasureScan
// Measure scan statistics.
void SequentialScan::StartMeasureScan(class BufferCtrl *ctrl)
{ 
  int i;

  for(i = 0;i < m_ucCount;i++) { 
    m_pDCCoder[i]        = NULL;
    m_pACCoder[i]        = NULL;
    if (m_bResidual == false && m_ucScanStart == 0) {
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

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);

  EntropyParser::StartWriteScan(NULL,NULL,ctrl);

  m_Stream.OpenForWrite(NULL,NULL);
}
///

/// SequentialScan::StartOptimizeScan
// Start making an optimization run to adjust the coefficients.
void SequentialScan::StartOptimizeScan(class BufferCtrl *ctrl)
{  
  int i;

  for(i = 0;i < m_ucCount;i++) {
    if (m_bResidual == false && m_ucScanStart == 0) {
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

  assert(!ctrl->isLineBased());
  m_pBlockCtrl = dynamic_cast<BlockCtrl *>(ctrl);
  m_pBlockCtrl->ResetToStartOfScan(m_pScan);

  EntropyParser::StartWriteScan(NULL,NULL,ctrl);

  m_Stream.OpenForWrite(NULL,NULL);
}
///

/// SequentialScan::StartMCURow
// Start a MCU scan. Returns true if there are more rows.
bool SequentialScan::StartMCURow(void)
{
  bool more = m_pBlockCtrl->StartMCUQuantizerRow(m_pScan);

  for(int i = 0;i < m_ucCount;i++) {
    m_ulX[i]   = 0;
  }

  return more;
}
///

/// SequentialScan::Restart
// Restart the parser at the next restart interval
void SequentialScan::Restart(void)
{
  for(int i = 0; i < m_ucCount;i++) {
    m_lDC[i]           = 0;
    m_usSkip[i]        = 0;
  }

  m_Stream.OpenForRead(m_Stream.ByteStreamOf(),m_Stream.ChecksumOf());
}
///

/// SequentialScan::Flush
// Flush the remaining bits out to the stream on writing.
void SequentialScan::Flush(bool)
{
  if (m_ucScanStop && m_bProgressive) {
    // Progressive, AC band. It looks weird to code the remaining
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

  for(int i = 0; i < m_ucCount;i++) {
    m_lDC[i]           = 0;
    m_usSkip[i]        = 0;
  }
} 
///

/// SequentialScan::WriteMCU
// Write a single MCU in this scan. Return true if there are more blocks in this row.
bool SequentialScan::WriteMCU(void)
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
    LONG &prevdc                    = m_lDC[c];
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
          memset(dummy ,0,sizeof(dummy) );
          block[0] = prevdc;
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
          MeasureBlock(block,dcstat,acstat,prevdc,skip);
        } else {
          EncodeBlock(block,dc,ac,prevdc,skip);
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

/// SequentialScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more blocks in this row.
bool SequentialScan::ParseMCU(void)
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
    LONG &prevdc             = m_lDC[c];
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
          DecodeBlock(block,dc,ac,prevdc,skip);
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

/// SequentialScan::MeasureBlock
// Make a block statistics measurement on the source data.
void SequentialScan::MeasureBlock(const LONG *block,
                                  class HuffmanStatistics *dc,class HuffmanStatistics *ac,
                                  LONG &prevdc,UWORD &skip)
{ 
  // DC coding
  if (m_ucScanStart == 0 && m_bResidual == false) {
    LONG  diff;
    UBYTE symbol = 0;
    // DPCM coding of the DC coefficient.
    diff   = block[0] >> m_ucLowBit; // Actually, only correct for two's complement machines...
    diff  -= prevdc;
    if (m_bDifferential) {
      prevdc = 0;
    } else {
      prevdc = block[0] >> m_ucLowBit;
    }

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
    int k = (m_ucScanStart)?(m_ucScanStart):((m_bResidual)?0:1);
    
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
        if (data == -0x8000 && !m_bProgressive && m_bResidual) {
          ac->Put(0x10);
        } else {
          symbol = 0;
          do {
            symbol++;
            if (symbol >= (m_bLargeRange?22:16))
              JPG_THROW(OVERFLOW_PARAMETER,"SequentialScan::MeasureBlock",
                        "Symbol is too large to be encoded in scan, enable refinement coding to avoid the problem");
            if (data > -(1L << symbol) && data < (1L << symbol)) {
              // Cathegory symbol, run length run
              if (symbol >= 16) {
                // This is the large-range DCT coding required for part 8 if the DCT
                // remains enabled.
                // This map converts symbol=16 into 16, symbol=17 into 32 and so on.
                ac->Put((symbol - 15) << 4);
              } else {
                ac->Put(symbol | (run << 4));
              }
              break;
            }
          } while(true);
          //
          // Run is over.
          run = 0;
        }
      }
    } while(++k <= m_ucScanStop);
    //
    // Is there still an open run? If so, code an EOB.
    if (run) {
      // In the progressive mode, absorb into the skip
      if (m_bProgressive) {
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

/// SequentialScan::CodeBlockSkip
// Code any run of zero blocks here. This is only valid in
// the progressive mode.
void SequentialScan::CodeBlockSkip(class HuffmanCoder *ac,UWORD &skip)
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

/// SequentialScan::EncodeBlock
// Encode a single huffman block
void SequentialScan::EncodeBlock(const LONG *block,
                                 class HuffmanCoder *dc,class HuffmanCoder *ac,
                                 LONG &prevdc,UWORD &skip)
{
  // DC coding
  if (m_ucScanStart == 0 && m_bResidual == false) {
    UBYTE symbol = 0;
    LONG diff;
    //
    // DPCM coding of the DC coefficient.
    diff   = block[0] >> m_ucLowBit; // Actually, only correct for two's complement machines...
    diff  -= prevdc;
    if (m_bDifferential) {
      prevdc = 0;
    } else {
      prevdc = block[0] >> m_ucLowBit;
    }

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
    int k = (m_ucScanStart)?(m_ucScanStart):((m_bResidual)?0:1);

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
        // This is a special case that can only happen in sequential mode, namely coding of the -0x8000
        // symbol.
        if (data == -0x8000 && !m_bProgressive && m_bResidual) {
          ac->Put(&m_Stream,0x10);
          m_Stream.Put(4,run);
        } else {
          symbol = 0;
          do {
            symbol++;
            if (symbol >= (m_bLargeRange?22:16))
              JPG_THROW(OVERFLOW_PARAMETER,"SequentialScan::EncodeBlock",
                        "Symbol is too large to be encoded in scan, enable refinement coding to avoid the problem");
            //
            if (data > -(1L << symbol) && data < (1L << symbol)) {
              // Cathegory symbol, run length run
              // If this is above the limit 16, use the huge DCT model. Note that the above
              // error already excluded the regular case.
              if (symbol >= 16) {
                // This map converts symbol=16 into 16, symbol=17 into 32 and so on.
                ac->Put(&m_Stream,((symbol - 15) << 4));
                m_Stream.Put(4,run);
              } else {
                ac->Put(&m_Stream,symbol | (run << 4));
              }
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
      }
    } while(++k <= m_ucScanStop);
    // Is there still an open run? If so, code an EOB in the regular mode.
    // If this is part of the (isolated) AC scan of the progressive JPEG,
    // check whether we could potentially accumulate this into a run of
    // zero blocks.
    if (run) {
      // Include in a block skip (or try to, rather).
      if (m_bProgressive) {
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

/// SequentialScan::DecodeBlock
// Decode a single huffman block.
void SequentialScan::DecodeBlock(LONG *block,
                                 class HuffmanDecoder *dc,class HuffmanDecoder *ac,
                                 LONG &prevdc,UWORD &skip)
{
  if (m_ucScanStart == 0 && m_bResidual == false) {
    // First DC level coding. If it is in the spectral selection.
    LONG diff   = 0;
    UBYTE value = dc->Get(&m_Stream);
    if (value > 0) {
      LONG v = 1 << (value - 1);
      if (value > 15)
        JPG_THROW(MALFORMED_STREAM,"SequentialScan::DecodeBlock",
                  "DC coefficient decoding out of sync");
      diff   = m_Stream.Get(value);
      if (diff < v) {
        diff += (-1L << value) + 1;
      }
    }
    if (m_bDifferential) {
      prevdc   = diff;
    } else {
      prevdc  += diff;
    }
    block[0] = prevdc << m_ucLowBit; // point transformation
  }

  if (m_ucScanStop) {
    // AC coding. 
    if (skip > 0) {
      skip--; // Still blocks to skip
    } else {
      int k = (m_ucScanStart)?(m_ucScanStart):((m_bResidual)?0:1);

      do {
        UBYTE rs = ac->Get(&m_Stream);
        UBYTE r  = rs >> 4;
        UBYTE s  = rs & 0x0f;
        
        if (s == 0) {
          if (r == 15) {
            k += 16;
            continue;
          } else {
            // A progressive EOB run.
            if (r == 0 || m_bProgressive) {
              skip  = 1 << r;
              if (r) skip |= m_Stream.Get(r);
              skip--; // this block is included in the count.
              break;
            } else if (m_bResidual && rs == 0x10) {
              // The symbol 0x8000
              r  = m_Stream.Get(4); // 4 bits for the run.
              k += r;
              if (k >= 64)
                JPG_THROW(MALFORMED_STREAM,"SequentialScan::DecodeBlock",
                          "AC coefficient decoding out of sync");
              block[DCT::ScanOrder[k]] = -0x8000 << m_ucLowBit; // Point transformation.
              k++;
              continue; //...with the iteration, skipping over zeros.
            } else if (m_bLargeRange) {
              // Large range coding coding codes the magnitude category and the run
              // separately. First extract the category from the bits that usually
              // take up the run.
              s = r + 15;          // This maps 16 into 16, 32 into 17 and so on.
              r = m_Stream.Get(4); // The run is decoded separately, without using Huffman.
              // Check whether this is too large. As we have only 16 bit output at most,
              // we should get away with most 16 here.
              if (s >= 24)
                JPG_THROW(NOT_IMPLEMENTED,"SequentialScan::DecodeBlock",
                          "AC coefficient too large, cannot decode");
              // Continues with the regular case.
            } else {
              JPG_THROW(MALFORMED_STREAM,"SequentialScan::DecodeBlock",
                        "AC coefficient decoding out of sync");
            }
          }
        }
        // Regular code case.
        {
          LONG diff;
          LONG v = 1 << (s - 1);
          k     += r;
          diff   = m_Stream.Get(s);
          if (diff < v) {
            diff += (-1L << s) + 1;
          }
          if (k >= 64)
            JPG_THROW(MALFORMED_STREAM,"SequentialScan::DecodeBlock",
                      "AC coefficient decoding out of sync");
          block[DCT::ScanOrder[k]] = diff << m_ucLowBit; // Point transformation.
          k++;
        }
      } while(k <= m_ucScanStop);
    }
  }
}
///

/// SequentialScan::WriteFrameType
// Write the marker that indicates the frame type fitting to this scan.
void SequentialScan::WriteFrameType(class ByteStream *io)
{
  if (m_bProgressive) {
    // any type of progressive.
    if (m_bResidual) {
      io->PutWord(0xffb2); // residual progressive
    } else if (m_bDifferential) {
      io->PutWord(0xffc6);
    } else {
      io->PutWord(0xffc2);
    }
  } else {    
    if (m_bResidual) {
      io->PutWord(0xffb1); // residual sequential
    } else if (m_bDifferential) {
      io->PutWord(0xffc5);
    } else if (m_bLargeRange) {
      io->PutWord(0xffb3);
    } else if (m_bBaseline) {
      io->PutWord(0xffc0);
    } else {
      io->PutWord(0xffc1);
    }
  }
}
///
/// SequentialScan::OptimizeBlock
// Make an R/D optimization for the given scan by potentially pushing
// coefficients into other bins. This runs an optimization for a single
// block and requires external control to run over the blocks.
// component is the component, critical is the critical slope for
// the R/D optimization of the functional J = \lambda D + R, i.e.
// this is lambda.
// Quant are the quantization parameters, i.e. deltas. These are eventually
// preshifted by "preshift".
// transformed are the dct-transformed but unquantized data. These are also pre-
// shifted by "preshift".
// quantized is the quantized data. These are potentially (and likely) adjusted.
#if ACCUSOFT_CODE 
void SequentialScan::OptimizeBlock(LONG bx,LONG by,UBYTE component,double critical,
                                   class DCT *dct,LONG quantized[64])
#else
void SequentialScan::OptimizeBlock(LONG,LONG,UBYTE,double,class DCT *,LONG[64])
#endif
{
#if ACCUSOFT_CODE 
  class HuffmanCoder *ac  = (m_ucScanStop)?m_pScan->ACHuffmanCoderOf(component):NULL;
  const LONG *transformed = dct->TransformedBlockOf();
  const LONG *delta       = dct->BucketSizes();
  double zdistbuf[64 + 1]; // the element at zero is zero to keep the code simple.
  double jfuncbuf[64 + 1]; // ditto.
  double *zdist = zdistbuf + 1;  // cumulative distortion for pushing coefficients into zero
  double *jfunc = jfuncbuf + 1;  // the cumulative J functional along the run
  LONG zero[64];     // The value of a coefficient if we "push it into zero".
  int start[64];     // start of runs.
  LONG coded[64];
  const LONG thres = (1L << m_ucLowBit) - 1;
  int eobpos = 0;    // position of the EOB
  int k; // position in the scan.
  int ss = m_ucScanStart;
  //
  // Create the DC buffer if we do not yet have it.
  if (m_plDCBuffer[component] == NULL) {
    class Component *comp       = m_pComponent[component];
    const ULONG width           = m_pFrame->WidthOf();
    const ULONG height          = m_pFrame->HeightOf();
    const UBYTE subx            = comp->SubXOf();
    const UBYTE suby            = comp->SubYOf();
    const ULONG blockwidth      = (((width  + subx - 1) / subx) + 7) >> 3;
    const ULONG blockheight     = (((height + suby - 1) / suby) + 7) >> 3;
    // Allocate now the DC buffer
    m_ulBlockWidth[component]   = blockwidth;
    m_ulBlockHeight[component]  = blockheight;
    m_plDCBuffer[component]     = (LONG *)m_pEnviron->AllocMem(sizeof(LONG) * blockwidth * blockheight);
    m_dCritical[component]      = critical;
    // Keep the DC quantizer value for later optimizer.
    m_lDCDelta[component]       = delta[0];
  }
  //
  // Keep the DC coefficient for later.
  m_plDCBuffer[component][bx + m_ulBlockWidth[component] * by] = transformed[0];
  //
  // Start of the scan. Do not include the DC coefficient if we have one.
  if (ss == 0 && !m_bResidual)
    ss = 1;
  //
  // Only consider AC coefficients. DC coefficients would require cross-block optimizations
  // and are hence harder to do.
  // Start of the trellis: Initialize the cumulative functionals to zero.
  zdist[ss - 1] = 0.0;
  jfunc[ss - 1] = 0.0;
  for(k = ss;k <= m_ucScanStop;k++) {
    int j         = DCT::ScanOrder[k];
    LONG quant    = quantized[j];
    LONG data; // the data encoded in the codestream after the point shift.
    double weight = 8.0 / delta[j];
    int symbol;  // the size category of the old symbol, or 0 if the amplitude is +1 or -1
    int newsymb; // the second alternative for the symbol if we modify the amplitude.
    double error;
    //
    // Include the point shift in the quantized data for the symbol computation.
    // Keep the encoded data for later.
    data     = (quant >= 0)?(quant >> m_ucLowBit):(-((-quant) >> m_ucLowBit));
    coded[j] = data;
    //
    // Pool up the entire error along the scan. This means that the total
    // error for initiating a zero-run can be given by the difference of the zdist
    // functional at the end minus the value at the start-1. Note that due to
    // the point-shift by low-bits, the coefficient may not be actually
    // zero after the adjustment. Compute the largest possible value to make
    // the coefficient consistent with the inclusion of the coefficient in the
    // zero-bin.
    if (quant < -thres) {
      zero[k] = -thres; // lowest possible value.
    } else if (quant > thres) {
      zero[k] = thres;
    } else {
      zero[k] = quant; // is already consistent with the value.
    } 
    //
    // Compute the distortion when setting the coefficient to zero.
    // Additional error due to including this coefficient in a run by
    // setting it to zero.
    error    = (zero[k] * delta[j] - transformed[j]) * weight;
    zdist[k] = critical * error * error + zdist[k - 1];
    //
    // This is the cumulative j functional up to the position k, so far. Will be
    // optimized during this iteration finding its minimum.
    jfunc[k] = HUGE_VAL;
    //
    // If the quantized version is already zero, there is no need to modify anything.
    if (data) {
      double distnew,distold;
      LONG newquant;
      LONG bestquant = quant; // quantized value that goes for the best coefficient decision.
      // Compute the new and old distortion for pushing the coefficient into the next lower bin.
      // The next higher bin makes no sense (higher rate, usually), 
      // and anything else is probably too unlikely to worry about.
      double errold;
      double errnew;
      // This coefficient may profit from an amplitude change. Actually, we may
      // consider more than one amplitude change, but in reality, it rately makes
      // sense to change the amplitude by more than one bucket. Thus, we only keep
      // two possibilities here (or actually three, namely set the coefficient to
      // zero completely).
      // The rate is only reduced if we change the amplitude category by one.
      // (or, theoretically, by more).
      symbol = 0;
      do {
        symbol++;
        if (data > -(1L << symbol) && data < (1L << symbol)) {
          // We got the right category for the symbol.
          if (symbol > 1) {
            // Ok, there is at least a chance to modify the symbol
            // Try to push it into the next lower category while keeping
            // it as large as possible. This also modifies the bits
            // for subsequent refinement scans.
            newquant = (1L << (symbol + m_ucLowBit - 1)) - 1;
            newsymb  = symbol - 1;
            if (quant < 0)
              newquant = -newquant;
          } else {
            // Magnitude category 1 does not really have a choice
            // between two options. This can either stay non-zero
            // or become part of a run (which is evaluated below).
            newquant = quant;
            newsymb  = symbol;
          }       
          break;
        }
      } while(true);
      //
      // Compute the distortion difference as new - old. 
      errold    = (double)(quant    * delta[j] - transformed[j]) * weight;
      errnew    = (double)(newquant * delta[j] - transformed[j]) * weight;
      distold   = errold * errold * critical;
      distnew   = errnew * errnew * critical;
      //
      // Now compute the cost for encoding the run in front of this coefficient.
      for (int l = ss - 1;l < k;l++) {
        // Accumulate j for starting the run at l (actually, l+1 is the first
        // coefficient that is part of the run, the coefficient at l is non-zero
        // or the DC coefficient).
        if (l == ss - 1 || coded[DCT::ScanOrder[l]]) {
          int run = k - 1 - l;
          int runrate = 0,rateold,ratenew;
          LONG qnt;
          double jf; // new candidate of the J functional.
          double jold,jnew;
          // Non-zero coefficient. This is now a potential "push-l-into-zero" case which might
          // create a new run of the size above.
          // For that, compute now the cost of the run. 
          // First, to encode the runs larger than 16 (if we can).
          if ((run >> 4)) {
            runrate = ac->isDefined(0xf0);
            if (runrate == 0)
              continue; // This is not an option if the Huffman code does not define this
            runrate = (run >> 4) * runrate;
          }
          // the rest goes into the 2D VLC.
          run    &= 0x0f;
          // Now check which overall rate we get if we modify the current coefficient. If the
          // coefficient is +1/-1, then the computation is different and there is only one
          // candidate, and the Huffman alphabet contains the necessary symbol.
          rateold = ac->isDefined((run << 4) | symbol );
          ratenew = ac->isDefined((run << 4) | newsymb);
          // Compute the total j functional for the modification. This is given by the 
          // contribution due to the coefficient itself, plus the contribution for the
          // run in front of it.
          jold    = distold + zdist[k-1] - zdist[l] + rateold + symbol  + runrate;
          jnew    = distnew + zdist[k-1] - zdist[l] + ratenew + newsymb + runrate;
          // Pick the minimum of the two as new candidate j.
          if (rateold && jold <= jnew) {
            jf        = jold;
            qnt       = quant;
          } else if (ratenew) {
            jf        = jnew;
            qnt       = newquant;
          } else continue; // the symbol is not in the alphabet.
          //
          // Include in jf the cumulated j functional up to the start position.
          jf += jfunc[l];
          // Ok, if the cost of the run up to me starting at the given position plus
          // the modification of the current coefficient is lower than the current
          // accumulated cost, make the change and start the run at position l.
          if (jf < jfunc[k]) {
            jfunc[k]  = jf;
            start[k]  = l;
            bestquant = qnt;
          }
        }
      }
      // Now we have in jfunc[k] the optimal accumulated J functional up to the current position
      // in start[k] the ideal start position of a run up to the current position and in
      // bestquant the ideal quantized value at position k, so fill it in.
      quantized[j] = bestquant;
    }
    // End of loop over coefficients.
  }
  //
  // Up to now, EOB coding has not been taken into account. Now check were to place the EOB.
  // eobpos contains the position behind which the EOB is placed.
  if (m_ucScanStop) {
    if (ac->isDefined(0x00)) {
      double jeob = zdist[m_ucScanStop] + ac->isDefined(0x00);
      // joeb is the value of the j functional for coding the entire block as zero, 
      // i.e. coding the eob directly at the first AC coefficient.
      for(k = ss;k <= m_ucScanStop;k++) {
        if (coded[DCT::ScanOrder[k]]) {
          // Include in the functional the cost for placing the EOB right behind 
          // this block and zero-ing out the rest of the block.
          double jf = jfunc[k] + zdist[m_ucScanStop] - zdist[k];
          // If this is not the end of the block, include the rate of the EOB.
          if (k < m_ucScanStop)
            jf += ac->isDefined(0x00);
          // If the functional gets lower by terminating the block at position k, remember this
          // choice.
          if (jf < jeob) {
            jeob  = jf;
            eobpos = k;
          }
        }
      }
    } else {
      // No EOB in the alphabet. Yuck. Ok, so stop at 63.
      eobpos = m_ucScanStop;
    }
    //
    // Done. Zero-out the coefficients in the runs and behind the EOB, or rather, push
    // them into the deadzone. Note that this is not the same for LowBit > 0.
    for(k = m_ucScanStop;k >= ss;k--) {
      if (k > eobpos) { // Is behind a run?
        quantized[DCT::ScanOrder[k]] = zero[k]; // zero out the coefficient.
      } else {
      // Otherwise, find the start of the run that ends at this coefficient,
      // and continue to clean out up to this position.
        eobpos = start[k];
      }
    }
  }
#else
  JPG_THROW(NOT_IMPLEMENTED,"SequentialScan::OptimizeBlock",
            "soft-threshold quantizer not implemented in this code version");
#endif  
}
///

/// SequentialScan::OptimizeDC
// Optimize the DC values of all blocks within this scan.
// Unlike the AC optimization, this requires a cross-block optimization.
void SequentialScan::OptimizeDC(void)
{
#if ACCUSOFT_CODE 
  UBYTE c;
  LONG dctrange = 1L << (m_pFrame->HiddenPrecisionOf() + 4);
  // Maximum number of bits we can possibly use in the DC value.
 
  assert(m_pFrame && m_pScan);
  //
  // This only makes sense if the start of the scan contains the DC value and we
  // do not use residual coding.
  if (m_ucScanStart || m_bResidual)
    return;

  StartMCURow();
  
  for(c = 0;c < m_ucCount;c++) {
    class Component *comp  = m_pComponent[c];
    const class QuantizedRow *volatile qr = m_pBlockCtrl->CurrentQuantizedRow(comp->IndexOf());
    DOUBLE critical        = m_dCritical[c];
    struct BackTrace {
      LONG  *bt_plData;         // Points to the original DC data we want to modify.
      LONG   bt_lDC[3];         // The various choices we have for the DC values.
      int    bt_iPrev[3];       // backtrace: The ideal predicessor for the current DC value.
      DOUBLE bt_dFunctional[3]; // the various values for the J functional J = R + \lambda D
    } *btr                 = NULL;
    volatile const UBYTE mcux = (m_ucCount > 1)?(comp->MCUWidthOf() ):(1);
    volatile const UBYTE mcuy = (m_ucCount > 1)?(comp->MCUHeightOf()):(1);
    ULONG blockwidth       = m_ulBlockWidth[c];
    ULONG blockheight      = m_ulBlockHeight[c];
    ULONG xmcu,ymcu;
    class HuffmanCoder *dc = m_pDCCoder[c];
    const LONG dcdelta     = m_lDCDelta[c];
    double weight          = 8.0 / dcdelta;
    //
    JPG_TRY {
      const class QuantizedRow *volatile q;
      struct BackTrace *bt = (struct BackTrace *)m_pEnviron->AllocVec(sizeof(struct BackTrace) * 
                                                                      (blockwidth * blockheight + 1));
      // Keep the pointer to the start of the array.
      btr = bt;
      //
      // Initialize the start of the trellis.
      for(int i = 0;i < 3;i++) {
        bt->bt_dFunctional[i] = 0.0;
        bt->bt_lDC[i]         = 0; // Previous DC value: Is always zero.
        bt->bt_iPrev[i]       = 0; // Backtrace.
      }
      // This does not belong to any data block. It is just the start of the trellis.
      bt->bt_plData = NULL;
      bt++;
      for(ymcu = 0;ymcu < blockheight;ymcu += mcuy) {
        for(xmcu = 0;xmcu < blockwidth;xmcu += mcux) {
          ULONG xmin        = xmcu;
          ULONG xmax        = xmin + mcux;
          ULONG ymin        = ymcu;
          ULONG ymax        = ymin + mcuy;
          ULONG x,y;
          for(y = ymin,q = qr;y < ymax;y++) {
            for(x = xmin;x < xmax;x++) {
              if (q && x < q->WidthOf()) {
                LONG transformed = m_plDCBuffer[c][x + m_ulBlockWidth[c] * y];
                // There is a previous block. If this is not the case, then
                // we do not need to optimize. The cost is always constant.
                // Get a pointer to the data that we want to modify if present.
                bt->bt_plData = q->BlockAt(x)->m_Data;
                // Now try all possible current values for this DC value. Test only the
                // nearby quantizer values. Everything else does not make sense, i.e.
                // fixed rate quantization is not *that* far away from varying rate
                // quantization that we can be off by more than one bucket.
                for(int curcand = 0;curcand < 3;curcand++) {
                  LONG newqnt = *bt->bt_plData + ((curcand - 1) << m_ucLowBit);
                  DOUBLE distortion;
                  DOUBLE jbest = HUGE_VAL;
                  LONG error;
                  int cbest    = 0;
                  // Ensure we do not overshoot the range.
                  if (newqnt >= dctrange)
                    newqnt = dctrange - 1;
                  if (newqnt <= -dctrange)
                    newqnt = 1 - dctrange;
                  // Compute the error in the DC domain.
                  error      = double(dcdelta * newqnt - transformed) * weight;
                  // Compute the resulting distortion.
                  distortion = critical * error * error;
                  // Keep the quantized (modified) DC candidate.
                  bt->bt_lDC[curcand] = newqnt;
                  // Compute now the symbol to decode, for all possible values
                  // of the last DC value.
                  for(int lastcand = 0;lastcand < 3;lastcand++) {
                    LONG prevdc = bt[-1].bt_lDC[lastcand] >> m_ucLowBit;
                    LONG curdc  = bt[0 ].bt_lDC[curcand ] >> m_ucLowBit;
                    LONG diff   = curdc;
                    LONG symbol = 0;
                    DOUBLE jnow;
                    if (!m_bDifferential) {
                      // Actually, pretty pointless to optimize over the last bit if we are differential...
                      // Anyhow, we support differential, so let's use a minimum attempt to support it...
                      diff     -= prevdc;
                    }
                    if (diff) {
                      do {
                        symbol++;
                        if (diff > -(1L << symbol) && diff < (1L << symbol)) {
                          break;
                        }
                      } while(true);
                    }
                    // symbol contains now the DC bits reqired to encode the data.
                    // Compute now the value of the functional.
                    jnow = distortion + dc->isDefined(symbol) + symbol + bt[-1].bt_dFunctional[lastcand];
                    if (jnow < jbest) {
                      jbest = jnow;
                      cbest = lastcand;
                    }
                  }
                  // Ok, the best possible previous candidate for the current selection of the
                  // candidate is now in cbest, and its contribution to the J functional is
                  // in jbest.
                  bt[0].bt_dFunctional[curcand] = jbest;
                  bt[0].bt_iPrev[curcand]       = cbest;
                }
                //assert((x == 0 && y == 0) || bt[0].bt_iPrev[dccandidates >> 1] == (dccandidates >> 1));
                // This DC candidate has been handled, go to the next.
                bt++;
              }
            } // of loop over MCU in X direction
            if (q) q = q->NextOf();
          } // of loop over MCU in Y direction
        }
        // Advance to the next row of MCUs.
        qr = q;
      }
      //
      // Now every block has been visited. Now trace back all blocks, using the minimal rate-distortion.
      bt--;
      if (bt > btr) {
        DOUBLE jopt = HUGE_VAL;
        int cand    = 0;
        for(int i = 0;i < 3;i++) {
          if (bt->bt_dFunctional[i] < jopt) {
            jopt = bt->bt_dFunctional[i];
            cand = i;
          }
        }
        // cand is now the right quantized DC value for the data referenced by bt.
        while(bt > btr) {
          assert(bt->bt_plData);
          *bt->bt_plData = bt->bt_lDC[cand];
          cand           = bt->bt_iPrev[cand];
          bt--;
        }
      }
      //
      // Release the backtrace.
      if (btr != NULL) {
        m_pEnviron->FreeVec(btr);
        btr = NULL;
      }
      // Advance to the next component.
    } JPG_CATCH {
      if (btr != NULL) {
        m_pEnviron->FreeVec(btr);
        btr = NULL;
      }
      JPG_RETHROW;
      //
    } JPG_ENDTRY;
  }
#else
  JPG_THROW(NOT_IMPLEMENTED,"SequentialScan::OptimizeDC",
            "soft-threshold quantizer not implemented in this code version");
#endif 
}
///

