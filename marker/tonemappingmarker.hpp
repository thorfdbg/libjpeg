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
** This class represents the APP9 marker carrying the tone mapping curve
** required to restore the HDR data from the LDR approximation.
**
** $Id: tonemappingmarker.hpp,v 1.3 2012-09-21 14:45:40 thor Exp $
**
*/

#ifndef MARKER_TONEMAPPINGMARKER_HPP
#define MARKER_TONEMAPPINGMARKER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class ToneMappingMarker
// This class collects color space information conforming to the
// tonemapping APP9 marker.
class ToneMappingMarker : public JKeeper {
  //
  // Linkage of markers: They pile up here.
  class ToneMappingMarker *m_pNext;
  //
  // The index of this table. Each component can pick an individual
  // curve by refering to this index.
  UBYTE                    m_ucIndex;
  //
  // The output bpp value of this tone mapping. 
  UBYTE                    m_ucDepth;
  //
  // The input bpp value of this tone mapping curve - this is the
  // number of bits spend internally in the JPEG representation before
  // cutting off the hidden bits.
  UBYTE                    m_ucInternalDepth;
  //
  // The tone mapping for decoding, i.e. generates output data (>8 bit) from
  // the 8bpp input data.
  UWORD                   *m_pusMapping;
  //
  // Inverse (encoding) tone mapping curve, if available.
  UWORD                   *m_pusInverseMapping;
  // 
  // Build the encoding tone mapper from the inverse mapping.
  void BuildInverseMapping(void);
  //
public:
  //
  ToneMappingMarker(class Environ *env);
  //
  ~ToneMappingMarker(void);
  //
  // Return the next marker.
  class ToneMappingMarker *NextOf(void) const
  {
    return m_pNext;
  }
  //
  // Return the next marker as lvalue.
  class ToneMappingMarker *&NextOf(void)
  {
    return m_pNext;
  }
  //
  // Parse the tone mapping marker from the stream
  // This will throw in case the marker is not
  // recognized. The caller will have to catch
  // the exception.
  void ParseMarker(class ByteStream *io,UWORD length);
  //
  // Return the index of this marker. This is used to
  // find the right tone mapping curve given the
  // component.
  UBYTE IndexOf(void) const
  {
    return m_ucIndex;
  }
  //
  // Return the number of internal bits spend for the table.
  UBYTE InternalBitsOf(void) const
  {
    return m_ucInternalDepth;
  }
  //
  // Return the external bit depth depth.
  UBYTE ExternalBitsOf(void) const
  {
    return m_ucDepth;
  }
  //
  // Return the (decoding) tone mapping curve.
  const UWORD *ToneMappingCurveOf(void) const
  {
    return m_pusMapping;
  }
  //
  //
  // Return the encoding tone mapping curve.
  const UWORD *EncodingCurveOf(void)
  {
    if (m_pusInverseMapping == NULL)
      BuildInverseMapping();

    return m_pusInverseMapping;
  }
  //
  // Install parameters - here the bpp value and the tone mapping curve.
  void InstallDefaultParameters(UBYTE idx,UBYTE bpp,UBYTE hiddenbits,const UWORD *curve);
  //
  // Write the marker.
  void WriteMarker(class ByteStream *io);
};
///

///
#endif
