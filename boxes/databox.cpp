/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
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
** This box implements a data container for refinement scans, as hidden
** in the APP11 markers.
**
** $Id: databox.cpp,v 1.10 2014/09/30 08:33:14 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/databox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "io/decoderstream.hpp"
///

/// DataBox::EncoderBufferOf
// Return the byte stream buffer where the encoder can drop the data
// on encoding.
ByteStream *DataBox::EncoderBufferOf(void)
{
  return OutputStreamOf();
}
///

/// DataBox::DecoderBufferOf
// Return the stream the decoder will decode from.
ByteStream *DataBox::DecoderBufferOf(void)
{
  return InputStreamOf();
}
///

/// DataBox::Flush
// Flush the buffered data of the box and create the markers.
void DataBox::Flush(class ByteStream *target,UWORD enumerator)
{
  WriteBoxContent(target,enumerator);
}
///
