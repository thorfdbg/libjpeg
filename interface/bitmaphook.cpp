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
 * Member functions of the bitmap hook class
 * 
 * $Id: bitmaphook.cpp,v 1.2 2012-06-02 10:27:14 thor Exp $
 *
 */

/// Includes
#include "interface/types.hpp"
#include "interface/bitmaphook.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "marker/component.hpp"
#include "tools/environment.hpp"
#include "std/assert.hpp"
///

/// BitMapHook::BitMapHook
BitMapHook::BitMapHook(const struct JPG_TagItem *tags)
  : m_pHook(NULL)
{
  assert(tags);
  Init(tags);
}
///

/// BitMapHook::ParseTags
// Parse off the user parameters to fill in the default image bitmap layout.
void BitMapHook::ParseTags(const struct JPG_TagItem *tag)
{  
  assert(tag);
  //
  // Override the default image layout
  while (tag) {
    switch (tag->ti_Tag) {
    case JPGTAG_BIO_MEMORY:
      m_DefaultImageLayout.ibm_pData           = tag->ti_Data.ti_pPtr;
      break;
    case JPGTAG_BIO_WIDTH:
      m_DefaultImageLayout.ibm_ulWidth         = tag->ti_Data.ti_lData;
      break;
    case JPGTAG_BIO_HEIGHT:
      m_DefaultImageLayout.ibm_ulHeight        = tag->ti_Data.ti_lData;
      break;
    case JPGTAG_BIO_BYTESPERROW:
      m_DefaultImageLayout.ibm_lBytesPerRow    = tag->ti_Data.ti_lData;
      break;
    case JPGTAG_BIO_BYTESPERPIXEL:
      m_DefaultImageLayout.ibm_cBytesPerPixel  = (UBYTE) tag->ti_Data.ti_lData;
      break;
    case JPGTAG_BIO_PIXELTYPE:
      m_DefaultImageLayout.ibm_ucPixelType     = (UBYTE) tag->ti_Data.ti_lData;
      break;
    case JPGTAG_BIO_USERDATA:
      m_DefaultImageLayout.ibm_pUserData       = tag->ti_Data.ti_pPtr;
      break;
    case JPGTAG_BIH_HOOK:
      m_pHook                                  = (struct JPG_Hook *)tag->ti_Data.ti_pPtr;
    }
    tag = tag->NextTagItem();
  }
}
///

/// BitMapHook::Init
// Init the tags for the given canvas/slice coordinates.
void BitMapHook::Init(const struct JPG_TagItem *tag)
{
  //
  // Fill in use useful defaults. This really depends on the user
  // As the following tags are all optional.
  //
  m_DefaultImageLayout.ibm_pData           = NULL;
  m_DefaultImageLayout.ibm_ulWidth         = 0;
  m_DefaultImageLayout.ibm_ulHeight        = 0;
  m_DefaultImageLayout.ibm_lBytesPerRow    = 0;
  m_DefaultImageLayout.ibm_cBytesPerPixel  = 0;
  m_DefaultImageLayout.ibm_ucPixelType     = 0;
  m_DefaultImageLayout.ibm_pUserData       = NULL;
  //
  //
  if (tag)
    ParseTags(tag); // Parse off the user tags.
  //
  // Setup the tags for the bitmap/image data
  m_BitmapTags[0].ti_Tag              = JPGTAG_BIO_ACTION;
  m_BitmapTags[1].ti_Tag              = JPGTAG_BIO_MEMORY;
  m_BitmapTags[2].ti_Tag              = JPGTAG_BIO_WIDTH;
  m_BitmapTags[3].ti_Tag              = JPGTAG_BIO_HEIGHT;
  m_BitmapTags[4].ti_Tag              = JPGTAG_BIO_BYTESPERROW;
  m_BitmapTags[5].ti_Tag              = JPGTAG_BIO_BYTESPERPIXEL;
  m_BitmapTags[6].ti_Tag              = JPGTAG_BIO_PIXELTYPE;
  m_BitmapTags[7].ti_Tag              = JPGTAG_BIO_ROI;
  m_BitmapTags[7].ti_Data.ti_lData    = false;
  m_BitmapTags[8].ti_Tag              = JPGTAG_BIO_COMPONENT;
  m_BitmapTags[9].ti_Tag              = JPGTAG_BIO_USERDATA;
  m_BitmapTags[9].ti_Data.ti_pPtr     = m_DefaultImageLayout.ibm_pUserData;
  m_BitmapTags[10].ti_Tag             = JPGTAG_BIO_MINX;
  m_BitmapTags[11].ti_Tag             = JPGTAG_BIO_MINY;
  m_BitmapTags[12].ti_Tag             = JPGTAG_BIO_MAXX;
  m_BitmapTags[13].ti_Tag             = JPGTAG_BIO_MAXY;
  m_BitmapTags[14].ti_Tag             = JPGTAG_TAG_IGNORE;
  m_BitmapTags[14].ti_Data.ti_lData   = 0;
  m_BitmapTags[15].ti_Tag             = JPGTAG_TAG_IGNORE; // was: BIO_SLICE;
  m_BitmapTags[15].ti_Data.ti_lData   = 0;
  m_BitmapTags[16].ti_Tag             = JPGTAG_TAG_IGNORE; // was: JPGTAG_BIO_COLOR;
  m_BitmapTags[17].ti_Tag             = JPGTAG_BIO_PIXEL_MINX;
  m_BitmapTags[18].ti_Tag             = JPGTAG_BIO_PIXEL_MINY;
  m_BitmapTags[19].ti_Tag             = JPGTAG_BIO_PIXEL_MAXX;
  m_BitmapTags[20].ti_Tag             = JPGTAG_BIO_PIXEL_MAXY;
  m_BitmapTags[21].ti_Tag             = JPGTAG_BIO_PIXEL_XORG;
  m_BitmapTags[22].ti_Tag             = JPGTAG_BIO_PIXEL_YORG;
  m_BitmapTags[23].ti_Tag             = JPGTAG_TAG_DONE;
}
///

/// BitMapHook::RequestClientData
// Pass an empty tag list over to the user,
// let the user fill out this tag list and
// fill out the image bitmap from this stuff.
void BitMapHook::RequestClientData(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
				   const class Component *comp)
{
  //
  // Fill in the encoding tags.
  m_BitmapTags[0].ti_Data.ti_lData  = JPGFLAG_BIO_REQUEST;
  m_BitmapTags[1].ti_Data.ti_pPtr   = m_DefaultImageLayout.ibm_pData;
  m_BitmapTags[2].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_ulWidth;
  m_BitmapTags[3].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_ulHeight;
  m_BitmapTags[4].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_lBytesPerRow;
  m_BitmapTags[5].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_cBytesPerPixel;
  m_BitmapTags[6].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_ucPixelType;
  m_BitmapTags[8].ti_Data.ti_lData  = comp->IndexOf();
  m_BitmapTags[9].ti_Data.ti_pPtr   = m_DefaultImageLayout.ibm_pUserData;
  m_BitmapTags[10].ti_Data.ti_lData = rect.ra_MinX;
  m_BitmapTags[11].ti_Data.ti_lData = rect.ra_MinY;
  m_BitmapTags[12].ti_Data.ti_lData = rect.ra_MaxX;
  m_BitmapTags[13].ti_Data.ti_lData = rect.ra_MaxY;
  m_BitmapTags[15].ti_Data.ti_lData = 0;
  m_BitmapTags[16].ti_Data.ti_lData = comp->IndexOf();
  m_BitmapTags[17].ti_Data.ti_lData = (rect.ra_MinX + comp->SubXOf() - 1) / comp->SubXOf();
  m_BitmapTags[18].ti_Data.ti_lData = (rect.ra_MinY + comp->SubYOf() - 1) / comp->SubYOf();
  m_BitmapTags[19].ti_Data.ti_lData = (rect.ra_MaxX + comp->SubXOf() + 1 - 1) / comp->SubXOf() - 1;
  m_BitmapTags[20].ti_Data.ti_lData = (rect.ra_MaxY + comp->SubYOf() + 1 - 1) / comp->SubYOf() - 1;
  m_BitmapTags[21].ti_Data.ti_lData = 0;
  m_BitmapTags[22].ti_Data.ti_lData = 0;
  //
  // Now call the hook if it exists
  if (m_pHook)
    m_pHook->CallLong(m_BitmapTags);

  // and now, finally, scan what we got back
  ibm->ibm_pData           = m_BitmapTags[1].ti_Data.ti_pPtr;
  ibm->ibm_ulWidth         = m_BitmapTags[2].ti_Data.ti_lData;
  ibm->ibm_ulHeight        = m_BitmapTags[3].ti_Data.ti_lData;
  ibm->ibm_lBytesPerRow    = m_BitmapTags[4].ti_Data.ti_lData;
  ibm->ibm_cBytesPerPixel  = m_BitmapTags[5].ti_Data.ti_lData;
  ibm->ibm_ucPixelType     = m_BitmapTags[6].ti_Data.ti_lData;
  ibm->ibm_pUserData       = m_BitmapTags[9].ti_Data.ti_pPtr;
}
///

/// BitMapHook::ReleaseClientData
// Tell the client that we are done with the data
// and release it. The user may use this
// call here to release temporary memory, etc, etc.
void BitMapHook::ReleaseClientData(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
				   const class Component *comp)
{
  if (m_pHook) {
    m_BitmapTags[0].ti_Data.ti_lData  = JPGFLAG_BIO_RELEASE;
    m_BitmapTags[1].ti_Data.ti_pPtr   = ibm->ibm_pData;
    m_BitmapTags[2].ti_Data.ti_lData  = ibm->ibm_ulWidth;
    m_BitmapTags[3].ti_Data.ti_lData  = ibm->ibm_ulHeight;
    m_BitmapTags[4].ti_Data.ti_lData  = ibm->ibm_lBytesPerRow;
    m_BitmapTags[5].ti_Data.ti_lData  = ibm->ibm_cBytesPerPixel;
    m_BitmapTags[6].ti_Data.ti_lData  = ibm->ibm_ucPixelType;
    m_BitmapTags[8].ti_Data.ti_lData  = comp->IndexOf();
    m_BitmapTags[9].ti_Data.ti_pPtr   = ibm->ibm_pUserData;
    m_BitmapTags[10].ti_Data.ti_lData = rect.ra_MinX;
    m_BitmapTags[11].ti_Data.ti_lData = rect.ra_MinY;
    m_BitmapTags[12].ti_Data.ti_lData = rect.ra_MaxX;
    m_BitmapTags[13].ti_Data.ti_lData = rect.ra_MaxY;
    m_BitmapTags[15].ti_Data.ti_lData = 0;
    m_BitmapTags[16].ti_Data.ti_lData = comp->IndexOf();
    m_BitmapTags[17].ti_Data.ti_lData = (rect.ra_MinX + comp->SubXOf() - 1) / comp->SubXOf();
    m_BitmapTags[18].ti_Data.ti_lData = (rect.ra_MinY + comp->SubYOf() - 1) / comp->SubYOf();
    m_BitmapTags[19].ti_Data.ti_lData = (rect.ra_MaxX + comp->SubXOf() + 1 - 1) / comp->SubXOf() - 1;
    m_BitmapTags[20].ti_Data.ti_lData = (rect.ra_MaxY + comp->SubYOf() + 1 - 1) / comp->SubYOf() - 1;
    m_BitmapTags[21].ti_Data.ti_lData = 0;
    m_BitmapTags[22].ti_Data.ti_lData = 0;
    
    m_pHook->CallLong(m_BitmapTags);
  }
}
///
