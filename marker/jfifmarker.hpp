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
