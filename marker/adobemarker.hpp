/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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
