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
** arithmetic coding procedure.
**
** $Id: acsequentialscan.hpp,v 1.25 2012-09-23 14:10:12 thor Exp $
**
*/

#ifndef CODESTREAM_ACSEQUENTIALSCAN_HPP
#define CODESTREAM_ACSEQUENTIALSCAN_HPP

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
class BitmapCtrl;
class LineAdapter;
class BufferCtrl;
class BlockBuffer;
///

/// class ACSequentialScan
class ACSequentialScan : public EntropyParser {
  //
  // The QM coder doing the main work here.
  class QMCoder            m_Coder;
  //
  // Last DC value, required for the DPCM coder.
  LONG                     m_lDC[4];
  //
  // Last difference value, required for selecting the
  // AC coding context.
  LONG                     m_lDiff[4];
  //
  // Height in blocks.
  ULONG                    m_ulHeight;
  //
  // Context information
  struct QMContextSet {
    //
    // The DC Coding context set.
    struct DCContextZeroSet {
      QMContext S0,SS,SP,SN;
      //
      // Initialize
#ifdef DEBUG_QMCODER  
      void Init(const char *base)
      {
	char string[5] = "Z0S0";
	memcpy(string,base,2);
	S0.Init(string);
	string[3] = 'S';
	SS.Init(string);
	string[3] = 'P';
	SP.Init(string);
	string[3] = 'N';
	SN.Init(string);
      }
#else
      void Init(void)
      {
	S0.Init();
	SS.Init();
	SP.Init();
	SN.Init();
      }
#endif
      //
    } DCZero,DCSmallPositive,DCSmallNegative,DCLargePositive,DCLargeNegative;
    //
    // The DC Magnitude coding contexts.
    struct DCContextMagnitudeSet {
      QMContext X[15];
      QMContext M[15];
      //
      // Initialize
      void Init(void)
      {
	for(int i = 0;i < 15;i++) {
#ifdef DEBUG_QMCODER
	  char string[5] = "X0  ";
	  string[1] = (i / 10) + '0';
	  string[2] = (i % 10) + '0';
	  X[i].Init(string);
	  string[0] = 'M';
	  M[i].Init(string);
#else
	  X[i].Init();
	  M[i].Init();
#endif
	}
      }
    } DCMagnitude;
    //
    // The AC Coding Contexts.
    struct ACContextZeroSet {
      QMContext SE,S0,SP;
      //
      // Initialize.
#ifdef DEBUG_QMCODER
      void Init(int i)
      {
	char string[5] = "se00";
	string[2] = (i / 10) + '0';
	string[3] = (i % 10) + '0';
	SE.Init(string);
	string[1] = '0';
	S0.Init(string);
	string[1] = 'p';
	SP.Init(string);
      }
#else
      void Init(void)
      {
	SE.Init();
	S0.Init();
	SP.Init();
      }
#endif
    } ACZero[63];
    //
    // The AC Magnitude coder.
    struct ACContextMagnitudeSet {
      QMContext X[14];
      QMContext M[14];
      //

#ifdef DEBUG_QMCODER
      void Init(bool hi) 
      {
	for(int i = 0;i < 14;i++) {
	  char string[5] = "xl00";
	  string[1] = (hi)?('h'):('l');
	  string[2] = (i / 10) + '0';
	  string[3] = (i % 10) + '0';
	  X[i].Init(string);
	  string[0] = 'm';
	  M[i].Init(string);
	}
      }
#else
      void Init(void)
      {
	for(int i = 0;i < 14;i++) {
	  X[i].Init();
	  M[i].Init();
	}
      }
#endif
    } ACMagnitudeLow,ACMagnitudeHigh; // Exists only twice.
    //
    // The uniform context.
    QMContext Uniform;
    //
    // Initialize the full beast.
    void Init(void)
    {
#ifdef DEBUG_QMCODER 
      DCZero.Init("Z0");
      DCSmallPositive.Init("L+");
      DCSmallNegative.Init("L-");
      DCLargePositive.Init("U+");
      DCLargeNegative.Init("U-");      
#else
      DCZero.Init();
      DCSmallPositive.Init();
      DCSmallNegative.Init();
      DCLargePositive.Init();
      DCLargeNegative.Init();
#endif
      DCMagnitude.Init();
      DCMagnitude.Init();
      for(int i = 0;i < 63;i++) {
#ifdef DEBUG_QMCODER
	ACZero[i].Init(i);
#else
	ACZero[i].Init();
#endif
      }
#ifdef DEBUG_QMCODER
      ACMagnitudeLow.Init(false);
      ACMagnitudeHigh.Init(true);
#else
      ACMagnitudeLow.Init();
      ACMagnitudeHigh.Init();
#endif
#ifdef DEBUG_QMCODER
      Uniform.Init(QMCoder::Uniform_State,"uni ");
#else
      Uniform.Init(QMCoder::Uniform_State);
#endif
    }  
    //
    // Classify the DC difference into five categories, return it.
    struct DCContextZeroSet &Classify(LONG diff,UBYTE l,UBYTE u);
    //
  } m_Context[4];
  //
  // 
protected:
  //
  // The block control helper that maintains all the request/release
  // logic and the interface to the user.
  class BlockBuffer          *m_pBlockCtrl;
  //
  // Scan positions.
  ULONG                       m_ulX[4];
  //
  // Scan parameters.
  UBYTE                       m_ucScanStart;
  UBYTE                       m_ucScanStop;
  UBYTE                       m_ucLowBit; 
  //
  // AC conditioners, one per component.
  //
  // Context numbers to use for the conditional.
  UBYTE                       m_ucDCContext[4];
  UBYTE                       m_ucACContext[4];
  //
  // Small DC threshold value ('L' in the standard)
  UBYTE                       m_ucSmall[4];
  //
  // Large DC threshold value ('U' in the specs)
  UBYTE                       m_ucLarge[4];
  //
  // Higher block index discrimination ('kx' in the specs)
  UBYTE                       m_ucBlockEnd[4]; 
  //
  // Will always be false as there is no reason to measure anything.
  // This is only here to satisfy the expected interface of the
  // residual scan.
  bool                        m_bMeasure;
  //
  // Set if this is a differential scan.
  bool                        m_bDifferential;
  //
  // Set if this is a residual scan.
  bool                        m_bResidual;
  //
  // Encode a single block
  void EncodeBlock(const LONG *block,
		   LONG &prevdc,LONG &prevdiff,
		   UBYTE small,UBYTE large,UBYTE blockup,
		   UBYTE dctable,UBYTE actable);
  //
  // Decode a single block.
  void DecodeBlock(LONG *block,
		   LONG &prevdc,LONG &prevdiff,
		   UBYTE small,UBYTE large,UBYTE blockup,
		   UBYTE dctable,UBYTE actable);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final);
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
  // Return the data to process for component c. Will be overridden
  // for residual scan types.
  virtual class QuantizedRow *GetRow(UBYTE idx) const; 
  //
  // Check whether there are more rows to process, return true
  // if so, false if not. Start with the row if there are.
  virtual bool StartRow(void) const;
  //
private:
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  //
public:
  // Create an arithmetically coded sequential scan. The highbit is always
  // ignored as this setting only exists for progressive refinement scans.
  ACSequentialScan(class Frame *frame,class Scan *scan,UBYTE start,UBYTE stop,
		   UBYTE lowbit,UBYTE highbit,
		   bool differential = false,bool residual = false);
  //
  ~ACSequentialScan(void);
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
