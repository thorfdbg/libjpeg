/*
** This class represents the JFIF, placed in APP0. 
** This is only used to indicate a JFIF file and is otherwise unused.
**
** $Id: jfifmarker.cpp,v 1.7 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/jfifmarker.hpp"
///

/// JFIFMarker::JFIFMarker
JFIFMarker::JFIFMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// JFIFMarker::~JFIFMarker
JFIFMarker::~JFIFMarker(void)
{
}
///

/// JFIFMarker::WriteMarker
// Write the marker to the stream.
void JFIFMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1;
  const char *id = "JFIF";

  io->PutWord(len);

  // Identifier code: ASCII "JFIF"
  while(*id) {
    io->Put(*id);
    id++;
  }
  io->Put(0); // This comes with a terminating zero.

  // Version is 1.02.
  io->Put(1);
  io->Put(2);
  //
  io->Put(m_Unit);
  io->PutWord(m_usXRes);
  io->PutWord(m_usYRes);

  // Thumbnail size: No thumbnail
  io->Put(0);
  io->Put(0);
}
///

/// JFIFMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void JFIFMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UBYTE unit;
  
  if (len < 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1)
    JPG_THROW(MALFORMED_STREAM,"JFIFMarker::ParseMarker","misformed JFIF marker");

  io->Get(); // version
  io->Get(); // revision
  //
  // Currently ignore.

  unit     = io->Get();
  if (unit > Centimeter)
    JPG_THROW(MALFORMED_STREAM,"JFIFMarker::ParseMarker","JFIF specified unit is invalid");

  m_Unit   = (ResolutionUnit)unit;
  //
  // Read the dimensions.
  m_usXRes = io->GetWord();
  m_usYRes = io->GetWord();

  len     -= 2 + 5 + 2 + 1 + 2 + 2;
  // Skip the rest of the marker.
  if (len > 0)
    io->SkipBytes(len);
}
///
