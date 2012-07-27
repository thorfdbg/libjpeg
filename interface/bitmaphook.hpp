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
 * Definition of the bitmap hook class.
 * This hook function is used to pull data out of the
 * client to make it available to the jpeglib
 * 
 * $Id: bitmaphook.hpp,v 1.3 2012-06-02 10:27:14 thor Exp $
 *
 */

#ifndef BITMAPHOOK_HPP
#define BITMAPHOOK_HPP

/// Includes
#include "interface/types.hpp"
#include "interface/tagitem.hpp"
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "imagebitmap.hpp"
///

/// Forwards
struct JPG_Hook;
///

/// Design
/** Design
******************************************************************
** class BitMapHook						**
** Super Class:	none						**
** Sub Classes: none						**
** Friends:	none						**
******************************************************************

Using a JPG_Hook as its main ingredience, the bitmap hook defines
a call-back entry point that is called by the jpeglib as soon as
it either requires input data on encoding, or as it generated output
data to be placed into the client on decoding.

The required position information, i.e. which data has to be
delivered by the client, or which data is ready to be placed into
the client application, leaves the hook as a tag list that is
to be parsed by the client.

The client then has to deliver information where the image data has
to be placed, as the memory start position of the canvas, the
data type the pixels are stored within, the number of bytes each
pixel is wide, i.e. how many bytes one has to add to the address
of the current pixel to get to the pixel to the right, and the
modulo value, i.e. how many bytes one has to add to get to the
address of the pixel below the current pixel.

One special note about the pixel addressing, though: What the
bitmap hook expects as start address is always the base address
of the canvas, i.e. of pixel 0,0. Whether this pixel is actually
used by the current request is a different matter.

The library uses this pixel address, adds bytesperpixel * xmin
and bytesperrow * ymin to get the pixel address it is really
interested in. 

From the libraries point of view, the bitmap hook class
delivers this output data by means of an "ImageBitMap" structure.

* */
///

/// The bitmap hook class
// This class is mainly intented to pull bitmap type data out of the
// client.
class BitMapHook : public JObject {
  //
  struct JPG_Hook   *m_pHook; // Setup on creation, contains the jump-in location
  // 
  // The following keep default data for lazy applications:
  struct ImageBitMap m_DefaultImageLayout;
  //
  // Prepared tags for requesting usual bitmap tags.
  struct JPG_TagItem m_BitmapTags[25];
  //
  // Init the tags for the given canvas/slice coordinates.
  void Init(const struct JPG_TagItem *tags);
  //
public:
  //
  // Build a bitmap hook from a tag list (user parameters)
  // and the address for which this data is required.
  BitMapHook(const struct JPG_TagItem *tags);
  //
  // Only re-parse the tags to re-define the default bitmap layout.
  void ParseTags(const struct JPG_TagItem *tags);
  //
  void RequestClientData(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
			 const class Component *comp);
  //
  // Release the image portion from the client, called to signal the client
  // that all has gone well.
  void ReleaseClientData(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
			 const class Component *comp);
  //
};
///

///
#endif
