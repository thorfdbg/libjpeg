/*
** This class represents the EXIF marker, placed in APP1. This
** marker is currently only a dummy and not actually used. 
**
** $Id: exifmarker.hpp,v 1.6 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef MARKER_EXIFMARKER_HPP
#define MARKER_EXIFMARKER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class EXIFMarker
// This class collects the EXIF information
class EXIFMarker : public JKeeper {
  //
  // Really nothing in it right now.
  //
public:
  //
  EXIFMarker(class Environ *env);
  //
  ~EXIFMarker(void);
  //
  // Write the marker to the stream.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the EXIF marker from the stream
  // This will throw in case the marker is not
  // recognized. The caller will have to catch
  // the exception.
  void ParseMarker(class ByteStream *io,UWORD len);
};
///

///
#endif
