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
** $Id: lslosslesstrafo.hpp,v 1.4 2012-09-28 10:53:49 thor Exp $
**
*/

#ifndef COLORTRAFO_LSLOSSLESSTRAFO_HPP
#define COLORTRAFO_LSLOSSLESSTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
///

/// Forwards
class Frame;
class LSColorTrafo;
///

/// Class LSLosslessTrafo
// This class implements the color transformation specified in 
// the JPEG-LS part 2 standard.
template<typename external,int count>
class LSLosslessTrafo : public ColorTrafo {
  //
  // The maximum value of the external sample value.
  LONG    m_lMaxTrans;
  //
  // The modulo of the input = maxtrans + 1
  LONG    m_lModulo;
  //
  // The offset when adding to components.
  LONG    m_lOffset;
  //
  // The near value on encoding.
  LONG    m_lNear;
  //
  // On decoding, the component indices (not labels!) that
  // are used as input, in the order they appear. This array,
  // when using the index i, delivers the component index j
  // used as i'th input to the transformation.
  UBYTE   m_ucInternal[count];
  //
  // The inverse of the above array, given the internal color
  // transformation index, delivers the component index (not
  // label) it maps to.
  UBYTE   m_ucInverse[count];
  //
  // The right shift values.
  UBYTE   m_ucRightShift[count];
  //
  // Subtract or add to the base component? If
  // true, the output is subtracted, otherwise
  // added. Also, if false, the half offset is added.
  bool    m_bCentered[count];
  //
  // Matrix multipliers. The first index
  // runs over the output (external) components, the
  // second over the internal components.
  UWORD   m_usMatrix[count][count-1];
  //
public:
  LSLosslessTrafo(class Environ *env);
  //
  virtual ~LSLosslessTrafo(void);
  //
  // Install the transformation from an LSColorTrafo marker, one of
  // the JPEG LS extensions markers.
  void InstallMarker(const class LSColorTrafo *marker,const class Frame *frame);
  //
  // Transform a block from RGB to YCbCr. Input are the three image bitmaps
  // already clipped to the rectangle to transform, the coordinate rectangle to use
  // and the level shift.
  virtual void RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
			 LONG dcshift,LONG max);
  //
  // Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
  // shift.
  virtual void YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
			 LONG dcshift,LONG max);
  //
  // Return the number of fractional bits this color transformation requires.
  // None, this is integer to integer.
  virtual UBYTE FractionalBitsOf(void) const
  {
    return 0;
  }
  //
  // Return the pixel type of this transformer.
  virtual UBYTE PixelTypeOf(void) const
  {
    return TypeTrait<external>::TypeID;
  }
};
///

///
#endif
