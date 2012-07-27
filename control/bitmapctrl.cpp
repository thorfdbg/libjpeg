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
**
** Basic control helper for requesting and releasing bitmap data.
**
** $Id: bitmapctrl.cpp,v 1.8 2012-06-11 10:19:15 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "control/bitmapctrl.hpp"
#include "interface/bitmaphook.hpp"
#include "std/string.hpp"
///

/// BitmapCtrl::BitmapCtrl
BitmapCtrl::BitmapCtrl(class Frame *frame)
  : BufferCtrl(frame->EnvironOf()), m_pFrame(frame), 
    m_ppBitmap(NULL)
{
}
///

/// BitmapCtrl::BuildCommon
void BitmapCtrl::BuildCommon(void)
{
  m_ulPixelWidth  = m_pFrame->WidthOf();
  m_ulPixelHeight = m_pFrame->HeightOf();
  m_ucPixelType   = 0;
  m_ucCount       = m_pFrame->DepthOf();
  m_ucPointShift  = m_pFrame->PointPreShiftOf();

  if (m_ppBitmap == NULL) {
    m_ppBitmap      = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap *) * m_ucCount);
    memset(m_ppBitmap,0,sizeof(struct ImageBitMap *) * m_ucCount);
    
    for(UBYTE i = 0;i < m_ucCount;i++) {
      m_ppBitmap[i] = new(m_pEnviron) struct ImageBitMap();
    }
  }
}
///

/// BitmapCtrl::~BitmapCtrl
BitmapCtrl::~BitmapCtrl(void)
{
  UBYTE i;

  if (m_ppBitmap) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppBitmap[i];
    }
    m_pEnviron->FreeMem(m_ppBitmap,sizeof(struct ImageBitMap *) * m_ucCount);
  }
}
///

/// BitmapCtrl::ClipToImage
// Clip a rectangle to the image region
void BitmapCtrl::ClipToImage(RectAngle<LONG> &rect) const
{
  if (rect.ra_MinX < 0)
    rect.ra_MinX = 0;
  if (rect.ra_MaxX >= LONG(m_ulPixelWidth))
    rect.ra_MaxX = m_ulPixelWidth  - 1;
  if (rect.ra_MinY < 0)
    rect.ra_MinY = 0;
  if (m_ulPixelHeight && rect.ra_MaxY >= LONG(m_ulPixelHeight))
    rect.ra_MaxY = m_ulPixelHeight - 1;
}
///

/// BitmapCtrl::RequestUserData
// Request data from the user through the indicated bitmap hook
// for the given rectangle. The rectangle is first clipped to
// range (as appropriate, if the height is already known) and
// then the desired n'th component of the scan (not the component
// index) is requested.
void BitmapCtrl::RequestUserData(class BitMapHook *bmh,const RectAngle<LONG> &r,UBYTE comp)
{
  assert(comp < m_ucCount && bmh);

  bmh->RequestClientData(r,m_ppBitmap[comp],m_pFrame->ComponentOf(comp));

  if (m_ucPixelType == 0) { // Not yet defined
    m_ucPixelType = m_ppBitmap[comp]->ibm_ucPixelType;
  } else if (m_ppBitmap[comp]->ibm_ucPixelType) {
    if (m_ucPixelType != m_ppBitmap[comp]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"BitmapCtrl::RequestUserData","pixel types must be consistent accross components");
    }
  }
}
///

/// BitmapCtrl::ReleaseUserData
// Release the user data again through the bitmap hook.
void BitmapCtrl::ReleaseUserData(class BitMapHook *bmh,const RectAngle<LONG> &r,UBYTE comp)
{
  assert(comp < 4 && bmh);
  
  bmh->ReleaseClientData(r,m_ppBitmap[comp],m_pFrame->ComponentOf(comp));

  m_ucPixelType = 0;
}
///

/// BitmapCtrl::ExtractBitmap
// Extract the region of the bitmap covering the indicated rectangle
void BitmapCtrl::ExtractBitmap(struct ImageBitMap *ibm,const RectAngle<LONG> &rect,UBYTE i)
{
  assert(i < m_ucCount);
  
  ibm->ExtractBitMap(m_ppBitmap[i],rect);
}
///
