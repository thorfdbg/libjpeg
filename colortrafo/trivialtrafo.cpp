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
** This file provides the trival transformation from RGB to RGB
**
** $Id: trivialtrafo.cpp,v 1.7 2012-07-18 19:35:45 thor Exp $
**
*/

/// Includes
#include "colortrafo/trivialtrafo.hpp"
#include "tools/traits.hpp"
#include "std/string.hpp"
///

/// TrivialTrafo::TrivialTrafo
template<typename external,int count>
TrivialTrafo<external,count>::TrivialTrafo(class Environ *env)
  : ColorTrafo(env)
{
}
///

/// TrivialTrafo::~TrivialTrafo
template<typename external,int count>
TrivialTrafo<external,count>::~TrivialTrafo(void)
{
}
///

/// TrivialTrafo::RGB2YCbCr
// Transform a block from RGB to YCbCr. Input are the three image bitmaps
// already clipped to the rectangle to transform, the coordinate rectangle to use
// and the level shift.
template<typename external,int count>
void TrivialTrafo<external,count>::RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
					     LONG,LONG max)
{
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;

  NOREF(max);

  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    switch(count) {
    case 4:
      memset(m_lA,0,sizeof(m_lA));
    case 3:
      memset(m_lCr,0,sizeof(m_lCr));
    case 2:
      memset(m_lCb,0,sizeof(m_lCb));
    case 1:
      memset(m_lY ,0,sizeof(m_lY));
    }
  }
  
  for(x = 1; x < count;x++) {
    if (source[0]->ibm_ucPixelType != source[x]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"TrivialTrafo::RGB2YCbCr",
		"pixel types of all three components in a RGB to RGB conversion must be identical");
    }
  }

  {
    const external *rptr,*gptr,*bptr,*aptr;
    switch(count) {
    case 4:
      aptr = (const external *)(source[3]->ibm_pData);
    case 3:
      bptr = (const external *)(source[2]->ibm_pData);
    case 2:
      gptr = (const external *)(source[1]->ibm_pData);
    case 1:
      rptr = (const external *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst,*adst;
      const external *r,*g,*b,*a;
      switch(count) {
      case 4:
	adst    = m_lA  + xmin + (y << 3);
	a       = aptr;
      case 3:
	crdst   = m_lCr + xmin + (y << 3);
	b       = bptr;
      case 2:
	cbdst   = m_lCb + xmin + (y << 3);
	g       = gptr;
      case 1:
	ydst    = m_lY  + xmin + (y << 3);
	r       = rptr;
      }
      for(x = xmin;x <= xmax;x++) { 
	switch(count) {
	case 4:
	  *adst = m_pusEncodingLUT[3][*a];
	  assert(*adst <= max);
	  adst++;
	  a  = (const external *)((const UBYTE *)(a) + source[3]->ibm_cBytesPerPixel);
	case 3:
	  *crdst = m_pusEncodingLUT[2][*b];
	  assert(*crdst <= max);
	  crdst++;
	  b  = (const external *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
	case 2:
	  *cbdst = m_pusEncodingLUT[1][*g];
	  assert(*cbdst <= max);
	  cbdst++;
	  g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
	case 1:
	  *ydst  = m_pusEncodingLUT[0][*r];
	  assert(*ydst <= max);
	  ydst++;
	  r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
	}
      }
      switch(count) {
      case 4:
	aptr  = (const external *)((const UBYTE *)(aptr) + source[3]->ibm_lBytesPerRow);
      case 3:
	bptr  = (const external *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
      case 2:
	gptr  = (const external *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
      case 1:
	rptr  = (const external *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// TrivialTrafo::YCbCr2RGB
// Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
// shift.
template<typename external,int count>
void TrivialTrafo<external,count>::YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
					     LONG,LONG max)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  
  if (max > TypeTrait<external>::Max) {
    JPG_THROW(OVERFLOW_PARAMETER,"TrivialTrafo::YCbCr2RGB",
	      "RGB maximum intensity for pixel type does not fit into the type");
  }
  
  for(x = 1;x < count;x++) {
    if (dest[0]->ibm_ucPixelType != dest[x]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"TrivialTrafo::YCbCr2RGB",
		"pixel types of all three components in a RGB to RGB conversion must be identical");
    }
  }

  {
    external *rptr,*gptr,*bptr,*aptr;
    switch(count) {
    case 4:
      aptr = (external *)(dest[3]->ibm_pData);
    case 3:
      bptr = (external *)(dest[2]->ibm_pData);
    case 2:
      gptr = (external *)(dest[1]->ibm_pData);
    case 1:
      rptr = (external *)(dest[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ysrc,*cbsrc,*crsrc,*asrc;
      external *r,*g,*b,*a;
      switch(count) {
      case 4:
	asrc   = m_lA  + xmin + (y << 3);
	a      = aptr;
      case 3:
	crsrc  = m_lCr + xmin + (y << 3);
	b      = bptr;
      case 2:
	cbsrc  = m_lCb + xmin + (y << 3);
	g      = gptr;
      case 1:
	ysrc   = m_lY  + xmin + (y << 3);
	r      = rptr;
      }
      for(x = xmin;x <= xmax;x++) {
	LONG rv,gv,bv,av;
	switch(count) {
	case 4:
	  av = *asrc++;
	  if (av < 0)   av = 0;
	  if (av > max) av = max;
	  *a = m_pusDecodingLUT[3][av];
	  a  = (external *)((UBYTE *)(a) + dest[3]->ibm_cBytesPerPixel);
	case 3:
	  bv = *crsrc++;
	  if (bv < 0)   bv = 0;
	  if (bv > max) bv = max;
	  *b = m_pusDecodingLUT[2][bv];
	  b  = (external *)((UBYTE *)(b) + dest[2]->ibm_cBytesPerPixel);
	case 2:
	  gv = *cbsrc++;
	  if (gv < 0)   gv = 0;
	  if (gv > max) gv = max;
	  *g = m_pusDecodingLUT[1][gv];
	  g  = (external *)((UBYTE *)(g) + dest[1]->ibm_cBytesPerPixel);
	case 1:
	  rv = *ysrc++;
	  if (rv < 0)   rv = 0;
	  if (rv > max) rv = max;
	  *r = m_pusDecodingLUT[0][rv];
	  r  = (external *)((UBYTE *)(r) + dest[0]->ibm_cBytesPerPixel);
	}
      }
      switch(count) {
      case 4:
	aptr  = (external *)((UBYTE *)(aptr) + dest[3]->ibm_lBytesPerRow);
      case 3:
	bptr  = (external *)((UBYTE *)(bptr) + dest[2]->ibm_lBytesPerRow);
      case 2:
	gptr  = (external *)((UBYTE *)(gptr) + dest[1]->ibm_lBytesPerRow);
      case 1:
	rptr  = (external *)((UBYTE *)(rptr) + dest[0]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// Explicit instanciations
template class TrivialTrafo<UBYTE,1>;
template class TrivialTrafo<UWORD,1>;
template class TrivialTrafo<UBYTE,2>;
template class TrivialTrafo<UWORD,2>;
template class TrivialTrafo<UBYTE,3>;
template class TrivialTrafo<UWORD,3>;
template class TrivialTrafo<UBYTE,4>;
template class TrivialTrafo<UWORD,4>;
///

