/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** $Id: encoder.hpp,v 1.16 2012-06-02 10:27:13 thor Exp $
**
*/

#ifndef CODESTREAM_ENCODER_HPP
#define CODESTREAM_ENCODER_HPP

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Forwards
class ByteStream;
class Quantization;
class HuffmanTable;
class Frame;
class Tables;
class Scan;
class Image;
struct JPG_TagItem;
///

/// class Encoder
// The default encoder. Nothing fancy, just
// uses the default tables and the default quantization
// settings. More to come.
class Encoder : public JKeeper {
  //
  // The image containing all the data
  class Image        *m_pImage;
  //
  // Side information.
  class Tables       *m_pTables;
  //
public:
  Encoder(class Environ *env);
  //
  ~Encoder(void);
  //
  // Create an image from the layout specified in the tags. See interface/parameters
  // for the available tags.
  class Image *CreateImage(const struct JPG_TagItem *tags);
};
///

///
#endif
