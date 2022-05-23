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
** Represents the scan including the scan header for the
** arithmetic coding procedure.
**
** $Id: acsequentialscan.hpp,v 1.39 2022/05/23 05:56:51 thor Exp $
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
class BlockCtrl;
///

/// class ACSequentialScan
class ACSequentialScan : public EntropyParser {
  //
#if ACCUSOFT_CODE
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
      enum {
        MagnitudeContexts = 19
      };
      //
      QMContext X[MagnitudeContexts];
      QMContext M[MagnitudeContexts];
      //
      // Initialize
      void Init(void)
      {
        for(int i = 0;i < MagnitudeContexts;i++) {
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
      enum {
        MagnitudeContexts = 18
      };
      //
      QMContext X[MagnitudeContexts];
      QMContext M[MagnitudeContexts];
      //
#ifdef DEBUG_QMCODER
      void Init(bool hi) 
      {
        for(int i = 0;i < MagnitudeContexts;i++) {
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
        for(int i = 0;i < MagnitudeContexts;i++) {
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
  class BlockCtrl            *m_pBlockCtrl;
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
  // Set if this is a large range scan.
  bool                        m_bLargeRange;
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
#endif
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
  // Create an arithmetically coded sequential scan. The highbit is always
  // ignored as this setting only exists for progressive refinement scans.
  ACSequentialScan(class Frame *frame,class Scan *scan,UBYTE start,UBYTE stop,
                   UBYTE lowbit,UBYTE highbit,
                   bool differential = false,bool residual = false,bool largerange = false);
  //
  ~ACSequentialScan(void);
  // 
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding 
  virtual void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Measure scan statistics. Not implemented here since it is not
  // required. The AC coder is adaptive.
  virtual void StartMeasureScan(class BufferCtrl *ctrl);
  //
  // Start making an optimization run to adjust the coefficients.
  virtual void StartOptimizeScan(class BufferCtrl *ctrl);
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
  //
  // Make an R/D optimization for the given scan by potentially pushing
  // coefficients into other bins. 
  virtual void OptimizeBlock(LONG bx,LONG by,UBYTE component,double critical,
                             class DCT *dct,LONG quantized[64]); 
  //
  // Make an R/D optimization of the DC scan. This includes all DC blocks in
  // total, not just a single block. This is because the coefficients are not
  // coded independently.
  virtual void OptimizeDC(void);
};
///


///
#endif
