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
** This is the base class for all predictive scan types, it provides the
** services useful to implement them such that the derived classes can
** focus on the actual algorithm.
**
** $Id: predictivescan.hpp,v 1.13 2024/11/05 06:39:25 thor Exp $
**
*/

#ifndef CODESTREAM_PREDICTIVESCAN_HPP
#define CODESTREAM_PREDICTIVESCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "codestream/entropyparser.hpp"
#include "codestream/predictorbase.hpp"
#include "tools/line.hpp"
///

/// Forwards
class Frame;
class LineCtrl;
class ByteStream;
class LineBitmapRequester;
class LineBuffer;
class LineAdapter;
class Scan;
///

/// class PredictiveScan
// This is the base class for all predictive scan types, it provides the
// services useful to implement them such that the derived classes can
// focus on the actual algorithm.
class PredictiveScan : public EntropyParser {
  //
  // Services for the derived classes.
protected:
  //
#if ACCUSOFT_CODE
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
  // Current pixel positions.
  ULONG                      m_ulX[4];
  ULONG                      m_ulY[4];
  //
  // MCU dimensions.
  UBYTE                      m_ucMCUWidth[4];
  UBYTE                      m_ucMCUHeight[4];
  //
  // The currently active predictors for the MCU processed.
  class PredictorBase       *m_pPredict[4];
  class PredictorBase       *m_pLinePredict[4];
  //
  // The predictor to use.
  UBYTE                      m_ucPredictor;
  //
  // The low bit for the point transform.
  UBYTE                      m_ucLowBit;
  //
  // Encoding a differential scan.
  bool                       m_bDifferential;
  //
  // The predictors used to encode or decode the scan.
  // Prediction always starts with the entry [0] and then
  // uses the down/right functions to advance the predictor.
  class PredictorBase       *m_pPredictors[4];
  //
#endif
  // Collect component information and install the component dimensions.
  void FindComponentDimensions(void);
  //
  // Clear the entire MCU
  void ClearMCU(struct Line **top);
  //
#if ACCUSOFT_CODE
  //
  // Advance to the next MCU to the right. Returns true if there
  // are more MCUs to the right.
  bool AdvanceToTheRight(void)
  {
    UBYTE i;
    bool  more;

    for(i = 0,more = true;i < m_ucCount;i++) {
      m_ulX[i]     += m_ucMCUWidth[i];
      m_pPredict[i] = m_pPredict[i]->MoveRight();
      if (m_ulX[i] >= m_ulWidth[i])
        more = false;
    }
    return more;
  }
  //
  // Advance to the next MCU to the bottom. Returns true if there are more
  // MCUs below.
  bool AdvanceToTheNextLine(struct Line **prev,struct Line **top)
  {
    UBYTE i;
    bool more;
    //
    // Advance to the next line.
    for(i = 0,more = true;i < m_ucCount;i++) {
      UBYTE cnt         = m_ucMCUHeight[i];
      m_ulX[i]          = 0;
      m_ulY[i]         += cnt;
      m_pLinePredict[i] = m_pLinePredict[i]->MoveDown();
      m_pPredict[i]     = m_pLinePredict[i];
      if (m_ulHeight[i] && m_ulY[i] >= m_ulHeight[i]) {
        more = false;
      } else do {
          prev[i] = top[i];
          if (top[i]->m_pNext) {
            top[i] = top[i]->m_pNext;
          }
        } while(--cnt);
    }
    
    return more;
  }
  //
#endif
  //
  // Build a predictive scan: This is not stand alone, let subclasses do that.
  PredictiveScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,
                 bool differential = false);
  //
  // Destroy a predictive scan.
  virtual ~PredictiveScan(void);
  //
  // Everything else goes into the derivced classes. 
  //
  // Parse a single MCU in this scan. Return true if there are more
  // MCUs in this row.
  virtual bool ParseMCU(void) = 0;
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void) = 0;
  //
  // Flush at the end of a restart interval
  // when writing out code. Reset predictors, check
  // for the correctness of the restart alignment.
  void FlushOnMarker(void);
  //
  // Restart after reading a full restart interval,
  // reset the predictors, check for the correctness
  // of the restart interval.
  void RestartOnMarker(void);
  //
  // Start making an optimization run to adjust the coefficients.
  virtual void StartOptimizeScan(class BufferCtrl *ctrl);
  //
  // Post the height of the image in lines. This happens when the DNL
  // marker is processed.
  virtual void PostImageHeight(ULONG height);
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
