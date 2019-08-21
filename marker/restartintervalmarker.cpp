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
    Accusoft. (C) 2019 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select beween these two options.

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
** This class keeps the restart interval size in MCUs.
**
** $Id: restartintervalmarker.cpp,v 1.7 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "marker/restartintervalmarker.hpp"
///

/// RestartIntervalMarker::RestartIntervalMarker
RestartIntervalMarker::RestartIntervalMarker(class Environ *env)
  : JKeeper(env), m_usRestartInterval(0)
{
}
///

/// RestartIntervalMarker::WriteMarker
// Write the marker (without the marker id) to the stream.
void RestartIntervalMarker::WriteMarker(class ByteStream *io) const
{
  io->PutWord(0x04); // size of the marker.
  io->PutWord(m_usRestartInterval);
}
///

/// RestartIntervalMarker::ParseMarker
// Parse the marker from the stream.
void RestartIntervalMarker::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len != 4)
    JPG_THROW(MALFORMED_STREAM,"RestartIntervalMarker::ParseMarker",
              "DRI restart interval definition marker size is invalid");

  len = io->GetWord();
  if (len == ByteStream::EOF)
    JPG_THROW(UNEXPECTED_EOF,"RestartIntervalMarker::ParseMarker",
              "DRI restart interval definition marker run out of data");

  m_usRestartInterval = UWORD(len);
}
///

