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
** This class contains the marker that defines the JPEG LS thresholds.
**
** $Id: thresholds.hpp,v 1.3 2012-07-19 18:59:12 thor Exp $
**
*/

#ifndef MARKER_THRESHOLDS_HPP
#define MARKER_THRESHOLDS_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// Thresholds
class Thresholds : public JKeeper {
  //
  // Parameters of the JPEG LS coder as defined by C.2.4.1.1
  //
  // Maximum sample value.
  UBYTE m_usMaxVal;
  //
  // Bucket threshold 1
  UBYTE m_usT1;
  //
  // Bucket threshold 2
  UBYTE m_usT2;
  //
  // Bucket threshold 3
  UBYTE m_usT3;
  //
  // The statistics reset value.
  UBYTE m_usReset;
  //
public:
  Thresholds(class Environ *env);
  //
  ~Thresholds(void);
  //
  // Write the marker contents to a LSE marker.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the marker contents of a LSE marker.
  // marker length and ID are already parsed off.
  void ParseMarker(class ByteStream *io,UWORD len);
  //
  // Return the maximum sample value.
  UWORD MaxValOf(void) const
  {
    return m_usMaxVal;
  }
  //
  // Return the T1 value.
  UWORD T1Of(void) const
  {
    return m_usT1;
  }
  //
  // Return the T2 value.
  UWORD T2Of(void) const
  {
    return m_usT2;
  }
  //
  // Return the T3 value.
  UWORD T3Of(void) const
  {
    return m_usT3;
  }
  //
  // Return the Reset interval.
  UWORD ResetOf(void) const
  {
    return m_usReset;
  }
  //
  // Install the defaults for a given bits per pixel value
  // and the quality/near value.
  void InstallDefaults(UBYTE bpp,UWORD near);
};
///

///
#endif
