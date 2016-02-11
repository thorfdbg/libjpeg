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
