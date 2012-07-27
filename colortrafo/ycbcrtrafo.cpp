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
** This file provides the transformation from RGB to YCbCr
**
** $Id: ycbcrtrafo.cpp,v 1.12 2012-07-14 12:07:35 thor Exp $
**
*/

/// Includes
#include "colortrafo/ycbcrtrafo.hpp"
#include "tools/traits.hpp"
#include "std/string.hpp"
///

/// Defines
// Number of fractional bits allocated
#define FIX_BITS 13
// Convert floating point to a fixpoint representation
#define TO_FIX(x) LONG((x) * (1L << FIX_BITS) + 0.5)
// Conversion from fixpoint to integer
#define FIX_TO_INT(x) (((x) + ((1L << FIX_BITS) >> 1)) >> FIX_BITS)
// Conversion from fixpoint to color preshifted bits
#define FIX_TO_COLOR(x) (((x) + ((1L << (FIX_BITS - COLOR_BITS)) >> 1)) >> (FIX_BITS - COLOR_BITS))
// Conversion from fixpoint plus color preshifted bits to INT
#define FIX_COLOR_TO_INT(x) (((x) + ((1L << (FIX_BITS + COLOR_BITS)) >> 1)) >> (FIX_BITS + COLOR_BITS))
///

/// YCbCrTrafo::YCbCrTrafo
template<typename external,int count>
YCbCrTrafo<external,count>::YCbCrTrafo(class Environ *env)
  : ColorTrafo(env)
{
}
///

/// YCbCrTrafo::~YCbCrTrafo
template<typename external,int count>
YCbCrTrafo<external,count>::~YCbCrTrafo(void)
{
}
///

/// YCbCrTrafo::RGB2YCbCr
// Transform a block from RGB to YCbCr. Input are the three image bitmaps
// already clipped to the rectangle to transform, the coordinate rectangle to use
// and the level shift.
template<typename external,int count>
void YCbCrTrafo<external,count>::RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
					   LONG dcshift,LONG max)
{
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;

  // Convert to fixpoint.
  dcshift <<= FIX_BITS;
  max     <<= COLOR_BITS;
  max      += (1L << COLOR_BITS) - 1;

  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    switch(count) {
    case 4:
      memset(m_lA ,0,sizeof(m_lA));
    case 3:
      memset(m_lY ,0,sizeof(m_lY));
      memset(m_lCb,0,sizeof(m_lCb));
      memset(m_lCr,0,sizeof(m_lCr));
    }
  }

  for(x = 1;x < count;x++) {
    if (source[0]->ibm_ucPixelType != source[x]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"YCbCrTrafo::RGB2YCbCr",
		"pixel types of all three components in a RGB to YCbCr conversion must be identical");
    }
  }

  {
    const external *rptr,*gptr,*bptr,*aptr;
    switch(count) {
    case 4:
      aptr = (const external *)(source[3]->ibm_pData);
    case 3:
      rptr = (const external *)(source[0]->ibm_pData);
      gptr = (const external *)(source[1]->ibm_pData);
      bptr = (const external *)(source[2]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst,*adst;
      const external *r,*g,*b,*a;
      switch(count) {
      case 4:
	adst    = m_lA  + xmin + (y << 3);
	a       = aptr;
      case 3:
	ydst    = m_lY  + xmin + (y << 3);
	cbdst   = m_lCb + xmin + (y << 3);
	crdst   = m_lCr + xmin + (y << 3);
	r       = rptr;
	g       = gptr;
	b       = bptr;
      }
      for(x = xmin;x <= xmax;x++) { 
	switch(count) {
	case 4:
	  *adst = m_pusEncodingLUT[3][*a];
	  assert(*adst <= max);
	  adst++;
	  a  = (const external *)((const UBYTE *)(a) + source[3]->ibm_cBytesPerPixel);
	case 3:
	  external rv = m_pusEncodingLUT[0][*r];
	  external gv = m_pusEncodingLUT[1][*g];
	  external bv = m_pusEncodingLUT[2][*b];
	  *ydst  = FIX_TO_COLOR(rv *  TO_FIX(0.29900)      + gv *  TO_FIX(0.58700)      + bv * TO_FIX(0.11400));
	  *cbdst = FIX_TO_COLOR(rv * -TO_FIX(0.1687358916) + gv * -TO_FIX(0.3312641084) + bv * TO_FIX(0.5) + dcshift);
	  *crdst = FIX_TO_COLOR(rv *  TO_FIX(0.5)          + gv * -TO_FIX(0.4186875892) + bv *-TO_FIX(0.08131241085) + dcshift);
	  assert(*ydst <= max);
	  assert(*cbdst <= max);
	  assert(*crdst <= max);
	  ydst++,cbdst++,crdst++;
	  r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
	  g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
	  b  = (const external *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
	}
      }
      switch(count) {
      case 4:
	aptr  = (const external *)((const UBYTE *)(aptr) + source[3]->ibm_lBytesPerRow);
      case 3:
	rptr  = (const external *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
	gptr  = (const external *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
	bptr  = (const external *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// YCbCrTrafo::YCbCr2RGB
// Inverse transform a block from YCbCr to RGB, incuding a clipping operation and a dc level
// shift.
template<typename external,int count>
void YCbCrTrafo<external,count>::YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
					   LONG dcshift,LONG max)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  
  dcshift <<= COLOR_BITS; // scale correctly.
  
  if (max > TypeTrait<external>::Max) {
    JPG_THROW(OVERFLOW_PARAMETER,"YCbCrTrafo::YCbCr2RGB",
	      "RGB maximum intensity for pixel type does not fit into the type");
  }
  
  for(x = 0;x < count;x++) {
    if (dest[0]->ibm_ucPixelType != dest[x]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"YCbCrTrafo::YCbCr2RGB",
		"pixel types of all three components in a YCbCr to RGB conversion must be identical");
    }
  }

  {
    external *rptr,*gptr,*bptr,*aptr;
    switch(count) {
    case 4:
      aptr = (external *)(dest[3]->ibm_pData);
    case 3:
      rptr = (external *)(dest[0]->ibm_pData);
      gptr = (external *)(dest[1]->ibm_pData);
      bptr = (external *)(dest[2]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ysrc,*cbsrc,*crsrc,*asrc;
      external *r,*g,*b,*a;
      switch(count) {
      case 4:
	asrc    = m_lA  + xmin + (y << 3);
	a       = aptr;
      case 3:
	ysrc    = m_lY  + xmin + (y << 3);	
	cbsrc   = m_lCb + xmin + (y << 3);
	crsrc   = m_lCr + xmin + (y << 3);
	r       = rptr;
	g       = gptr;
	b       = bptr;
      }
      for(x = xmin;x <= xmax;x++) {
	LONG cr,cb,rv,gv,bv,av;
	switch(count) {
	case 4:
	  av = *asrc;
	  if (av < 0) av = 0;
	  if (av > max) av = max;
	  *a = m_pusDecodingLUT[3][av];
	  asrc++;
	  a  = (external *)((UBYTE *)(a) + dest[3]->ibm_cBytesPerPixel);
	case 3:
	  cr = *crsrc - dcshift;
	  cb = *cbsrc - dcshift;
	  rv = FIX_COLOR_TO_INT((*ysrc << FIX_BITS) + cr *  TO_FIX(1.40200));
	  gv = FIX_COLOR_TO_INT((*ysrc << FIX_BITS) + cr * -TO_FIX(0.7141362859) + cb * -TO_FIX(0.3441362861));
	  bv = FIX_COLOR_TO_INT((*ysrc << FIX_BITS) + cb *  TO_FIX(1.772));
	  if (rv < 0) rv = 0;
	  if (gv < 0) gv = 0;
	  if (bv < 0) bv = 0;
	  if (rv > max) rv = max;
	  if (gv > max) gv = max;
	  if (bv > max) bv = max;
	  *r = m_pusDecodingLUT[0][rv];
	  *g = m_pusDecodingLUT[1][gv];
	  *b = m_pusDecodingLUT[2][bv];
	  ysrc++,cbsrc++,crsrc++;
	  r  = (external *)((UBYTE *)(r) + dest[0]->ibm_cBytesPerPixel);
	  g  = (external *)((UBYTE *)(g) + dest[1]->ibm_cBytesPerPixel);
	  b  = (external *)((UBYTE *)(b) + dest[2]->ibm_cBytesPerPixel);
	}
      }
      switch(count) {
      case 4:
	aptr  = (external *)((UBYTE *)(aptr) + dest[3]->ibm_lBytesPerRow);
      case 3:
	rptr  = (external *)((UBYTE *)(rptr) + dest[0]->ibm_lBytesPerRow);
	gptr  = (external *)((UBYTE *)(gptr) + dest[1]->ibm_lBytesPerRow);
	bptr  = (external *)((UBYTE *)(bptr) + dest[2]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// Explicit instanciations
template class YCbCrTrafo<UBYTE,3>;
template class YCbCrTrafo<UWORD,3>;
template class YCbCrTrafo<UBYTE,4>;
template class YCbCrTrafo<UWORD,4>;
///

