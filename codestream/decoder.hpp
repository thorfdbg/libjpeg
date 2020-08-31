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
** $Id: decoder.hpp,v 1.17 2017/02/21 15:48:21 thor Exp $
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
  // Parse off the header, return the image in case
  // the header was parsed off completely.
  class Image *ParseHeaderIncremental(class ByteStream *io);
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
