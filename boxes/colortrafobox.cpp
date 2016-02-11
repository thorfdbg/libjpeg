/*
** This class represents multiple boxes that all contain a color
** transformation specification, or rather, a single index to a matrix.
**
** $Id: colortrafobox.cpp,v 1.3 2014/09/30 08:33:14 thor Exp $
**
*/


/// Includes
#include "boxes/box.hpp"
#include "boxes/colortrafobox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// ColorTrafoBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. Returns true in case the contents is
// parsed and the stream can go away.
bool ColorTrafoBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG v;

  if (boxsize != 1)
    JPG_THROW(MALFORMED_STREAM,"ColorTrafoBox::ParseBoxContent",
	      "Malformed JPEG stream - size of the color transformation box is invalid");

  v = stream->Get();

  m_ucTrafoIndex = v >> 4;
  
  if (v & 0x0f)
    JPG_THROW(MALFORMED_STREAM,"ColorTrafoBox::ParseBoxContent",
	      "Malformed JPEG stream - the reserved field is not zero");

  return true;
}
///

/// ColorTrafoBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns whether the box content is already complete and the stream
// can go away.
bool ColorTrafoBox::CreateBoxContent(class MemoryStream *target)
{
  target->Put(m_ucTrafoIndex << 4);

  return true;
}
///


    
