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
** This class pulls blocks from the frame and reconstructs from those
** quantized block lines or encodes from them.
**
** $Id: blockbuffer.hpp,v 1.5 2012-06-02 10:27:13 thor Exp $
**
*/

#ifndef CONTROL_BLOCKBUFFER_HPP
#define CONTROL_BLOCKBUFFER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class DCT;
class UpsamplerBase;
class DownsamplerBase;
class ColorTrafo;
class QuantizedRow;
class ResidualBlockHelper;
///

/// class BlockBuffer
// This class pulls blocks from the frame and reconstructs from those
// quantized block lines or encodes from them.
class BlockBuffer : public JKeeper {
  //
  class Frame               *m_pFrame;
  //
protected:
  //
  // Dimensions of the frame.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Number of components.
  UBYTE                      m_ucCount;
  //
  // Next line to be processed.
  ULONG                     *m_pulY;
  //
  // Number of the topmost line currently represented by the 
  // quantizer buffer line.
  ULONG                     *m_pulCurrentY;
  //
  // The DCT for encoding or decoding, together with the quantizer.
  class DCT                **m_ppDCT; 
  //
  // Quantized rows: 
  // Top row (start of the image buffer),
  // current encoding or decoding position for the codestream
  // parsers, current encoding or decoding position for the user
  // color transformer.
  //
  // First quantized image data row
  class QuantizedRow       **m_ppQTop;
  //
  // First residual data row.
  class QuantizedRow       **m_ppRTop;
  //
  // Current position in stream parsing or writing.
  class QuantizedRow      ***m_pppQStream;
  //
  // Current position in stream parsing for the residual.
  class QuantizedRow      ***m_pppRStream;
  //
  // Current position in reconstruction or encoding,
  // going through the color transformation.
  // On decoding, the line in here has the Y-coordinate 
  // in m_ulReadyLines.
  class QuantizedRow      ***m_pppQImage;
  //
  // Current position for the residual image.
  class QuantizedRow      ***m_pppRImage;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
  //
public:
  //
  BlockBuffer(class Frame *frame);
  //
  virtual ~BlockBuffer(void); 
  //
  // Return a pointer to an array of DCT transformers.
  class DCT *const *DCTsOf(void) const
  {
    return m_ppDCT;
  }
  //
  // Return the current top MCU quantized line.
  class QuantizedRow *CurrentQuantizedRow(UBYTE comp)
  {
    assert(comp < m_ucCount);
    return *m_pppQStream[comp];
  }
  //
  // Return the current top row of the residuals.
  class QuantizedRow *CurrentResidualRow(UBYTE comp)
  {
    assert(comp < m_ucCount);
    if (m_pppRStream[comp])
      return *m_pppRStream[comp];
    return NULL;
  }
  //
  // Start a MCU scan by initializing the quantized rows for this row
  // in this scan.
  virtual bool StartMCUQuantizerRow(class Scan *scan);
  //
  // The same for a row of residuals.
  virtual bool StartMCUResidualRow(void);
  //
  // Scan-dependent residual start
  virtual bool StartMCUResidualRow(class Scan *scan);
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
    return false;
  } 
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    m_ulPixelHeight = lines;
  }
};
///

///
#endif
