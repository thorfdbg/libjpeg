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
** This class represents the quantization tables.
**
** $Id: quantization.hpp,v 1.6 2012-07-26 19:17:35 thor Exp $
**
*/

#ifndef MARKER_QUANTIZATION_HPP
#define MARKER_QUANTIZATION_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class Quantization
// This class describes the quantization tables for lossless JPEG coding.
class Quantization : public JKeeper {
  //
  // The actual quantization tables. This marker can hold up to four of them.
  UWORD *m_pDelta[4];
  //
public:
  Quantization(class Environ *env);
  //
  ~Quantization(void);
  //
  // Write the DQT marker to the stream.
  void WriteMarker(class ByteStream *io);
  //
  // Parse off the contents of the DQT marker
  void ParseMarker(class ByteStream *io);
  //
  // Initialize the quantization table to the standard example
  // tables for quality q, q=0...100
  void InitDefaultTables(UBYTE quality,UBYTE hdrquality,bool colortrafo);
  //
  const UWORD *QuantizationTable(UBYTE idx) const
  {
    if (idx < 4)
      return m_pDelta[idx];
    return NULL;
  }
};
///

///
#endif
