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
** This class represents the EXIF marker, placed in APP1. This
** marker is currently only a dummy and not actually used. 
**
** $Id: exifmarker.cpp,v 1.7 2020/08/31 07:50:44 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/exifmarker.hpp"
///

/// EXIFMarker::EXIFMarker
EXIFMarker::EXIFMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// EXIFMarker::~EXIFMarker
EXIFMarker::~EXIFMarker(void)
{
}
///

/// EXIFMarker::WriteMarker
// Write the marker to the stream.
void EXIFMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 4 + 2 + 2 + 2 + 4 + 2 + 4;
  const char *id = "Exif";

  io->PutWord(len);

  // Identifier code: ASCII "Exif"
  while(*id) {
    io->Put(*id);
    id++;
  }
  io->Put(0); // This comes with two terminating zeros.
  io->Put(0); // This comes with two terminating zeros.

  // Now a regular little-endian tiff header follows
  io->Put(0x49);
  io->Put(0x49);

  // Tiff version: 42. The meaning of life and everything.
  io->Put(42);
  io->Put(0);

  // Tiff IFD offset. Place it at offset 8 from the start of
  // the marker.
  io->Put(8);
  io->Put(0);  
  io->Put(0);
  io->Put(0);

  // Here the first IFD starts. Number of entries: Zero.
  io->Put(0);
  io->Put(0);

  // And the offset to the next IFD follows. There is none.
  io->Put(0);
  io->Put(0);
  io->Put(0);
  io->Put(0);
}
///

/// EXIFMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void EXIFMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  if (len < 2 + 4 + 2 + 2 + 2 + 4 + 2 + 4)
    JPG_THROW(MALFORMED_STREAM,"EXIFMarker::ParseMarker","malformed EXIF marker");

  //
  // The exif header has already been parsed off.
  len     -= 2 + 4 + 2;
  // Skip the rest of the marker.
  if (len > 0)
    io->SkipBytes(len);
}
///
