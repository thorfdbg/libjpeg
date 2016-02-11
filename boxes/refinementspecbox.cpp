/*
** This class represents the refinement specification box, carrying the
** number of refinement scans in the base and residual layer of the
** image.
**
** $Id: refinementspecbox.cpp,v 1.3 2014/09/30 08:33:15 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/refinementspecbox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// RefinementSpecBox::ParseBoxContent
bool RefinementSpecBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG v;
  
  if (boxsize != 1)
    JPG_THROW(MALFORMED_STREAM,"RefinementSpecBox::ParseBoxContent",
	      "Malformed JPEG stream - the size of the refinement spec box is incorrect");

  
  v = stream->Get();

  if ((v >> 4) > 4)
    JPG_THROW(MALFORMED_STREAM,"RefinementSpecBox::ParseBoxContent",
	      "Malformed JPEG stream - the number of refinement scans must be smaller or equal than four");

  m_ucBaseRefinementScans = v >> 4;

  if ((v & 0x0f) > 4)
    JPG_THROW(MALFORMED_STREAM,"RefinementSpecBox::ParseBoxContent",
	      "Malformed JPEG stream - the number of residual refinement scans must be smaller or equal than four");

  m_ucResidualRefinementScans = v & 0x0f;

  return true;
}
///

/// RefinementSpecBox::CreateBoxContent
// Create the contents of the refinement spec box.
bool RefinementSpecBox::CreateBoxContent(class MemoryStream *target)
{
  target->Put((m_ucBaseRefinementScans << 4) | m_ucResidualRefinementScans);

  return true;
}
///
