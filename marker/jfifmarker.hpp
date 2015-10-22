/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
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
** This class represents the JFIF marker, placed in APP0. 
** This is only used to indicate a JFIF file and is otherwise unused.
**
** $Id: jfifmarker.hpp,v 1.6 2014/09/30 08:33:17 thor Exp $
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
