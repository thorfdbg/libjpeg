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
** Represents the scan including the scan header for the
** arithmetic coding procedure for follow-up refinement
** scans.
**
** $Id: acrefinementscan.hpp,v 1.13 2012-09-23 14:10:12 thor Exp $
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
///

/// class ACRefinementScan
class ACRefinementScan : public EntropyParser {
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
      //
    } ACZero[63];
    //
    // The uniform context.
    QMContext Uniform;
    //
    // Initialize the full beast.
    void Init(void)
    {
      for(int i = 0;i < 63;i++) {
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
  class BlockBuffer          *m_pBlockCtrl;
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
  // Encode a single block
  void EncodeBlock(const LONG *block);
  //
  // Decode a single block.
  void DecodeBlock(LONG *block);
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
  virtual void StartParseScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding 
  virtual void StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl);
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
