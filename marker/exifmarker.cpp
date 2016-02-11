/*
** This class represents the EXIF marker, placed in APP1. This
** marker is currently only a dummy and not actually used. 
**
** $Id: exifmarker.cpp,v 1.6 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/exifmarker.hpp"
///

/// EXIFMarker::EXIFMarker
EXIFMarker::EXIFMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// EXIFMarker::~EXIFMarker
EXIFMarker::~EXIFMarker(void)
{
}
///

/// EXIFMarker::WriteMarker
// Write the marker to the stream.
void EXIFMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 4 + 2 + 2 + 2 + 4 + 2 + 4;
  const char *id = "Exif";

  io->PutWord(len);

  // Identifier code: ASCII "Exif"
  while(*id) {
    io->Put(*id);
    id++;
  }
  io->Put(0); // This comes with two terminating zeros.
  io->Put(0); // This comes with two terminating zeros.

  // Now a regular little-endian tiff header follows
  io->Put(0x49);
  io->Put(0x49);

  // Tiff version: 42. The meaning of life and everything.
  io->Put(42);
  io->Put(0);

  // Tiff IFD offset. Place it at offset 8 from the start of
  // the marker.
  io->Put(8);
  io->Put(0);  
  io->Put(0);
  io->Put(0);

  // Here the first IFD starts. Number of entries: Zero.
  io->Put(0);
  io->Put(0);

  // And the offset to the next IFD follows. There is none.
  io->Put(0);
  io->Put(0);
  io->Put(0);
  io->Put(0);
}
///

/// EXIFMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void EXIFMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  if (len < 2 + 4 + 2 + 2 + 2 + 4 + 2 + 4)
    JPG_THROW(MALFORMED_STREAM,"EXIFMarker::ParseMarker","misformed EXIF marker");

  //
  // The exif header has already been parsed off.
  len     -= 2 + 4 + 2;
  // Skip the rest of the marker.
  if (len > 0)
    io->SkipBytes(len);
}
///
