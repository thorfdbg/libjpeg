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
** Represents the lossless scan - lines are coded directly with predictive
** coding, though here residuals are encoded with the arithmetic encoder.
**
** $Id: aclosslessscan.hpp,v 1.14 2012-06-02 10:27:13 thor Exp $
**
*/

#ifndef CODESTREAM_ACLOSSLESSSCAN_HPP
#define CODESTREAM_ACLOSSLESSSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "coding/qmcoder.hpp"
#include "codestream/entropyparser.hpp"
///

/// Forwards
class Frame;
class LineCtrl;
class ByteStream;
class LineBitmapRequester;
class LineBuffer;
class BitmaCtrl;
class Scan;
///

/// class LosslessScan
// A lossless scan creator.
class ACLosslessScan : public EntropyParser {
  //
  // The class used for pulling and pushing data.
  class LineBuffer          *m_pLineCtrl;
  //
  // Dimension of the frame in full pixels.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Dimensions of the components.
  ULONG                      m_ulWidth[4];
  ULONG                      m_ulHeight[4];
  // 
  // Small DC threshold value ('L' in the standard)
  UBYTE                      m_ucSmall[4];
  //
  // Large DC threshold value ('U' in the specs)
  UBYTE                      m_ucLarge[4];
  //
  // Differentials from the above and left, used
  // for prediction.
  LONG                       m_lDa[4],m_lDb[4];
  //
  // The real worker class.
  class QMCoder              m_Coder;
  //
  // Context information.
  struct QMContextSet {
    //
    // The Zero-Sign coding contexts - this is a 5x5 set.
    struct ContextZeroSet {
      QMContext S0,SS,SP,SN;
      //
      void Init(void)
      {
	S0.Init();
	SS.Init();
	SP.Init();
	SN.Init();
      }
    } SignZeroCoding[5][5];
    //
    // The Magnitude/refinement coding contexts.
    struct MagnitudeSet {
      QMContext X[15];
      QMContext M[15];
      //
      void Init(void)
      {
	for(int i = 0;i < 15;i++) {
	  X[i].Init();
	  M[i].Init();
	}
      }
    } MagnitudeLow,MagnitudeHigh;
    //
    void Init(void)
    {
      for(int i = 0;i <5;i++) {
	for(int j = 0;j < 5;j++) {
	  SignZeroCoding[i][j].Init();
	}
      }
      MagnitudeLow.Init();
      MagnitudeHigh.Init();
    }
  } m_Context;
  //
  // The predictor to use.
  UBYTE m_ucPredictor;
  //
  // The low bit for the point transform.
  UBYTE m_ucLowBit;
  //
  // Disable prediction as if we were at the first line.
  // Used behind restart markers.
  bool m_bNoPrediction;
  //
  //
  // Classify the difference according to the l and u conditioning parameters.
  static int Classify(LONG diff,UBYTE l,UBYTE u);
  //
  // Collect component information and install the component dimensions.
  void FindComponentDimensions(void);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(void); 
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
  // Clear the entire MCU
  void ClearMCU(class Line **top);
  //
  //
public:
  ACLosslessScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lobit);
  //
  virtual ~ACLosslessScan(void);
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding
  virtual void StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Start the measurement run - not required here.
  virtual void StartMeasureScan(class BufferCtrl *ctrl);
  //
  // Start a MCU scan. Returns true if there are more rows. False otherwise.
  // Note that we emulate here that MCUs are multiples of eight lines high
  // even though from a JPEG perspective a MCU is a single pixel in the
  // lossless coding case.
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
