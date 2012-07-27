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
** This class pulls single lines from the frame and reconstructs
** them from the codestream. Only the lossless scheme uses this buffer
** organization.
**
** $Id: linebuffer.hpp,v 1.6 2012-06-02 10:27:13 thor Exp $
**
*/

#ifndef CONTROL_LINEBUFFER_HPP
#define CONTROL_LINEBUFFER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class DCT;
class UpsamplerBase;
class DownsamplerBase;
class ColorTrafo;
struct Line;
///

/// class LineBuffer
// This class pulls single lines from the frame and reconstructs
// them from the codestream. Only the lossless scheme uses this buffer
// organization.
class LineBuffer : public JKeeper {
  //
  // The frame this is part of.
  class Frame               *m_pFrame;
  //
protected:
  //
  // Dimensions of the frame.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Number of components in the frame.
  UBYTE                      m_ucCount;
  //
  // Next line to be processed.
  ULONG                     *m_pulY;
  //
  // Number of the topmost line.
  ULONG                     *m_pulCurrentY;
  //
  // Number of pixels allocated per component.
  ULONG                     *m_pulWidth;
  //
  // Length of a line in valid pixels.
  ULONG                     *m_pulEnd;
  //
  // Lines: 
  // Top row (start of the image buffer),
  // current encoding or decoding position for the codestream
  // parsers, current encoding or decoding position for the user
  // color transformer.
  //
  // First quantized image data row
  struct Line              **m_ppTop;
  //
  // Current position in stream parsing or writing.
  struct Line             ***m_pppCurrent;
  //
  // Previous line, required for the predictor.
  struct Line              **m_ppPrev;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
public:
  //
  LineBuffer(class Frame *frame);
  //
  virtual ~LineBuffer(void); 
  //
  // Return the current top MCU quantized line.
  struct Line *CurrentLineOf(UBYTE comp)
  {
    assert(comp < m_ucCount);
    return *m_pppCurrent[comp];
  }
  //
  // Return the previous line on top of the current.
  struct Line *PreviousLineOf(UBYTE comp)
  {
    assert(comp < m_ucCount);
    return m_ppPrev[comp];
  }
  //
  // Return the current Y coordinate.
  ULONG CurrentYOf(UBYTE comp) const
  {
    assert(comp < m_ucCount);
    return m_pulCurrentY[comp];
  }

  //
  // Start a MCU scan by initializing the quantized rows for this row
  // in this scan.
  virtual bool StartMCUQuantizerRow(class Scan *scan);
  //
  // The same for a row of residuals.
  virtual bool StartMCUResidualRow(void);
  //
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(const struct RectangleRequest *rr) const;
  //
  // Make sure to reset the block control to the
  // start of the scan for the indicated components in the scan, 
  // required after collecting the statistics for this scan.
  virtual void ResetToStartOfScan(class Scan *scan);
  //
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const
  {
    return true;
  }
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    m_ulPixelHeight = lines;
  }
  //
  // Define a single 8x8 block starting at the x offset and the given
  // line, taking the input 8x8 buffer.
  void DefineRegion(LONG x,struct Line *line,const LONG *buffer,UBYTE comp);
  //
  // Define a single 8x8 block starting at the x offset and the given
  // line, taking the input 8x8 buffer.
  void FetchRegion(LONG x,const struct Line *line,LONG *buffer); 
};
///

///
#endif
