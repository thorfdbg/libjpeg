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
 * Definition of the ImageBitMap structure, used to specify
 * rectangular memory blocks for image representation.
 * 
 * $Id: imagebitmap.cpp,v 1.11 2022/06/14 06:18:30 thor Exp $
 *
 *
 */

/// Includes
#include "interface/imagebitmap.hpp"
#include "tools/traits.hpp"
#include "std/stddef.hpp"
///

/// ImageBitMap::ExtractBitMap
//
void ImageBitMap::ExtractBitMap(const struct ImageBitMap *source,const RectAngle<LONG> &rect)
{
  LONG xshift,yshift;
  ULONG width  = 0,height  = 0;
  ULONG iwidth = 0,iheight = 0;

  // just copy the bitmap data over, make modifications
  // later. Not required if source and destination are identically.
  if (this != source)
    *this = *source;

  // compute the left offset of the new bitmap within the
  // old bitmap. Negative rectangle coordiates are clipped
  // away.
  xshift = (rect.ra_MinX<0)?(0):(rect.ra_MinX);
  yshift = (rect.ra_MinY<0)?(0):(rect.ra_MinY);

  // compute the dimension of the remaining
  // rectangle if we clip away the overlapping
  // edges to the left
  if (xshift <= rect.ra_MaxX) {
    width  = rect.ra_MaxX - xshift + 1;
    if (source->ibm_ulWidth > ULONG(xshift))
      iwidth = source->ibm_ulWidth - xshift;
    else
      iwidth = 0; // clipped off completely
  }

  if (yshift <= rect.ra_MaxY) {
    height  = rect.ra_MaxY - yshift + 1;
    if (source->ibm_ulHeight > ULONG(yshift))
      iheight = source->ibm_ulHeight - yshift;
    else
      iheight = 0; // clipped off completely
  }
  
  // clip this again to the width of the parent bitmap 
  // to make sure that we do not write past the right or
  // bottom edge.

  if (width > iwidth)
    width = iwidth;

  if (height > iheight)
    height = iheight;

  // check whether the source bitmap is *blank*, i.e. has
  // a NULL data pointer. If so, make the child blank as well
  // and null out width and height for consistency
  if (source->ibm_ucPixelType && width && height && source->ibm_pData) {
    ibm_pData = ((UBYTE *)source->ibm_pData) + 
      xshift * ptrdiff_t(source->ibm_cBytesPerPixel) +
      yshift * ptrdiff_t(source->ibm_lBytesPerRow);

  } else {
    ibm_pData          = NULL;
    ibm_cBytesPerPixel = 0;
    ibm_lBytesPerRow   = 0;
    // ibm_ucPixelType = 0;  // FIX: Leave the pixel type valid such that further
    // requests can still find that data can be extracted from here, though the
    // result will always be blank.
    // Special rule: If the source bitmap is indicated as blank, do not
    // define constraints concerning its dimensions.
    if (source->ibm_ucPixelType == 0) {
      width  = MAX_LONG;
      height = MAX_LONG;
    }
  }
  //
  ibm_ulWidth  = width;  // the component transformer needs width
  ibm_ulHeight = height; // and height to be initialized even for blank BMs.
}
///

/// ImageBitMap::SubsampleFrom
// Extract by subsampling data from the given source bitmap with the subsampling
// factors as indicated. The offsets define the position within the subsampled pixel
// and shall be between 0..subsampling-1. Length, and modulo values get adjusted 
// accordingly.
// IMPORTANT! The caller must guarantee that the subsampling values are all positive
// and not zero.
void ImageBitMap::SubsampleFrom(const struct ImageBitMap *src,UBYTE subx,UBYTE suby,UBYTE xoffset,UBYTE yoffset)
{
  // For the following, note that the user IBM's are anchored at the canvas offset (0,0) and not at the
  // tile or requested rectangle.
  if (src->ibm_ulWidth > xoffset) {
    ibm_ulWidth         = 1 + (src->ibm_ulWidth  - xoffset - 1) / subx;
  } else {
    ibm_ulWidth         = 0;
  }
  if (src->ibm_ulHeight > yoffset) {    
    ibm_ulHeight        = 1 + (src->ibm_ulHeight - yoffset - 1) / suby;
  } else {
    ibm_ulHeight        = 0;
  }
  if (src->ibm_ucPixelType && ibm_ulWidth > 0 && ibm_ulHeight > 0) {
    ibm_pData         = ((UBYTE *)(src->ibm_pData))
      + xoffset * ptrdiff_t(src->ibm_cBytesPerPixel) + yoffset * ptrdiff_t(src->ibm_lBytesPerRow);
  } else {
    ibm_pData         = NULL;
  }
  ibm_cBytesPerPixel  = src->ibm_cBytesPerPixel * subx;
  ibm_lBytesPerRow    = src->ibm_lBytesPerRow   * suby;
  ibm_ucPixelType     = src->ibm_ucPixelType;
}
///

/// ImageBitMap::DefineBitMap
// Define a bitmap to describe an image of the given dimension at the given rectangle.
void ImageBitMap::DefineBitMap(APTR buffer,UBYTE type,const RectAngle<LONG> &rect)
{
  UBYTE pixelsize    = type & CTYP_SIZE_MASK;
  // Initialize the internal bitmap (that the traditional color transformer maps to) to
  // the internal buffer of the client data. Note that libjpeg always
  // anchores the user bitmaps at zero,zero, so shift it.
  ibm_ulWidth        = rect.ra_MaxX + 1; // note that rect is always inclusive
  ibm_ulHeight       = rect.ra_MaxY + 1;
  // Fixpoint types.
  ibm_ucPixelType    = type;
  ibm_cBytesPerPixel = pixelsize;
  ibm_lBytesPerRow   = rect.WidthOf() * pixelsize;
  ibm_pData          = ((UBYTE *)buffer) - rect.ra_MinX * pixelsize - rect.ra_MinY * ibm_lBytesPerRow;
}
///

/// ImageBitMap::DefineBitMap
// Define a bitmap of the given dimension with the given modulo value.
void ImageBitMap::DefineBitMap(APTR buffer,UBYTE type,const RectAngle<LONG> &rect,ULONG samplesperrow)
{ 
  UBYTE pixelsize    = type & CTYP_SIZE_MASK;
  // Initialize the internal bitmap (that the traditional color transformer maps to) to
  // the internal buffer of the client data. Note that libjpeg always
  // anchores the user bitmaps at zero,zero, so shift it.
  ibm_ulWidth        = rect.ra_MaxX + 1; // note that rect is always inclusive
  ibm_ulHeight       = rect.ra_MaxY + 1;
  // Fixpoint types.
  ibm_ucPixelType    = type;
  ibm_cBytesPerPixel = pixelsize;
  ibm_lBytesPerRow   = samplesperrow * pixelsize;
  ibm_pData          = ((UBYTE *)buffer) - rect.ra_MinX * pixelsize - rect.ra_MinY * ibm_lBytesPerRow;
}
///

