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
** This scan type decodes the coding residuals and completes an image
** into a lossless image.
**
** $Id: residualscan.hpp,v 1.16 2012-07-27 08:08:33 thor Exp $
**
*/

#ifndef CODESTREAM_RESIDUALSCAN_HPP
#define CODESTREAM_RESIDUALSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "io/bitstream.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/entropyparser.hpp"
#include "coding/qmcoder.hpp"
#include "control/residualblockhelper.hpp"
///

/// Forwards
class Tables;
class ByteStream;
class DCT;
class Frame;
struct RectangleRequest;
class BlockBitmapRequester;
class BlockBuffer;
class MemoryStream;
///

/// class ResidualScan
class ResidualScan : public EntropyParser {
  //
  // The QM coder doing the main work here.
  class QMCoder            m_Coder;
  //
  // Scan positions.
  ULONG                      *m_pulX;
  ULONG                      *m_pulY;
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockBuffer          *m_pBlockCtrl; 
  //
  // Buffers the output before it is split into APP4 markers.
  class MemoryStream         *m_pResidualBuffer;
  //
  // Where the data finally ends up.
  class ByteStream           *m_pTarget;
  //
  class ResidualBlockHelper   m_Helper;
  //
  // Measure the scan? If so, nothing happens. As this just extends
  // scans, even huffman scans, nothing will happen then, but no error
  // can be generated.
  bool                        m_bMeasure;
  //
  // Frequency coding class.
  static const UBYTE          m_ucCodingClass[64];
  //
  // Finer resolution frequency coding class.
  static const UBYTE          m_ucFineClass[64];
  //
  // DC gain cancelation: May appear due to gamma mapping.
  LONG                       *m_plDC;
  // Event counter
  LONG                       *m_plN;
  // Average error counter
  LONG                       *m_plB;
  //
  // Enable or disable the Hadamard transformation.
  bool                        m_bHadamard;
  //
  // Update the predicted DC gain.
  void ErrorAdaption(LONG dc,UBYTE comp)
  {
    m_plB[comp] += dc;
    m_plN[comp]++;
    m_plDC[comp] = m_plB[comp] / m_plN[comp];

    if (m_plN[comp] > 64) {
      m_plN[comp] >>= 1;
      if (m_plB[comp] >= 0) {
	m_plB[comp] = (m_plB[comp]) >> 1;
      } else {
	m_plB[comp] = -((-m_plB[comp]) >> 1);
      }
    }
  }
  //
  // Contexts for the residual coder.
  struct QMContextSet {
    //
    // Magnitude context. Three are sufficient.
    QMContext M[16][24][4];
    QMContext X[16][24];
    QMContext S0[16][2],SP[16];
    QMContext SS[16];
    //
#ifdef DEBUG_QMCODER 
    void Init(void)
    {
      int i,j,k;
      char name[5] = "";
      
      for(j = 0;j < 24;j++) {
	for(k = 0;k < 16;k++) {
	  for(i = 0;i < 4;i++) {
	    name[0] = 'M';name[1] = k+'a';name[2] = j+'a';name[3] = i+'0';
	    M[k][j][i].Init(name);
	  }
	  name[0] = 'X';name[3] = 0;
	  X[k][j].Init(name);
	}
      }
      for(i = 0;i < 2;i++) {
	for(k = 0;k < 16;k++) {
	  name[0] = 'S';name[1]='0';name[2]=k+'a';name[3]=i+'0';
	  S0[k][i].Init(name);
	  name[1]='P';name[3]=0;
	  SP[k].Init(name);
	  name[1]='S';
	  SS[k].Init(name);
	}
      }
    }
#else
    void Init(void)
    {
      int i,j,k;
      
      for(j = 0;j < 24;j++) {
	for(k = 0;k < 16;k++) {
	  for(i = 0;i < 4;i++) {
	    M[k][j][i].Init();
	  }
	  X[k][j].Init();
	}
      }
      for(k = 0;k < 16;k++) {
	for(i = 0;i < 2;i++) {
	  S0[k][i].Init();
	}
	SP[k].Init();
	SS[k].Init();
      }
    }
#endif
    //
#ifdef DEBUG_QMCODER 
    void Print(void)
    {
      int i,j,k;
      //
      for(j = 0;j < 24;j++) {
	for(k = 0;k < 16;k++) {
	  for(i = 0;i < 4;i++) {
	    M[k][j][i].Print();
	  }
	  X[k][j].Print();
	}
      }
      for(i = 0;i < 2;i++) {
	for(k = 0;k < 16;k++) {
	  S0[k][i].Print();
	  SP[k].Print();
	  SS[k].Print();
	}
      }
    }
#endif
  }                           m_Context;
  //
  // Init the counters for the statistical measurement.
  void InitStatistics(void);
  //
  // Encode a single residual block
  void EncodeBlock(const LONG *rblock,UBYTE c);
  //
  // Decode a single residual block.
  void DecodeBlock(LONG *block,UBYTE c);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(void);
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void)
  {
    // As this never writes restart markers but rather relies on
    // the APP9 marker segment for restart, if any, this should
    // never be called.
    assert(!"residual streams do not write restart markers");
  }
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  //
public:
  ResidualScan(class Frame *frame,class Scan *scan);
  //
  ~ResidualScan(void);
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
