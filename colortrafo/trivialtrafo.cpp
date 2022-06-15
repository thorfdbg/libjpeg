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
** This file provides the trival transformation from RGB to RGB
**
** $Id: trivialtrafo.cpp,v 1.21 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "colortrafo/trivialtrafo.hpp"
#include "tools/traits.hpp"
#include "std/string.hpp"
///

/// TrivialTrafo::TrivialTrafo
template<typename internal,typename external,int count>
TrivialTrafo<internal,external,count>::TrivialTrafo(class Environ *env,LONG dcshift,LONG max)
  : ColorTrafo(env,dcshift,max,dcshift,max,dcshift,max)
{
}
///

/// TrivialTrafo::~TrivialTrafo
template<typename internal,typename external,int count>
TrivialTrafo<internal,external,count>::~TrivialTrafo(void)
{
}
///

/// TrivialTrafo::RGB2YCbCr
// Transform a block from RGB to YCbCr. Input are the three image bitmaps
// already clipped to the rectangle to transform, the coordinate rectangle to use
// and the level shift.
template<typename internal,typename external,int count>
void TrivialTrafo<internal,external,count>::RGB2YCbCr(const RectAngle<LONG> &r,const struct ImageBitMap *const *source,
                                                      Buffer target)
{
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;

  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    switch(count) {
    case 4:
      memset(target[3],0,sizeof(Block));
      /* fall through */
    case 3:
      memset(target[2],0,sizeof(Block));
      /* fall through */
    case 2:
      memset(target[1],0,sizeof(Block));
      /* fall through */
    case 1:
      memset(target[0],0,sizeof(Block));
    }
  }
  
  for(x = 1; x < count;x++) {
    if (source[0]->ibm_ucPixelType != source[x]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"TrivialTrafo::RGB2YCbCr",
                "pixel types of all three components in a RGB to RGB conversion must be identical");
    }
  }

  {
    const external *rptr,*gptr,*bptr,*kptr;
    switch(count) {
    case 4:
      kptr = (const external *)(source[3]->ibm_pData);
      /* fall through */
    case 3:
      bptr = (const external *)(source[2]->ibm_pData);
      /* fall through */
    case 2:
      gptr = (const external *)(source[1]->ibm_pData);
      /* fall through */
    case 1:
      rptr = (const external *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      internal *ydst,*cbdst,*crdst,*kdst;
      const external *r,*g,*b,*k;
      switch(count) {
      case 4:
        kdst    = (internal *)target[3] + xmin + (y << 3);
        k       = kptr;
        /* fall through */
      case 3:
        crdst   = (internal *)target[2] + xmin + (y << 3);
        b       = bptr;
        /* fall through */
      case 2:
        cbdst   = (internal *)target[1] + xmin + (y << 3);
        g       = gptr;
        /* fall through */
      case 1:
        ydst    = (internal *)target[0] + xmin + (y << 3);
        r       = rptr;
      }
      for(x = xmin;x <= xmax;x++) { 
        switch(count) {
        case 4:
          *kdst = *k;
          assert(TypeTrait<external>::isFloat || *kdst <= m_lMax);
          kdst++;
          /* fall through */
        case 3:
          *crdst = *b;
          assert(TypeTrait<external>::isFloat || *crdst <= m_lMax);
          crdst++;
          b  = (const external *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
          /* fall through */
        case 2:
          *cbdst = *g;
          assert(TypeTrait<external>::isFloat || *cbdst <= m_lMax);
          cbdst++;
          g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
          /* fall through */
        case 1:
          *ydst  = *r;
          assert(TypeTrait<external>::isFloat || *ydst <= m_lMax);
          ydst++;
          r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
        }
      }
      switch(count) {
      case 4:
        kptr  = (const external *)((const UBYTE *)(kptr) + source[3]->ibm_lBytesPerRow);
        /* fall through */
      case 3:
        bptr  = (const external *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
        /* fall through */
      case 2:
        gptr  = (const external *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
        /* fall through */
      case 1:
        rptr  = (const external *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// TrivialTrafo::YCbCr2RGB
// Inverse transform a block from YCbCr to RGB, including a clipping operation and a dc level
// shift.
template<typename internal,typename external,int count>
void TrivialTrafo<internal,external,count>::YCbCr2RGB(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
                                                      Buffer source,Buffer)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  
  if (TypeTrait<external>::isFloat == false && m_lMax > TypeTrait<external>::Max) {
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
    external *rptr,*gptr,*bptr,*kptr;
    switch(count) {
    case 4:
      kptr = (external *)(dest[3]->ibm_pData);
      /* fall through */
    case 3:
      bptr = (external *)(dest[2]->ibm_pData);
      /* fall through */
    case 2:
      gptr = (external *)(dest[1]->ibm_pData);
      /* fall through */
    case 1:
      rptr = (external *)(dest[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      internal *ysrc,*cbsrc,*crsrc,*ksrc;
      external *r,*g,*b,*k;
      
      switch(count) {
      case 4:
        ksrc   = (internal *)source[3]   + xmin + (y << 3);
        k      = kptr;
        /* fall through */
      case 3:
        crsrc  = (internal *)source[2]   + xmin + (y << 3);
        b      = bptr;
        /* fall through */
      case 2:
        cbsrc  = (internal *)source[1]   + xmin + (y << 3);
        g      = gptr;
        /* fall through */
      case 1:
        ysrc   = (internal *)source[0]   + xmin + (y << 3);
        r      = rptr;
      }
      for(x = xmin;x <= xmax;x++) {
        internal rv,gv,bv,kv;

        switch(count) {
        case 4:
          kv = *ksrc++;
          if (!TypeTrait<external>::isFloat) {
            if (kv < 0)      kv = 0;
            if (kv > m_lMax) kv = m_lMax;
          }
          *k = kv;
          k  = (external *)((UBYTE *)(k) + dest[3]->ibm_cBytesPerPixel);
          /* fall through */
        case 3:
          bv = *crsrc++; 
          if (!TypeTrait<external>::isFloat) {
            if (bv < 0)      bv = 0;
            if (bv > m_lMax) bv = m_lMax;
          }
          *b = bv;
          b  = (external *)((UBYTE *)(b) + dest[2]->ibm_cBytesPerPixel);
          /* fall through */
        case 2:
          gv = *cbsrc++;
          if (!TypeTrait<external>::isFloat) {
            if (gv < 0)      gv = 0;
            if (gv > m_lMax) gv = m_lMax;
          }
          *g = gv;
          g  = (external *)((UBYTE *)(g) + dest[1]->ibm_cBytesPerPixel);
          /* fall through */
        case 1:
          rv = *ysrc++;
          if (!TypeTrait<external>::isFloat) {
            if (rv < 0)      rv = 0;
            if (rv > m_lMax) rv = m_lMax;
          }
          *r = rv;
          r  = (external *)((UBYTE *)(r) + dest[0]->ibm_cBytesPerPixel);
        }
      }
      switch(count) {
      case 4:
        kptr  = (external *)((UBYTE *)(kptr) + dest[3]->ibm_lBytesPerRow);
        /* fall through */
      case 3:
        bptr  = (external *)((UBYTE *)(bptr) + dest[2]->ibm_lBytesPerRow);
        /* fall through */
      case 2:
        gptr  = (external *)((UBYTE *)(gptr) + dest[1]->ibm_lBytesPerRow);
        /* fall through */
      case 1:
        rptr  = (external *)((UBYTE *)(rptr) + dest[0]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// Explicit instantiations
template class TrivialTrafo<LONG,UBYTE,1>;
template class TrivialTrafo<LONG,UWORD,1>;
template class TrivialTrafo<LONG,UBYTE,2>;
template class TrivialTrafo<LONG,UWORD,2>;
template class TrivialTrafo<LONG,LONG,1>;
template class TrivialTrafo<LONG,UBYTE,3>;
template class TrivialTrafo<LONG,UWORD,3>;
template class TrivialTrafo<LONG,LONG,3>;
template class TrivialTrafo<FLOAT,FLOAT,1>;
template class TrivialTrafo<FLOAT,FLOAT,3>;
template class TrivialTrafo<LONG,UBYTE,4>;
template class TrivialTrafo<LONG,UWORD,4>;
template class TrivialTrafo<LONG,LONG,4>;
///

