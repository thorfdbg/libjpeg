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
** This class represents the JFIF marker, placed in APP0. 
** This is only used to indicate a JFIF file and is otherwise unused.
**
** $Id: jfifmarker.hpp,v 1.1 2012-06-10 21:46:05 thor Exp $
**
*/

#ifndef MARKER_JFIFMARKER_HPP
#define MARKER_JFIFMARKER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class JFIFMarker
// This class collects the JFIF information
class JFIFMarker : public JKeeper {
  //
  enum ResolutionUnit {
    Unspecified = 0,
    Inch        = 1,
    Centimeter  = 2
  }     m_Unit;
  //
  // Resoluton of the image.
  UWORD m_usXRes;
  UWORD m_usYRes;
  //
public:
  //
  JFIFMarker(class Environ *env);
  //
  ~JFIFMarker(void);
  //
  // Write the marker to the stream.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the JFIF marker from the stream
  // This will throw in case the marker is not
  // recognized. The caller will have to catch
  // the exception.
  void ParseMarker(class ByteStream *io,UWORD len);
  //
  //
  // Define the image resolution in pixels per inch.
  void SetImageResolution(UWORD xres,UWORD yres)
  {
    m_usXRes  = xres;
    m_usYRes  = yres;
    m_Unit    = Inch;
  }
};
///

///
#endif
