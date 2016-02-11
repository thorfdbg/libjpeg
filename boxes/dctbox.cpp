/*
** This class represents multiple boxes that all contain specifications
** on the DCT process. These boxes are only used by part-8.
**
** $Id: dctbox.cpp,v 1.3 2014/09/30 08:33:14 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/dctbox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// DCTBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. Returns true in case the contents is
// parsed and the stream can go away.
bool DCTBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG v;
  UBYTE t;

  if (boxsize != 1)
    JPG_THROW(MALFORMED_STREAM,"DCTBox::ParseBoxContent","Malformed JPEG stream - size of the DCT box is incorrect");

  v = stream->Get();

  t = v >> 4; // The DCT type.

  if (t != FDCT && t != IDCT && t != Bypass)
    JPG_THROW(MALFORMED_STREAM,"DCTBox::ParseBoxContent","Malformed JPEG stream - invalid DCT specified");

  m_ucDCTType = t;

  t = v & 0x0f;

  if (t != 0 && t != 1)
    JPG_THROW(MALFORMED_STREAM,"DCTBox::ParseBoxContent","Malformed JPEG stream - invalid noise shaping specified");

  if (t && m_ucDCTType != Bypass)
    JPG_THROW(MALFORMED_STREAM,"DCTBox::ParseBoxContent",
	      "Malformed JPEG stream - cannot enable noise shaping without bypassing the DCT");

  m_bNoiseShaping = (t != 0)?true:false;

  return true;
}
///

/// DCTBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns whether the box content is already complete and the stream
// can go away.
bool DCTBox::CreateBoxContent(class MemoryStream *target)
{
  assert(!(m_bNoiseShaping && m_ucDCTType != Bypass));

  target->Put((m_ucDCTType << 4) | (m_bNoiseShaping?1:0));

  return true;
}
///
