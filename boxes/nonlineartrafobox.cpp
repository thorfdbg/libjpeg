/*
** This class represents multiple boxes that all contain four non-linear
** transformation indices by referencing to either a parametric curve
** or a lookup table. It is used for multiple boxes.
**
** $Id: nonlineartrafobox.cpp,v 1.3 2014/09/30 08:33:15 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/nonlineartrafobox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// NonlinearTrafoBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. Returns true in case the contents is
// parsed and the stream can go away.
bool NonlinearTrafoBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG v;
  
  if (boxsize != 2)
    JPG_THROW(MALFORMED_STREAM,"NonlinearTrafoBox::ParseBoxContent",
	      "Malformed JPEG stream - the size of a non-linear transformation box is incorrect");

  
  v = stream->Get();
  m_ucTrafoIndex[0] = v >> 4;
  m_ucTrafoIndex[1] = v & 0x0f;
  
  v = stream->Get();
  m_ucTrafoIndex[2] = v >> 4;
  m_ucTrafoIndex[3] = v & 0x0f;

  return true;
}
///

/// NonlinearTrafoBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns whether the box content is already complete and the stream
// can go away.
bool NonlinearTrafoBox::CreateBoxContent(class MemoryStream *target)
{
  target->Put((m_ucTrafoIndex[0] << 4) | m_ucTrafoIndex[1]);
  target->Put((m_ucTrafoIndex[2] << 4) | m_ucTrafoIndex[3]);

  return true;
}
///
