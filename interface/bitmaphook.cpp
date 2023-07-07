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
 * Member functions of the bitmap hook class
 * 
 * $Id: bitmaphook.cpp,v 1.11 2023/07/07 12:19:54 thor Exp $
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
#include "tools/traits.hpp"
#include "std/assert.hpp"
///

/// BitMapHook::BitMapHook
BitMapHook::BitMapHook(const struct JPG_TagItem *tags)
  : m_pHook(NULL), m_pLDRHook(NULL), m_pAlphaHook(NULL)
{
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
  InitHookTags(m_BitmapTags);
  InitHookTags(m_LDRTags);

  if (tags)
    ParseTags(tags); // Parse off the user tags.
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
      break;
    case JPGTAG_BIH_LDRHOOK:
      m_pLDRHook                               = (struct JPG_Hook *)tag->ti_Data.ti_pPtr;
      break;
    case JPGTAG_BIH_ALPHAHOOK:
      m_pAlphaHook                             = (struct JPG_Hook *)tag->ti_Data.ti_pPtr;
      break;
    }
    tag = tag->NextTagItem();
  }
}
///

/// BitMapHook::InitHookTags
// Setup the input parameter tags for the user hook.
void BitMapHook::InitHookTags(struct JPG_TagItem *tags)
{
  // Setup the tags for the bitmap/image data
  tags[0].ti_Tag              = JPGTAG_BIO_ACTION;
  tags[1].ti_Tag              = JPGTAG_BIO_MEMORY;
  tags[2].ti_Tag              = JPGTAG_BIO_WIDTH;
  tags[3].ti_Tag              = JPGTAG_BIO_HEIGHT;
  tags[4].ti_Tag              = JPGTAG_BIO_BYTESPERROW;
  tags[5].ti_Tag              = JPGTAG_BIO_BYTESPERPIXEL;
  tags[6].ti_Tag              = JPGTAG_BIO_PIXELTYPE;
  tags[7].ti_Tag              = JPGTAG_BIO_ROI;
  tags[7].ti_Data.ti_lData    = false;
  tags[8].ti_Tag              = JPGTAG_BIO_COMPONENT;
  tags[9].ti_Tag              = JPGTAG_BIO_USERDATA;
  tags[9].ti_Data.ti_pPtr     = m_DefaultImageLayout.ibm_pUserData;
  tags[10].ti_Tag             = JPGTAG_BIO_MINX;
  tags[11].ti_Tag             = JPGTAG_BIO_MINY;
  tags[12].ti_Tag             = JPGTAG_BIO_MAXX;
  tags[13].ti_Tag             = JPGTAG_BIO_MAXY;
  tags[14].ti_Tag             = JPGTAG_BIO_ALPHA;
  tags[14].ti_Data.ti_lData   = false;
  tags[15].ti_Tag             = JPGTAG_TAG_IGNORE; // was: BIO_SLICE;
  tags[15].ti_Data.ti_lData   = 0;
  tags[16].ti_Tag             = JPGTAG_TAG_IGNORE; // was: JPGTAG_BIO_COLOR;
  tags[17].ti_Tag             = JPGTAG_BIO_PIXEL_MINX;
  tags[18].ti_Tag             = JPGTAG_BIO_PIXEL_MINY;
  tags[19].ti_Tag             = JPGTAG_BIO_PIXEL_MAXX;
  tags[20].ti_Tag             = JPGTAG_BIO_PIXEL_MAXY;
  tags[21].ti_Tag             = JPGTAG_BIO_PIXEL_XORG;
  tags[22].ti_Tag             = JPGTAG_BIO_PIXEL_YORG;
  tags[23].ti_Tag             = JPGTAG_TAG_DONE;
}
///

/// BitMapHook::Request
// Fill the tag items for a request call and make the call.
void BitMapHook::Request(struct JPG_Hook *hook,struct JPG_TagItem *tags,UBYTE pixeltype,
                         const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                         const class Component *comp,bool alpha)
{
  // Fill in the encoding tags.
  tags[0].ti_Data.ti_lData  = JPGFLAG_BIO_REQUEST;
  tags[1].ti_Data.ti_pPtr   = m_DefaultImageLayout.ibm_pData;
  tags[2].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_ulWidth;
  tags[3].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_ulHeight;
  tags[4].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_lBytesPerRow;
  tags[5].ti_Data.ti_lData  = m_DefaultImageLayout.ibm_cBytesPerPixel;
  tags[6].ti_Data.ti_lData  = pixeltype;
  tags[8].ti_Data.ti_lData  = comp->IndexOf();
  tags[9].ti_Data.ti_pPtr   = m_DefaultImageLayout.ibm_pUserData;
  tags[10].ti_Data.ti_lData = rect.ra_MinX;
  tags[11].ti_Data.ti_lData = rect.ra_MinY;
  tags[12].ti_Data.ti_lData = rect.ra_MaxX;
  tags[13].ti_Data.ti_lData = rect.ra_MaxY;
  tags[14].ti_Data.ti_lData = alpha;
  tags[15].ti_Data.ti_lData = 0;
  tags[16].ti_Data.ti_lData = comp->IndexOf();
  tags[17].ti_Data.ti_lData = (rect.ra_MinX + comp->SubXOf() - 1) / comp->SubXOf();
  tags[18].ti_Data.ti_lData = (rect.ra_MinY + comp->SubYOf() - 1) / comp->SubYOf();
  tags[19].ti_Data.ti_lData = (rect.ra_MaxX + comp->SubXOf() + 1 - 1) / comp->SubXOf() - 1;
  tags[20].ti_Data.ti_lData = (rect.ra_MaxY + comp->SubYOf() + 1 - 1) / comp->SubYOf() - 1;
  tags[21].ti_Data.ti_lData = 0;
  tags[22].ti_Data.ti_lData = 0;
  //
  // Now call the hook if it exists
  if (hook) {
    LONG result = hook->CallLong(tags);
    if (result < 0)
      comp->EnvironOf()->Throw(result,"BitmapHook::Request",__LINE__,__FILE__,"BitMapHook signalled an error");
  }

  // and now, finally, scan what we got back
  ibm->ibm_pData           = tags[1].ti_Data.ti_pPtr;
  ibm->ibm_ulWidth         = tags[2].ti_Data.ti_lData;
  ibm->ibm_ulHeight        = tags[3].ti_Data.ti_lData;
  ibm->ibm_lBytesPerRow    = tags[4].ti_Data.ti_lData;
  ibm->ibm_cBytesPerPixel  = tags[5].ti_Data.ti_lData;
  ibm->ibm_ucPixelType     = tags[6].ti_Data.ti_lData;
  ibm->ibm_pUserData       = tags[9].ti_Data.ti_pPtr;
}
///

/// BitMapHook::Release
// Release the tag items for a release call and make the call.
void BitMapHook::Release(struct JPG_Hook *hook,struct JPG_TagItem *tags,UBYTE pixeltype,
                         const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                         const class Component *comp,bool alpha)
{
  if (hook) {
    tags[0].ti_Data.ti_lData  = JPGFLAG_BIO_RELEASE;
    tags[1].ti_Data.ti_pPtr   = ibm->ibm_pData;
    tags[2].ti_Data.ti_lData  = ibm->ibm_ulWidth;
    tags[3].ti_Data.ti_lData  = ibm->ibm_ulHeight;
    tags[4].ti_Data.ti_lData  = ibm->ibm_lBytesPerRow;
    tags[5].ti_Data.ti_lData  = ibm->ibm_cBytesPerPixel;
    tags[6].ti_Data.ti_lData  = pixeltype;
    tags[8].ti_Data.ti_lData  = comp->IndexOf();
    tags[9].ti_Data.ti_pPtr   = ibm->ibm_pUserData;
    tags[10].ti_Data.ti_lData = rect.ra_MinX;
    tags[11].ti_Data.ti_lData = rect.ra_MinY;
    tags[12].ti_Data.ti_lData = rect.ra_MaxX;
    tags[13].ti_Data.ti_lData = rect.ra_MaxY;
    tags[14].ti_Data.ti_lData = alpha;
    tags[15].ti_Data.ti_lData = 0;
    tags[16].ti_Data.ti_lData = comp->IndexOf();
    tags[17].ti_Data.ti_lData = (rect.ra_MinX + comp->SubXOf() - 1) / comp->SubXOf();
    tags[18].ti_Data.ti_lData = (rect.ra_MinY + comp->SubYOf() - 1) / comp->SubYOf();
    tags[19].ti_Data.ti_lData = (rect.ra_MaxX + comp->SubXOf() + 1 - 1) / comp->SubXOf() - 1;
    tags[20].ti_Data.ti_lData = (rect.ra_MaxY + comp->SubYOf() + 1 - 1) / comp->SubYOf() - 1;
    tags[21].ti_Data.ti_lData = 0;
    tags[22].ti_Data.ti_lData = 0;

    if (hook) {
      LONG result = hook->CallLong(tags);
      if (result < 0)
        comp->EnvironOf()->Throw(result,"BitmapHook::Release",__LINE__,__FILE__,"BitMapHook signalled an error");
    }
  }
}
///

/// BitMapHook::RequestClientData
// Pass an empty tag list over to the user,
// let the user fill out this tag list and
// fill out the image bitmap from this stuff.
void BitMapHook::RequestClientData(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                                   const class Component *comp)
{
  Request(m_pHook,m_BitmapTags,m_DefaultImageLayout.ibm_ucPixelType,rect,ibm,comp,false);
}
///

/// BitMapHook::ReleaseClientData
// Tell the client that we are done with the data
// and release it. The user may use this
// call here to release temporary memory, etc, etc.
void BitMapHook::ReleaseClientData(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                                   const class Component *comp)
{
  Release(m_pHook,m_BitmapTags,m_DefaultImageLayout.ibm_ucPixelType,rect,ibm,comp,false);
}
///

/// BitMapHook::RequestClientAlpha
// Pass an empty tag list over to the user,
// let the user fill out this tag list and
// fill out the image bitmap from this stuff.
void BitMapHook::RequestClientAlpha(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                                   const class Component *comp)
{
  Request(m_pAlphaHook,m_BitmapTags,m_DefaultImageLayout.ibm_ucPixelType,rect,ibm,comp,true);
}
///

/// BitMapHook::ReleaseClientAlpha
// Tell the client that we are done with the data
// and release it. The user may use this
// call here to release temporary memory, etc, etc.
void BitMapHook::ReleaseClientAlpha(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                                    const class Component *comp)
{
  Release(m_pAlphaHook,m_BitmapTags,m_DefaultImageLayout.ibm_ucPixelType,rect,ibm,comp,true);
}
///



/// BitMapHook::RequestLDRData
// Retrieve the LDR tone mapped version of the user. This requires that an
// LDR hook function is available, i.e. should only be called if the 
// providesLDRImage() method above returns true.
void BitMapHook::RequestLDRData(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                                const class Component *comp)
{
  Request(m_pLDRHook,m_LDRTags,CTYP_UBYTE,rect,ibm,comp,false);
}
///

/// BitMapHook::ReleaseLDRData
// Release the requested LDR data. Requires that an LDR hook is available, i.e.
// providesLDRImage() must have been checked before and must have returned
// true for this to make sense.
void BitMapHook::ReleaseLDRData(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                                const class Component *comp)
{
  Release(m_pLDRHook,m_LDRTags,CTYP_UBYTE,rect,ibm,comp,false);
}
///
