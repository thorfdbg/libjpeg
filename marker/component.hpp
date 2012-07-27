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
** This class represents a single component.
**
** $Id: component.hpp,v 1.12 2012-06-02 10:27:14 thor Exp $
**
*/

#ifndef MARKER_COMPONENT_HPP
#define MARKER_COMPONENT_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
///

/// Forwards
class ByteStream;
///

/// class Component
// This class represents a single component
class Component : public JKeeper {
  //
  // The component number from zero up.
  UBYTE m_ucIndex;
  //
  // The component identifier
  UBYTE m_ucID;
  //
  // The horizontal subsampling factor.
  UBYTE m_ucMCUWidth;
  //
  // The vertical subsampling factor.
  UBYTE m_ucMCUHeight;
  //
  // Subsampling factors in X and Y direction.
  UBYTE m_ucSubX,m_ucSubY;
  //
  // The quantization table index to use for quantization.
  UBYTE m_ucQuantTable;
  //
  // The bit precision of this component.
  UBYTE m_ucPrecision;
  //
public:
  Component(class Environ *env,UBYTE idx,UBYTE prec,UBYTE subx = 1,UBYTE suby = 1);
  //
  ~Component(void);
  //
  // Parse off parts of the frame marker.
  void ParseMarker(class ByteStream *io);
  //
  // Write the contents of the component to the marker.
  void WriteMarker(class ByteStream *io);
  //
  // Compute the subsampling factors for this component. 
  // Requires the maximum MCU size.
  void SetSubsampling(UBYTE maxwidth,UBYTE maxheight)
  {
    if (maxwidth % m_ucMCUWidth != 0 || maxheight % m_ucMCUHeight) {
      JPG_THROW(NOT_IMPLEMENTED,"Component::SetSubsampling",
		"non-integer subsampling factors are not supported by this implementation, sorry");
    }
    m_ucSubX = maxwidth  / m_ucMCUWidth;
    m_ucSubY = maxheight / m_ucMCUHeight;
  }
  //
  // Compute the MCU dimensions from the subsampling factors and
  // the smallest common multiple of all subsampling factors.
  void SetMCUSize(UBYTE maxwidth,UBYTE maxheight)
  {
    m_ucMCUWidth  = maxwidth  / m_ucSubX;
    m_ucMCUHeight = maxheight / m_ucSubY;
  }
  //
  // Return the component ID. This is arbitrary and
  // only required to identify components.
  UBYTE IDOf(void) const
  {
    return m_ucID;
  }
  //
  // Return the component index. Counts from zero up.
  UBYTE IndexOf(void) const
  {
    return m_ucIndex;
  }
  //
  // Return the MCU width in blocks.
  UBYTE MCUWidthOf(void) const
  {
    return m_ucMCUWidth;
  }
  //
  // Return the MCU height in blocks
  UBYTE MCUHeightOf(void) const
  {
    return m_ucMCUHeight;
  }
  //
  // Return the subsampling in X dimension.
  UBYTE SubXOf(void) const
  {
    return m_ucSubX;
  }
  //
  // Return the subsampling in Y dimension.
  UBYTE SubYOf(void) const
  {
    return m_ucSubY;
  }
  //
  // Return the quantizer responsible for this component.
  UBYTE QuantizerOf(void) const
  {
    return m_ucQuantTable;
  }
  //
  // Bit precision of the component.
  UBYTE PrecisionOf(void) const
  {
    return m_ucPrecision;
  }
  //
  // Install the component label.
  void SetComponentID(UBYTE id)
  {
    m_ucID = id;
  }
  //
  // Install the component quantizer index.
  void SetQuantizer(UBYTE quant)
  {
    m_ucQuantTable = quant;
  }
};
///

///
#endif
