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
** This file provides the transformation from RGB to YCbCr
**
** $Id: colortrafo.hpp,v 1.7 2012-07-14 12:07:35 thor Exp $
**
*/

#ifndef COLORTRAFO_COLORTRAFO_HPP
#define COLORTRAFO_COLORTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
///

/// Class ColorTrafo
// The base class for all decorrelation transformation
class ColorTrafo : public JKeeper {
  //
public:
  // Number of bits preshifted for the color-transformed channels
  enum {
    COLOR_BITS = 4
  }; 
  //
protected:
  // The output data. Note that these are kept here only for one block
  // as this is all what is required for the DCT.
  LONG m_lY[64];
  LONG m_lCb[64];
  LONG m_lCr[64];
  LONG m_lA[64]; // additional or alpha.
  //
  // Pointer to the above array.
  LONG *m_ppBuffer[4];
  //
  // Encoding and decoding tone mapping lookup tables.
  // These tables are only populated when required.
  //
  // The decoding LUT that maps LDR to HDR.
  const UWORD *m_pusDecodingLUT[4];
  //
  // The encoding LUT that maps HDR to LDR.
  const UWORD *m_pusEncodingLUT[4];
  //
public:
  ColorTrafo(class Environ *env);
  //
  virtual ~ColorTrafo(void);
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift.
  // The rightshift is performed on the sample data before entering the color transformation
  // to allow an adjustment of the bit depth. Residual coding addresses then potentially
  // the errors created by this shift.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 LONG dcshift,LONG max) = 0;
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
			 LONG dcshift,LONG max) = 0;
  //
  // Return the input buffer or output buffer of the data as a [4][64] array.
  LONG *const *BufferOf(void) const
  {
    return m_ppBuffer;
  }
  //
  // Return the number of fractional bits this color transformation requires.
  virtual UBYTE FractionalBitsOf(void) const = 0;
  //
  // Return the external pixel type of this trafo.
  virtual UBYTE PixelTypeOf(void) const = 0;
  //
  // Define the encoding LUTs.
  void DefineEncodingTables(const UWORD *encoding[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_pusEncodingLUT[i] = encoding[i];
    }
  }
  //
  // Define the decoding LUTs.
  void DefineDecodingTables(const UWORD *decoding[4])
  {
    int i;
    for(i = 0;i < 4;i++) {
      m_pusDecodingLUT[i] = decoding[i];
    }
  }
};
///

///
#endif
