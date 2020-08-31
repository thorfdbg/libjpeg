/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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
** This box keeps all the information for opacity coding, the alpha mode
** and the matte color.
**
** $Id: alphabox.cpp,v 1.3 2016/01/22 10:09:54 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/alphabox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// AlphaBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. Returns true in case the contents is
// parsed and the stream can go away.
bool AlphaBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  UBYTE mode1,mode2;
  
  if (boxsize != 2 + 4 * 2)
    JPG_THROW(MALFORMED_STREAM,"AlphaBox::ParseBoxContent",
              "Malformed JPEG stream, the alpha channel composition box size is invalid");

  mode1 = stream->Get();
  mode2 = stream->Get();

  if ((mode1 >> 4) > MatteRemoval)
    JPG_THROW(MALFORMED_STREAM,"AlphaBox::ParseBoxContent",
              "Malformed JPEG stream, the alpha composition method is invalid");

  m_ucAlphaMode = (mode1 >> 4);

  if (((mode1 & 0x0f) != 0) || mode2 != 0)
    JPG_THROW(MALFORMED_STREAM,"AlphaBox::ParseBoxContent",
              "Malformed JPEG stream, found invalid values for reserved fields");

  m_ulMatteRed   = stream->GetWord();
  m_ulMatteGreen = stream->GetWord();
  m_ulMatteBlue  = stream->GetWord();
  
  stream->GetWord();

  return true;
}
///

/// AlphaBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns whether the box content is already complete and the stream
// can go away.
bool AlphaBox::CreateBoxContent(class MemoryStream *target)
{
  //
  // First write the mode bytes.
  target->Put(m_ucAlphaMode << 4);
  target->Put(0);
  //
  // Write the matte colors.
  target->PutWord(m_ulMatteRed);
  target->PutWord(m_ulMatteGreen);
  target->PutWord(m_ulMatteBlue);
  target->PutWord(0);

  return true;
}
///
