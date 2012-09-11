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
** This is the base class for all predictive scan types, it provides the
** services useful to implement them such that the derived classes can
** focus on the actual algorithm.
**
** $Id: predictivescan.hpp,v 1.2 2012-09-11 13:30:00 thor Exp $
**
*/

#ifndef CODESTREAM_PREDICTIVESCAN_HPP
#define CODESTREAM_PREDICTIVESCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "codestream/entropyparser.hpp"
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
  // The predictor to use.
  UBYTE                      m_ucPredictor;
  //
  // The low bit for the point transform.
  UBYTE                      m_ucLowBit;
  //
  // The neutral color. Grey or zero for differential.
  LONG                       m_lNeutral;
  //
  // Disable prediction as if we were at the first line.
  // Used behind restart markers.
  bool                       m_bNoPrediction;
  //
  //
  // Encoding a differential scan.
  bool                       m_bDifferential;
  //
  // Collect component information and install the component dimensions.
  void FindComponentDimensions(void);
  //
  // Clear the entire MCU
  void ClearMCU(class Line **top);
  //
  // Get the prediction mode given a couple of flags.
  UBYTE PredictionMode(LONG x,LONG y) const
  {
    // prediction is turned off if : This is a differential line, or
    // the first sample, or the first sample after a restart marker.
    if (m_bDifferential || (x == 0 && y == 0)) 
      return 0;
    // Predict from left if this is on the first line or after a restart
    // marker.
    if (y == 0)
      return 1; // Predict from left.
    //
    // Predict from above at the start of the line.
    if (x == 0) 
      return 2; // Predict from above.
    return m_ucPredictor;
  }
  //
  // Predict a sample value depending on the prediction mode.
  // lp is the pointer to the current line, pp the one to the previous line.
  LONG PredictSample(UBYTE mode,UBYTE preshift,const LONG *lp,LONG *pp) const
  {
    switch(mode) {
    case 0: // No prediction. 
      return m_lNeutral >> preshift;
    case 1: // predict from left
      return lp[-1] >> preshift;
    case 2: // predict from top
      return pp[0] >> preshift;
    case 3: // predict from left-top
      return pp[-1] >> preshift;
    case 4: // linear interpolation
      return (lp[-1]  >> preshift) + (pp[0]  >> preshift) - (pp[-1] >> preshift);
    case 5: // linear interpolation with weight on A
      return (lp[-1]  >> preshift) + (((pp[0]  >> preshift) - (pp[-1] >> preshift)) >> 1);
    case 6: // linear interpolation with weight on B
      return (pp[0]  >> preshift) + (((lp[-1]  >> preshift) - (pp[-1] >> preshift)) >> 1);
    case 7: // Only between A and B
      return ((lp[-1] >> preshift) + (pp[0] >> preshift)) >> 1;
    default:
      assert(!"impossible prediction mode, did the check in the scan header fail?");
      return 0;
    }
  }  
  //
  // Advance to the next MCU to the right. Returns true if there
  // are more MCUs to the right.
  bool AdvanceToTheRight(void)
  {
    UBYTE i;
    bool  more;

    for(i = 0,more = true;i < m_ucCount;i++) {
      m_ulX[i] += m_ucMCUWidth[i];
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
      UBYTE cnt      = m_ucMCUHeight[i];
      m_ulX[i]       = 0;
      m_ulY[i]      += cnt;
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
};
///


///
#endif
