/*
** This class keeps a simple checksum over the data in the legacy codestream
** thus enabling decoders to check whether the data has been tampered with.
**
** $Id: checksumbox.cpp,v 1.4 2014/09/30 08:33:14 thor Exp $
**
*/

/// Includes
#include "boxes/checksumbox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "tools/checksum.hpp"
///

/// ChecksumBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. Returns true in case the contents is
// parsed and the stream can go away.
bool ChecksumBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG pack1,pack2;
  
  if (boxsize != 4)
    JPG_THROW(MALFORMED_STREAM,"ChecksumBox::ParseBoxContent",
	      "Malformed JPEG stream, the checksum box size is invalid");
  
  pack1 = stream->GetWord();
  pack2 = stream->GetWord();
  
  m_ulCheck = ULONG(pack1 << 16) | ULONG(pack2);

  return true;
}
///

/// ChecksumBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns whether the box content is already complete and the stream
// can go away.
bool ChecksumBox::CreateBoxContent(class MemoryStream *target)
{
  target->PutWord(m_ulCheck >> 16);
  target->PutWord(m_ulCheck);

  return true;
}
///

/// ChecksumBox::InstallChecksum
// Install the value from the checksum into the checksum box.
void ChecksumBox::InstallChecksum(const class Checksum *check)
{
  m_ulCheck = check->ValueOf();
}
///

  
  
