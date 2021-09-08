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
** This class parses the markers and holds the decoder together.
**
** $Id: decoder.cpp,v 1.29 2021/09/08 10:30:06 thor Exp $
**
*/

/// Include
#include "codestream/decoder.hpp"
#include "io/bytestream.hpp"
#include "std/assert.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "codestream/image.hpp"

///

/// Decoder::Decoder
// Construct the decoder
Decoder::Decoder(class Environ *env)
  : JKeeper(env), m_pImage(NULL)
{
}
///

/// Decoder::~Decoder
// Delete the decoder
Decoder::~Decoder(void)
{
  delete m_pImage;
}
///

/// Decoder::ParseHeaderIncremental
// Parse off the header, return the image in case
// the header was parsed off completely.
class Image *Decoder::ParseHeaderIncremental(class ByteStream *io)
{
  
  if (m_pImage) {
    //
    // Continue parsing the header.
    // At this stage, we may not yet know what type of image we have, so
    // be conservative and allow also JPEG LS markers.
    if (m_pImage->TablesOf()->ParseTablesIncremental(io,NULL,false,true) == false) {
      //
      // Parsing the header is done, return the image. All remaining
      // parsing is done by the image.
      return m_pImage;
    }
  } else {
    LONG marker = io->GetWord();

    if (marker != 0xffd8) // SOI
      JPG_THROW(MALFORMED_STREAM,"Decoder::ParseHeader",
                "stream does not contain a JPEG file, SOI marker missing");

    m_pImage  = new(m_pEnviron) class Image(m_pEnviron);
    //
    // The checksum is not going over the headers but starts at the SOF.
    m_pImage->TablesOf()->ParseTablesIncrementalInit(false);
  }
  //
  return NULL;
}
///

/// Decoder::ParseTags
// Accept decoder options.
void Decoder::ParseTags(const struct JPG_TagItem *)
{
}
///
