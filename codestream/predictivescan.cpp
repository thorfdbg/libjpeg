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
** $Id: predictivescan.cpp,v 1.2 2012-09-11 13:30:00 thor Exp $
**
*/

/// Includes
#include "codestream/predictivescan.hpp"
#include "io/bytestream.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "tools/line.hpp"
///

/// PredictiveScan::PredictiveScan
PredictiveScan::PredictiveScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,bool differential)
  : EntropyParser(frame,scan), m_pLineCtrl(NULL), m_ucPredictor(predictor), m_ucLowBit(lowbit),
    m_bDifferential(differential)
{ 
  m_ucCount = scan->ComponentsInScan();
}
///

/// PredictiveScan::FindComponentDimensions
// Collect the component information.
void PredictiveScan::FindComponentDimensions(void)
{ 
  int i;

  m_ulPixelWidth  = m_pFrame->WidthOf();
  m_ulPixelHeight = m_pFrame->HeightOf();
  if (m_bDifferential)
    m_lNeutral    = 0;
  else 
    m_lNeutral    = ((1L << m_pFrame->PrecisionOf()) >> 1) << FractionalColorBitsOf();
  
  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE subx            = comp->SubXOf();
    UBYTE suby            = comp->SubYOf();
    
    m_ulWidth[i]          = (m_ulPixelWidth  + subx - 1) / subx;
    m_ulHeight[i]         = (m_ulPixelHeight + suby - 1) / suby; 
    m_ucMCUWidth[i]       = comp->MCUWidthOf();
    m_ucMCUHeight[i]      = comp->MCUHeightOf();
    m_ulX[i]              = 0;
    m_ulY[i]              = 0;
  }

  if (m_ucCount == 1) {
    m_ucMCUWidth[0]  = 1;
    m_ucMCUHeight[0] = 1;
  }
  
}
///

/// PredictiveScan::~PredictiveScan
PredictiveScan::~PredictiveScan(void)
{
}
///

/// PredictiveScan::ClearMCU
// Clear the entire MCU
void PredictiveScan::ClearMCU(class Line **top)
{ 
  for(int i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    struct Line *line     = top[i];
    UBYTE ym              = comp->MCUHeightOf();
    //
    do {
      LONG *p = line->m_pData;
      LONG *e = line->m_pData + m_ulWidth[i];
      do {
	*p = m_lNeutral;
      } while(++p < e);

      if (line->m_pNext)
	line = line->m_pNext;
    } while(--ym);
  }
}
///
