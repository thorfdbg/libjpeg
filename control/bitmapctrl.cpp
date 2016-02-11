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
**
** Basic control helper for requesting and releasing bitmap data.
**
** $Id: bitmapctrl.cpp,v 1.17 2015/03/12 15:58:31 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "control/bitmapctrl.hpp"
#include "interface/bitmaphook.hpp"
#include "codestream/rectanglerequest.hpp"
#include "std/string.hpp"
///

/// BitmapCtrl::BitmapCtrl
BitmapCtrl::BitmapCtrl(class Frame *frame)
  : BufferCtrl(frame->EnvironOf()), m_pFrame(frame), 
    m_ppBitmap(NULL), m_ppLDRBitmap(NULL), m_ppCTemp(NULL), m_pColorBuffer(NULL)
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

  if (m_ppCTemp == NULL)
    m_ppCTemp     = (LONG **)m_pEnviron->AllocMem(m_ucCount * sizeof(LONG *));

  if (m_pColorBuffer == NULL)
    m_pColorBuffer = (LONG *)m_pEnviron->AllocMem(m_ucCount * 64 * sizeof(LONG));

  if (m_ppBitmap == NULL) {
    m_ppBitmap      = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap *) * m_ucCount);
    memset(m_ppBitmap,0,sizeof(struct ImageBitMap *) * m_ucCount);
    
    for(UBYTE i = 0;i < m_ucCount;i++) {
      m_ppBitmap[i] = new(m_pEnviron) struct ImageBitMap();
      m_ppCTemp[i]  = m_pColorBuffer + i * 64;
    }
  }
}
///

/// BitmapCtrl::~BitmapCtrl
BitmapCtrl::~BitmapCtrl(void)
{
  UBYTE i;

  if (m_ppCTemp)
    m_pEnviron->FreeMem(m_ppCTemp,m_ucCount * sizeof(LONG *));
  
  if (m_pColorBuffer)
    m_pEnviron->FreeMem(m_pColorBuffer,m_ucCount * 64 * sizeof(LONG));
  
  if (m_ppBitmap) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppBitmap[i];
    }
    m_pEnviron->FreeMem(m_ppBitmap,sizeof(struct ImageBitMap *) * m_ucCount);
  }

  if (m_ppLDRBitmap) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppLDRBitmap[i];
    }
    m_pEnviron->FreeMem(m_ppLDRBitmap,sizeof(struct ImageBitMap *) * m_ucCount);
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
void BitmapCtrl::RequestUserData(class BitMapHook *bmh,const RectAngle<LONG> &r,UBYTE comp,bool alpha)
{
  assert(comp < m_ucCount && bmh);

  if (alpha) {
    bmh->RequestClientAlpha(r,m_ppBitmap[comp],m_pFrame->ComponentOf(comp));
  } else {
    bmh->RequestClientData(r,m_ppBitmap[comp],m_pFrame->ComponentOf(comp));
  }

  if (m_ucPixelType == 0) { // Not yet defined
    m_ucPixelType = m_ppBitmap[comp]->ibm_ucPixelType;
  } else if (m_ppBitmap[comp]->ibm_ucPixelType) {
    if (m_ucPixelType != m_ppBitmap[comp]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"BitmapCtrl::RequestUserData","pixel types must be consistent accross components");
    }
  }

  //
  // Now check whether the user supplies a dedicated LDR part.
  if (!alpha && bmh->providesLDRImage()) {
    // Need to build the LDR image layout?
    if (m_ppLDRBitmap == NULL) { 
      m_ppLDRBitmap    = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap *) * m_ucCount);
      memset(m_ppLDRBitmap,0,sizeof(struct ImageBitMap *) * m_ucCount);
    
      for(UBYTE i = 0;i < m_ucCount;i++) {
        m_ppLDRBitmap[i] = new(m_pEnviron) struct ImageBitMap();
      }
    }
    bmh->RequestLDRData(r,m_ppLDRBitmap[comp],m_pFrame->ComponentOf(comp));
  }
}
///

/// BitmapCtrl::ReleaseUserData
// Release the user data again through the bitmap hook.
void BitmapCtrl::ReleaseUserData(class BitMapHook *bmh,const RectAngle<LONG> &r,UBYTE comp,bool alpha)
{
  assert(comp < 4 && bmh);
  
  // If we have LDR bitmaps, release this one first as it was requested last.
  if (m_ppLDRBitmap && !alpha) {
    bmh->ReleaseLDRData(r,m_ppLDRBitmap[comp],m_pFrame->ComponentOf(comp));
  }

  //
  // Now for the HDR part, or the only part.
  if (alpha) {
    bmh->ReleaseClientAlpha(r,m_ppBitmap[comp],m_pFrame->ComponentOf(comp));
  } else {
    bmh->ReleaseClientData(r,m_ppBitmap[comp],m_pFrame->ComponentOf(comp));
  }

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

/// BitmapCtrl::ExtractLDRBitmap
// Extract a region from the LDR data.
void BitmapCtrl::ExtractLDRBitmap(struct ImageBitMap *ibm,const RectAngle<LONG> &rect,UBYTE i)
{
  assert(i < m_ucCount);
  assert(m_ppLDRBitmap);

  ibm->ExtractBitMap(m_ppLDRBitmap[i],rect);
}
///

 

/// BitmapCtrl::ReleaseUserDataFromEncoding 
// Release user data after encoding.
void BitmapCtrl::ReleaseUserDataFromEncoding(class BitMapHook *bmh,const RectAngle<LONG> &region,bool alpha)
{
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    ReleaseUserData(bmh,region,i,alpha);
  }
}
///

/// BitmapCtrl::ReleaseUserDataFromDecoding
// Release user data after decoding.
void BitmapCtrl::ReleaseUserDataFromDecoding(class BitMapHook *bmh,const struct RectangleRequest *rr,bool alpha)
{
  int i;
  
  for(i = rr->rr_usFirstComponent;i <=rr->rr_usLastComponent;i++) {
    ReleaseUserData(bmh,rr->rr_Request,i,alpha);
  }
}
///

/// BitmapCtrl::CropDecodingRegion
// First step of a region decoder: Find the region that can be provided in the next step.
void BitmapCtrl::CropDecodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *)
{
  ClipToImage(region);
}
///


