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
** This class parses the markers and holds the decoder together.
**
** $Id: decoder.hpp,v 1.16 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CODESTREAM_DECODER_HPP
#define CODESTREAM_DECODER_HPP

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
class Quantization;
class HuffmanTable;
class Frame;
class Image;
class Tables;
///

/// class Decoder
class Decoder : public JKeeper {
  //
  // The image object
  class Image        *m_pImage;
  //
public:
  Decoder(class Environ *env);
  //
  ~Decoder(void);
  //
  // Parse off the header.
  class Image *ParseHeader(class ByteStream *io);
  //
  // Parse off the EOI marker at the end of the image.
  // Returns true if there are more scans in the file,
  // else false.
  bool ParseTrailer(class ByteStream *io);
  //
  // Parse off decoder parameters.
  void ParseTags(const struct JPG_TagItem *tags);
};
///

///
#endif
