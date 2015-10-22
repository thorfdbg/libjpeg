/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/
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


    
