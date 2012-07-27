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
 * Defininition of the ImageBitMap structure, used to specify
 * rectangular memory blocks for image representation.
 * 
 * $Id: imagebitmap.hpp,v 1.2 2012-06-02 10:27:14 thor Exp $
 *
 *
 */

#ifndef TOOLS_IMAGEBITMAP_HPP
#define TOOLS_IMAGEBITMAP_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/rectangle.hpp"
#include "tools/environment.hpp"
#include "std/stddef.hpp"
///

/// Design
/** Design
******************************************************************
** struct ImageBitmap						**
** Super Class:	none						**
** Sub Classes: none						**
** Friends:							**
******************************************************************

Defines a rectangular array of memory containing image data.

However, this image bitmap is more flexible than a matrix could
be, even though it supports very similar features. 

The first difference is that modulo values are counted in bytes
rather than in elements. This is mainly as convenience for the
client. Furthermore, it does not only provide "vertical"
modulo value (add it to the pixel address to get to the pixel
in the line below), but also a horizontal modulo to be added
to an address to advance by one pixel in horizontal direction.

The idea is here that this allows very easy addressing of
"interleaved" image architectures where red, green and blue
are contained in three (or four, plus alpha channel) continuous
bytes in memory.

A pixel type describes wether the data of a single pixel is
signed, unsigned, byte or word sized. It takes the coefficient 
type indicator as in bandnotation.hpp.

Furthermore, user data can be stored here.

Image bitmaps are the destination object of bitmap hooks. They
create and fill out ImageBitMaps by parsing the result tag list
of the user callback function. The image bitmap then enters
the color transformer for further operation.
* */
///

/// ImageBitMap
// The image bitmap is a small helper structure that indicates
// where to access a bitmap in memory, i.e. where to get the data
// from or where to place it.
struct ImageBitMap : public JObject {
  ULONG   ibm_ulWidth;         // width and height of the data 
  ULONG   ibm_ulHeight;
  BYTE    ibm_cBytesPerPixel;  // byte offset to get from one pixel to the next
  UBYTE   ibm_ucPixelType;     // type identifier as in bandnotation.hpp
  LONG    ibm_lBytesPerRow;    // byte offset to get down from one row to the next. 
  APTR    ibm_pData;           // pointer to the image data
  APTR    ibm_pUserData;       // an ID the client might use for whatever he likes
public:
  // Extract a smaller bitmap from
  // a larger one by giving a rectangle.
  // This will clip correctly to image bitmap coordinates
  // and shift ibm_Data accordingly.
  void ExtractBitMap(const struct ImageBitMap *source,const RectAngle<LONG> &rect);
  //
  // Define a bitmap to describe an image of the given dimension at the given rectangle.
  void DefineBitMap(APTR buffer,UBYTE type,const RectAngle<LONG> &rect);
  //
  // Define a bitmap of the given dimension with the given modulo value.
  void DefineBitMap(APTR buffer,UBYTE type,const RectAngle<LONG> &rect,ULONG samplesperrow);
  //
  // Zero out a bitmap such that it is no longer valid
  void Blank(void) 
  {
    ibm_ulWidth         = 0;
    ibm_ulHeight        = 0;
    ibm_cBytesPerPixel  = 0;
    ibm_lBytesPerRow    = 0;
    ibm_ucPixelType     = 0;
    ibm_pData           = NULL;
  }
  //
  // Return a pointer to the data at a given position.
  APTR At(ULONG x,ULONG y) const
  {
    if (ibm_ucPixelType == 0)
      return NULL; // Blank bitmaps keep blank
   
    assert(x < ibm_ulWidth && y < ibm_ulHeight);
    //
    return (((UBYTE *)(ibm_pData)) + (ptrdiff_t(ibm_cBytesPerPixel) * x) + (ptrdiff_t(ibm_lBytesPerRow) * y));
  }
  //
  // Extract by subsampling data from the given source bitmap with the subsampling
  // factors as indicated. The offsets define the position within the subsampled pixel
  // and shall be between 0..subsampling-1. Length, and modulo values get adjusted 
  // accordingly.
  // IMPORTANT! The caller must guarantee that the subsampling values are all positive
  // and not zero.
  void SubsampleFrom(const struct ImageBitMap *source,
		     UBYTE subx,UBYTE suby,UBYTE offsetx,UBYTE offsety);
  //
  // Advance the line of the data to the next larger Y position. Deliver false in case this
  // is not possible (because we went out of data already).
  bool NextLine(void)
  {
    if (ibm_ulHeight == 0)
      return false;
    ibm_pData = (((UBYTE *)(ibm_pData)) + ibm_lBytesPerRow); 
    if (--ibm_ulHeight == 0)
      return false;
    return true;
  }
  //
  // Initialize the bitmap to have a single object as source or target.
  void AsSinglePoint(ULONG width,ULONG height)
  {  
    ibm_ulWidth         = width;
    ibm_ulHeight        = height;
    ibm_cBytesPerPixel  = 0;
    ibm_lBytesPerRow    = 0;
  }
  // Similar, but from a rectangle.
  void AsSinglePoint(const RectAngle<LONG> &rect)
  {
    ibm_ulWidth         = rect.WidthOf();
    ibm_ulHeight        = rect.HeightOf();
    ibm_cBytesPerPixel  = 0;
    ibm_lBytesPerRow    = 0;
  } 
  //
  // Initialize the bitmap to have a single object as source or target,
  // keep the height unaltered.
  void AsSinglePoint(ULONG width)
  {  
    ibm_ulWidth         = width;
    ibm_cBytesPerPixel  = 0;
    ibm_lBytesPerRow    = 0;
  }
};
///

///
#endif
