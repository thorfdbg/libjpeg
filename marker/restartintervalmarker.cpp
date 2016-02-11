/*
** This class keeps the restart interval size in MCUs.
**
** $Id: restartintervalmarker.cpp,v 1.7 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/restartintervalmarker.hpp"
///

/// RestartIntervalMarker::RestartIntervalMarker
RestartIntervalMarker::RestartIntervalMarker(class Environ *env)
  : JKeeper(env), m_usRestartInterval(0)
{
}
///

/// RestartIntervalMarker::WriteMarker
// Write the marker (without the marker id) to the stream.
void RestartIntervalMarker::WriteMarker(class ByteStream *io) const
{
  io->PutWord(0x04); // size of the marker.
  io->PutWord(m_usRestartInterval);
}
///

/// RestartIntervalMarker::ParseMarker
// Parse the marker from the stream.
void RestartIntervalMarker::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len != 4)
    JPG_THROW(MALFORMED_STREAM,"RestartIntervalMarker::ParseMarker",
	      "DRI restart interval definition marker size is invalid");

  len = io->GetWord();
  if (len == ByteStream::EOF)
    JPG_THROW(UNEXPECTED_EOF,"RestartIntervalMarker::ParseMarker",
	      "DRI restart interval definition marker run out of data");

  m_usRestartInterval = UWORD(len);
}
///

