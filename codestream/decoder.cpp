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
** This class parses the markers and holds the decoder together.
**
** $Id: decoder.cpp,v 1.27 2017/02/21 15:48:21 thor Exp $
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
    if (m_pImage->TablesOf()->ParseTablesIncremental(io,NULL) == false) {
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
    m_pImage->TablesOf()->ParseTablesIncrementalInit();
  }
  //
  return NULL;
}
///

/// Decoder::ParseTags
// Accept decoder options.
void Decoder::ParseTags(const struct JPG_TagItem *tags)
{
  if (tags->GetTagData(JPGTAG_MATRIX_LTRAFO,JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR) == 
      JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE) {
    if (m_pImage) {
      m_pImage->TablesOf()->ForceColorTrafoOff();
    }
  }
  if (tags->GetTagData(JPGTAG_IMAGE_LOSSLESSDCT,false)) {
    if (m_pImage) {
      m_pImage->TablesOf()->ForceIntegerDCT();
    }
  }
}
///
