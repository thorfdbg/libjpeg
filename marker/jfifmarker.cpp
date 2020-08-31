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
** This class represents the JFIF, placed in APP0. 
** This is only used to indicate a JFIF file and is otherwise unused.
**
** $Id: jfifmarker.cpp,v 1.8 2020/08/31 07:50:44 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/jfifmarker.hpp"
///

/// JFIFMarker::JFIFMarker
JFIFMarker::JFIFMarker(class Environ *env)
  : JKeeper(env)
{
}
///

/// JFIFMarker::~JFIFMarker
JFIFMarker::~JFIFMarker(void)
{
}
///

/// JFIFMarker::WriteMarker
// Write the marker to the stream.
void JFIFMarker::WriteMarker(class ByteStream *io)
{
  UWORD len = 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1;
  const char *id = "JFIF";

  io->PutWord(len);

  // Identifier code: ASCII "JFIF"
  while(*id) {
    io->Put(*id);
    id++;
  }
  io->Put(0); // This comes with a terminating zero.

  // Version is 1.02.
  io->Put(1);
  io->Put(2);
  //
  io->Put(m_Unit);
  io->PutWord(m_usXRes);
  io->PutWord(m_usYRes);

  // Thumbnail size: No thumbnail
  io->Put(0);
  io->Put(0);
}
///

/// JFIFMarker::ParseMarker
// Parse the adobe marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void JFIFMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UBYTE unit;
  
  if (len < 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1)
    JPG_THROW(MALFORMED_STREAM,"JFIFMarker::ParseMarker","malformed JFIF marker");

  io->Get(); // version
  io->Get(); // revision
  //
  // Currently ignore.

  unit     = io->Get();
  if (unit > Centimeter)
    JPG_THROW(MALFORMED_STREAM,"JFIFMarker::ParseMarker","JFIF specified unit is invalid");

  m_Unit   = (ResolutionUnit)unit;
  //
  // Read the dimensions.
  m_usXRes = io->GetWord();
  m_usYRes = io->GetWord();

  len     -= 2 + 5 + 2 + 1 + 2 + 2;
  // Skip the rest of the marker.
  if (len > 0)
    io->SkipBytes(len);
}
///
