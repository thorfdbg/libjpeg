/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** This class represents the adobe color specification marker, placed
** in APP14. Used here to indicate the color space and to avoid a color
** transformation.
**
** $Id: adobemarker.hpp,v 1.8 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef MARKER_ADOBEMARKER_HPP
#define MARKER_ADOBEMARKER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class AdobeMarker
// This class collects color space information conforming to the
// Adobe APP14 marker.
class AdobeMarker : public JKeeper {
  //
public:
  // 
  // Color space specifications.
  enum EnumeratedColorSpace {
    YCbCr = 1,
    YCCK  = 2,
    None  = 0, // RGB or CMYK, depending on the channel count.
    Last  = 2
  };
  //
private:
  //
  // Stored decoded color space.
  UBYTE m_ucColorSpace;
  //
public:
  //
  AdobeMarker(class Environ *env);
  //
  ~AdobeMarker(void);
  //
  // Write the marker to the stream.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the adobe marker from the stream
  // This will throw in case the marker is not
  // recognized. The caller will have to catch
  // the exception.
  void ParseMarker(class ByteStream *io,UWORD len);
  //
  // Initialize the color space for this marker.
  void SetColorSpace(EnumeratedColorSpace spec)
  {
    m_ucColorSpace = spec;
  }
  //
  // Return the color information.
  UBYTE EnumeratedColorSpaceOf(void) const
  {
    return m_ucColorSpace;
  }
};
///

///
#endif
