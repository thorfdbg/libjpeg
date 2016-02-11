/*
** This class represents the adobe color specification marker, placed
** in APP14. Used here to indicate the color space and to avoid a color
** transformation.
**
** $Id: adobemarker.cpp,v 1.9 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/adobemarker.hpp"
///

/// AdobeMarker::AdobeMarker
AdobeMarker::AdobeMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// AdobeMarker::~AdobeMarker
AdobeMarker::~AdobeMarker(void)
{
}
///

/// AdobeMarker::WriteMarker
// Write the marker to the stream.
void AdobeMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 5 + 2 + 2 + 2 + 1;
  const char *id = "Adobe";

  io->PutWord(len);

  // Identifier code: ASCII "Adobe"
  while(*id) {
    io->Put(*id);
    id++;
  }

  io->PutWord(100); // version
  io->PutWord(0);   // flags 0
  io->PutWord(0);   // flags 1
  io->Put(m_ucColorSpace);
}
///

/// AdobeMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void AdobeMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UWORD version;
  LONG color;

  if (len != 2 + 5 + 2 + 2 + 2 + 1)
    JPG_THROW(MALFORMED_STREAM,"AdobeMarker::ParseMarker","misformed Adobe marker");

  version = io->GetWord();
  if (version != 100) // Includes EOF
    JPG_THROW(MALFORMED_STREAM,"AdobeMarker::ParseMarker","Adobe marker version unrecognized");

  io->GetWord();
  io->GetWord(); // ignored.

  color  = io->Get();

  if (color < 0 || color > Last)
    JPG_THROW(MALFORMED_STREAM,"AdobeMarker::ParseMarker","Adobe color information unrecognized");

  m_ucColorSpace = color;
}
///
