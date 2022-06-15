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
** This file provides the transformation from RGB to YCbCr
**
** $Id: ycbcrtrafo.cpp,v 1.78 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "colortrafo/ycbcrtrafo.hpp"
#include "tools/traits.hpp"
#include "tools/numerics.hpp"
#include "boxes/mergingspecbox.hpp"
#include "std/string.hpp"
///

/// Defines
// Apply a lookup table to the argument x, clamp x to range. If the
// LUT is not there, return x identitical.
#define APPLY_LUT(lut,max,x) ((lut)?((lut)[((x) >= 0)?(((x) <= LONG(max))?(x):(LONG(max))):(0)]):(x))
// Clamp a coefficient in range 0,max
#define CLAMP(max,x) (((x) >= 0)?(((x) <= LONG(max))?(x):(LONG(max))):(0))
// Wrap-around logic
#define WRAP(max,x)  UWORD((x) & (max))
// 
// For half-floats, invert the mapping for negative numbers.
#define INVERT_NEGS(x) WORD(((WORD(x) >> 15) & 0x7fff)^(x))
///

/// YCbCrTrafo::YCbCrTrafo
template<typename external,int count,UBYTE oc,int trafo,int rtrafo>
YCbCrTrafo<external,count,oc,trafo,rtrafo>::YCbCrTrafo(class Environ *env,LONG dcshift,LONG max,
                                                       LONG rdcshift,LONG rmax,LONG outshift,LONG outmax)
      : IntegerTrafo(env,dcshift,max,rdcshift,rmax,outshift,outmax), m_TrivialHelper(env,outshift,outmax)
{
}
///

/// YCbCrTrafo::~YCbCrTrafo
template<typename external,int count,UBYTE oc,int trafo,int rtrafo>
YCbCrTrafo<external,count,oc,trafo,rtrafo>::~YCbCrTrafo(void)
{
}
///

/// YCbCrTrafo::RGB2YCbCr
// Transform a block from RGB to YCbCr. Input are the three image bitmaps
// already clipped to the rectangle to transform, the coordinate rectangle to use
// and the level shift.
template<typename external,int count,UBYTE oc,int trafo,int rtrafo>
void YCbCrTrafo<external,count,oc,trafo,rtrafo>::RGB2YCbCr(const RectAngle<LONG> &r,
                                                           const struct ImageBitMap *const *source,
                                                           Buffer target)
{
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;

  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    for(x = 0;x < 64;x++) {
      // LDR data is always preshifted by COLOR_BITS
      switch(count) {
      case 4:
        target[3][x] = m_lDCShift << COLOR_BITS;
        // fall through
      case 3:
        target[2][x] = m_lDCShift << COLOR_BITS;
        // fall through
      case 2:
        target[1][x] = m_lDCShift << COLOR_BITS;
        // fall through
      case 1:
        target[0][x] = m_lDCShift << COLOR_BITS;
      }
    }
  }

  {
    const external *rptr,*gptr,*bptr,*kptr;
    switch(count) {
    case 4:
      kptr = (const external *)(source[3]->ibm_pData);
      // fall through
    case 3:
      bptr = (const external *)(source[2]->ibm_pData);
      // fall through
    case 2:
      gptr = (const external *)(source[1]->ibm_pData);
      // fall through
    case 1:
      rptr = (const external *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst,*kdst;
      const external *r,*g,*b,*k;
      switch(count) {
      case 4:
        kdst    = target[3] + xmin + (y << 3);
        k       = kptr;
        // fall through
      case 3:
        crdst   = target[2] + xmin + (y << 3);
        b       = bptr;
        // fall through
      case 2:
        cbdst   = target[1] + xmin + (y << 3);
        g       = gptr;
        // fall through
      case 1:
        ydst    = target[0] + xmin + (y << 3);
        r       = rptr;
      }
      //
      // Only the L-tables are used here.
      for(x = xmin;x <= xmax;x++) { 
        LONG rv,gv,bv,kv;
        LONG y,cb,cr;
        
        switch(count) {
        case 4:
          kv     = *k;
          *kdst  = INT_TO_COLOR(kv);
          assert(*kdst  <= ((m_lMax + 1) << COLOR_BITS) - 1);
          kdst++;
          k  = (const external *)((const UBYTE *)(k) + source[3]->ibm_cBytesPerPixel);
          // Runs into the following.
        case 3:
          // Run the forwards C transformation.
          if (oc & Extended) {
            rv = FIX_TO_INT(QUAD(*r) * m_lCFwd[0] + QUAD(*g) * m_lCFwd[1] + QUAD(*b) * m_lCFwd[2]);
            gv = FIX_TO_INT(QUAD(*r) * m_lCFwd[3] + QUAD(*g) * m_lCFwd[4] + QUAD(*b) * m_lCFwd[5]);
            bv = FIX_TO_INT(QUAD(*r) * m_lCFwd[6] + QUAD(*g) * m_lCFwd[7] + QUAD(*b) * m_lCFwd[8]);
            rv = APPLY_LUT(m_plEncodingLUT[0],m_lOutMax,rv);
            gv = APPLY_LUT(m_plEncodingLUT[1],m_lOutMax,gv);
            bv = APPLY_LUT(m_plEncodingLUT[2],m_lOutMax,bv);
          } else {
            rv = *r;
            gv = *g;
            bv = *b;
          }
          switch(trafo) {
          case MergingSpecBox::YCbCr:
            // Offset data such that it is preshifted by COLOR_BITS
            y  = FIX_TO_COLOR(QUAD(rv) * m_lLFwd[0] + QUAD(gv) * m_lLFwd[1] + QUAD(bv) * m_lLFwd[2]);
            cb = FIX_TO_COLOR(QUAD(rv) * m_lLFwd[3] + QUAD(gv) * m_lLFwd[4] + QUAD(bv) * m_lLFwd[5] + 
                              (QUAD(m_lDCShift) << FIX_BITS));
            cr = FIX_TO_COLOR(QUAD(rv) * m_lLFwd[6] + QUAD(gv) * m_lLFwd[7] + QUAD(bv) * m_lLFwd[8] + 
                              (QUAD(m_lDCShift) << FIX_BITS));
            // If this is not the traditional RGB2YCbCr, overflows may happen. This is the encoder,
            // so we can do what we want. Just clamp in this case.
            *ydst  = CLAMP(((m_lMax + 1) << COLOR_BITS) - 1,y);
            *cbdst = CLAMP(((m_lMax + 1) << COLOR_BITS) - 1,cb);
            *crdst = CLAMP(((m_lMax + 1) << COLOR_BITS) - 1,cr);
            //
            assert(*ydst  <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*cbdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*crdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            break;
          case MergingSpecBox::Identity:
            *ydst  = INT_TO_COLOR(rv);
            *cbdst = INT_TO_COLOR(gv);
            *crdst = INT_TO_COLOR(bv);
            assert(*ydst  <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*cbdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*crdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            break;
          }
          ydst++,cbdst++,crdst++;
          r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
          g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
          b  = (const external *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
          break;
        case 2:
          *cbdst = INT_TO_COLOR(m_plEncodingLUT[1][*g]);
          cbdst++;
          g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
          // fall through
        case 1: 
          *ydst = INT_TO_COLOR(m_plEncodingLUT[0][*r]);
          ydst++;
          r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
          break;
        }
      }
      switch(count) {
      case 4:
        kptr  = (const external *)((const UBYTE *)(kptr) + source[3]->ibm_lBytesPerRow);
        // fall through
      case 3:
        bptr  = (const external *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
        // fall through
      case 2:
        gptr  = (const external *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
        // fall through
      case 1:
        rptr  = (const external *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// YCbCrTrafo::LDRRGB2YCbCr
// Transform a block from RGB to YCbCr. Input are the three image bitmaps
// already clipped to the rectangle to transform, the coordinate rectangle to use
// and the level shift.
template<typename external,int count,UBYTE oc,int trafo,int rtrafo>
void YCbCrTrafo<external,count,oc,trafo,rtrafo>::LDRRGB2YCbCr(const RectAngle<LONG> &r,
                                                              const struct ImageBitMap *const *source,
                                                              Buffer target)
{
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;

  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    for(x = 0;x < 64;x++) {
      // LDR Data is always preshifted by COLOR_BITS
      switch(count) {
      case 4:
        target[3][x] = m_lDCShift << COLOR_BITS;
        // fall through
      case 3:
        target[2][x] = m_lDCShift << COLOR_BITS;
        // fall through
      case 2:
        target[1][x] = m_lDCShift << COLOR_BITS;
        // fall through
      case 1:
        target[0][x] = m_lDCShift << COLOR_BITS;
        }
    }
  }
  
  {
    const UBYTE *rptr,*gptr,*bptr,*kptr;
    switch(count) {
    case 4:
      kptr = (const UBYTE *)(source[3]->ibm_pData);
      // fall through
    case 3:
      bptr = (const UBYTE *)(source[2]->ibm_pData);
      // fall through
    case 2:
      gptr = (const UBYTE *)(source[1]->ibm_pData);
      // fall through
    case 1:
      rptr = (const UBYTE *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst,*kdst;
      const UBYTE *r,*g,*b,*k;
      switch(count) {
      case 4:
        kdst    = target[3] + xmin + (y << 3);
        k       = kptr;
        // fall through
      case 3:
        crdst   = target[2] + xmin + (y << 3);
        b       = bptr;
        // fall through
      case 2:
        cbdst   = target[1] + xmin + (y << 3);
        g       = gptr;
        // fall through
      case 1:
        ydst    = target[0] + xmin + (y << 3);
        r       = rptr;
      }
      //
      // No tables used at all, user already supplies a tone mapped image.
      for(x = xmin;x <= xmax;x++) { 
        UBYTE rv,gv,bv,kv;
        
        switch(count) {
        case 4:
          kv    = *k;
          *kdst = INT_TO_COLOR(kv);
          kdst++;
          k    += source[3]->ibm_cBytesPerPixel;
          // fall through
        case 3:
          rv = *r;
          gv = *g;
          bv = *b;
          switch(trafo) {
          case MergingSpecBox::YCbCr:
            // Offset data such that it is preshifted by COLOR_BITS
            // THIS IS NOT A TYPO! The LDR data is (for legacy reasons) always
            // in the 601 color space and requires exactly this transformation,
            // no matter what the user specifies.
            *ydst  = FIX_TO_COLOR(QUAD(rv) *  TO_FIX(0.29900)      +
                                  QUAD(gv) *  TO_FIX(0.58700)      +
                                  QUAD(bv) * TO_FIX(0.11400));
            *cbdst = FIX_TO_COLOR(QUAD(rv) * -TO_FIX(0.1687358916) +
                                  QUAD(gv) * -TO_FIX(0.3312641084) +
                                  QUAD(bv) * TO_FIX(0.5) +
                                  (QUAD(m_lDCShift) << FIX_BITS));
            *crdst = FIX_TO_COLOR(QUAD(rv) *  TO_FIX(0.5)          +
                                  QUAD(gv) * -TO_FIX(0.4186875892) +
                                  QUAD(bv) *-TO_FIX(0.08131241085) +
                                  (QUAD(m_lDCShift) << FIX_BITS));
            assert(*ydst  <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*cbdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*crdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            break;
          case MergingSpecBox::Identity:
            *ydst  = INT_TO_COLOR(rv);
            *cbdst = INT_TO_COLOR(gv);
            *crdst = INT_TO_COLOR(bv);
            assert(*ydst  <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*cbdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            assert(*crdst <= ((m_lMax + 1) << COLOR_BITS) - 1);
            break;
          }
          ydst++,cbdst++,crdst++;
          r += source[0]->ibm_cBytesPerPixel;
          g += source[1]->ibm_cBytesPerPixel;
          b += source[2]->ibm_cBytesPerPixel;
          break;
        case 2:
          *cbdst = INT_TO_COLOR(*g);
          cbdst++;
          g  += source[1]->ibm_cBytesPerPixel;
          // fall through
        case 1: 
          *ydst = INT_TO_COLOR(*r);
          ydst++;
          r  += source[0]->ibm_cBytesPerPixel;
          break;
        }
      }
      switch(count) {
      case 4:
        kptr += source[3]->ibm_lBytesPerRow;
        // fall through
      case 3:
        bptr += source[2]->ibm_lBytesPerRow;
        // fall through
      case 2:
        gptr += source[1]->ibm_lBytesPerRow;
        // fall through
      case 1:
        rptr += source[0]->ibm_lBytesPerRow;
      }
    }
  }
}
///

/// YCbCrTrafo::RGB2Residual
// Compute the residual from the original image and the decoded LDR image, place result in
// the output buffer. This depends rather on the coding model.
template<typename external,int count,UBYTE oc,int trafo,int rtrafo>
void YCbCrTrafo<external,count,oc,trafo,rtrafo>::RGB2Residual(const RectAngle<LONG> &r,
                                                              const struct ImageBitMap *const *source,
                                                              Buffer reconstructed,Buffer residual)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;

  //
  // makes little sense to call that without a residual
  assert(oc & Residual);
  // There is no JPEG XT support for four component images.
  assert(count < 4);
  assert(m_lOutMax <= TypeTrait<external>::Max);
  
  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    for(x = 0;x < 64;x++) {
      switch(rtrafo) {
      case MergingSpecBox::Identity:
        switch(count) {
        case 3:
          residual[2][x] = m_lRDCShift;
          // fall through
        case 2:
          residual[1][x] = m_lRDCShift;
          // fall through
        case 1:
          residual[0][x] = m_lRDCShift;
        }
        break;
      case MergingSpecBox::YCbCr:
        // Data is preshifted by COLOR_BITS
        switch(count) {
        case 3:
          residual[2][x] = m_lRDCShift << COLOR_BITS;
          residual[1][x] = m_lRDCShift << COLOR_BITS;
          // fall through
        case 1:
          residual[0][x] = m_lRDCShift << COLOR_BITS;
        }
        break;
      case MergingSpecBox::RCT:
        // Chroma data has one extra bit which is included in the frame precision
        switch(count) {
        case 3:
          residual[2][x] = m_lRDCShift;
          residual[1][x] = m_lRDCShift;
          // fall through
        case 1:
          residual[0][x] = m_lRDCShift;
        }
        break;
      case MergingSpecBox::Zero:
        // No transformation, this should not be called.
        assert(!"Attempt to compute a residual that is supposed to be absent");
        break;
      }
    }
  }
  
  {
    const external *rptr,*gptr,*bptr;
    switch(count) {
    case 3:
      bptr = (const external *)(source[2]->ibm_pData);
      // fall through
    case 2:
      gptr = (const external *)(source[1]->ibm_pData);
      // fall through
    case 1:
      rptr = (const external *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst;
      LONG *yrec,*cbrec,*crrec;
      const external *r,*g,*b;
      LONG rr,rg,rb;

      switch(count) {
      case 3:
        crdst   = residual[2]      + xmin + (y << 3);
        crrec   = reconstructed[2] + xmin + (y << 3);
        b       = bptr;
        // fall through
      case 2:
        cbdst   = residual[1]      + xmin + (y << 3);
        cbrec   = reconstructed[1] + xmin + (y << 3);
        g       = gptr;
        // fall through
      case 1:
        ydst    = residual[0]      + xmin + (y << 3);
        yrec    = reconstructed[0] + xmin + (y << 3);
        r       = rptr;
      }
      
      for(x = xmin;x <= xmax;x++) {
        LONG y,cb,cr,rv,gv,bv;
        
        // First the L-transformation of the legacy data. This transforms
        // the encoded and reconstructed data back from the RGB space.
        switch(count) {
        case 3:
          switch(trafo) {
          case MergingSpecBox::YCbCr:
            // Data arrives preshifted by COLOR_BITS here.
            cr = *crrec - (m_lDCShift << COLOR_BITS);
            cb = *cbrec - (m_lDCShift << COLOR_BITS);
            rv = FIX_COLOR_TO_INT(QUAD(*yrec) * m_lL[0] + QUAD(cb) * m_lL[1] + QUAD(cr) * m_lL[2]);
            gv = FIX_COLOR_TO_INT(QUAD(*yrec) * m_lL[3] + QUAD(cb) * m_lL[4] + QUAD(cr) * m_lL[5]);
            bv = FIX_COLOR_TO_INT(QUAD(*yrec) * m_lL[6] + QUAD(cb) * m_lL[7] + QUAD(cr) * m_lL[8]);
            break;
          case MergingSpecBox::Identity:
            rv = COLOR_TO_INT(*yrec);
            gv = COLOR_TO_INT(*cbrec);
            bv = COLOR_TO_INT(*crrec);
            break;
          }
          yrec++,cbrec++,crrec++;
          //
          // Followed by the L-Lut. Data is now all in integer.
          rv = APPLY_LUT(m_plDecodingLUT[0],m_lMax,rv);
          gv = APPLY_LUT(m_plDecodingLUT[1],m_lMax,gv);
          bv = APPLY_LUT(m_plDecodingLUT[2],m_lMax,bv);
          //
          // Followed by the C-Transformation.
          rr = FIX_TO_INT(QUAD(rv) * m_lC[0] + QUAD(gv) * m_lC[1] + QUAD(bv) * m_lC[2]);
          rg = FIX_TO_INT(QUAD(rv) * m_lC[3] + QUAD(gv) * m_lC[4] + QUAD(bv) * m_lC[5]);
          rb = FIX_TO_INT(QUAD(rv) * m_lC[6] + QUAD(gv) * m_lC[7] + QUAD(bv) * m_lC[8]);
          //
          // No truncation here. Now compute the residual
          if (oc & Float) {
            rr = INVERT_NEGS(*r) - rr;
            rg = INVERT_NEGS(*g) - rg;
            rb = INVERT_NEGS(*b) - rb;
          } else {
            rr = *r - rr;
            rg = *g - rg;
            rb = *b - rb;
          }
          //
          // Advance to the next pixel.
          r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
          g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
          b  = (const external *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
          break;
        case 2:
          gv = COLOR_TO_INT(*cbrec);
          rg = APPLY_LUT(m_plDecodingLUT[1],m_lMax,gv);
          if (oc & Float) {
            rg = INVERT_NEGS(*g) - rg;
          } else {
            rg = *g - rg;
          }
          cbrec++;
          g  = (const external *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
          // fall through
        case 1:
          rv = COLOR_TO_INT(*yrec);
          rr = APPLY_LUT(m_plDecodingLUT[0],m_lMax,rv);
          if (oc & Float) {
            rr = INVERT_NEGS(*r) - rr;
          } else {
            rr = *r - rr;
          }
          yrec++;
          r  = (const external *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
          break;
        } // of switch over components.
        //
        // The residuals are now in ra,rr,rg,rb. The transformation works here
        // differently: Coefficients are *first* transformed, and then go into the
        // LUT.
        switch(count) {
        case 3:
          rr += m_lCreating2Shift;
          rg += m_lCreating2Shift;
          rb += m_lCreating2Shift;
          switch(rtrafo) {
          case MergingSpecBox::YCbCr:
            // Go into the secondary R-tables first.
            rr = APPLY_LUT(m_plCreating2LUT[0],((m_lOutMax + 1) << 1) - 1,rr);
            rg = APPLY_LUT(m_plCreating2LUT[1],((m_lOutMax + 1) << 1) - 1,rg);
            rb = APPLY_LUT(m_plCreating2LUT[2],((m_lOutMax + 1) << 1) - 1,rb);
            // Generate data that is preshifted by rdcshift << COLOR_BITS
            y  = FIXCOLOR_TO_COLOR(QUAD(rr) * m_lRFwd[0] + QUAD(rg) * m_lRFwd[1] + QUAD(rb) * m_lRFwd[2]);
            cb = FIXCOLOR_TO_COLOR(QUAD(rr) * m_lRFwd[3] + QUAD(rg) * m_lRFwd[4] + QUAD(rb) * m_lRFwd[5] + 
                                   (QUAD(m_lOutDCShift) << (FIX_BITS + COLOR_BITS)));
            cr = FIXCOLOR_TO_COLOR(QUAD(rr) * m_lRFwd[6] + QUAD(rg) * m_lRFwd[7] + QUAD(rb) * m_lRFwd[8] + 
                                   (QUAD(m_lOutDCShift) << (FIX_BITS + COLOR_BITS)));
            y  = APPLY_LUT(m_plCreatingLUT[0],((m_lOutMax + 1) << COLOR_BITS) - 1,y );
            cb = APPLY_LUT(m_plCreatingLUT[1],((m_lOutMax + 1) << COLOR_BITS) - 1,cb);
            cr = APPLY_LUT(m_plCreatingLUT[2],((m_lOutMax + 1) << COLOR_BITS) - 1,cr);
            break;
          case MergingSpecBox::RCT:
            // Generate data where the chroma has an extended range of one bit.
            // No LUT here.
            // FIX: First, apply modulo arithmetic by converting everything
            // into the range of N bits, where N is the output precision.
            rr = rr & m_lOutMax;
            rg = rg & m_lOutMax;
            rb = rb & m_lOutMax;
            // is by that preshifted by rdcshift << 1
            // Note that the range of y is between 0..2^17-1. The quantization
            // must be at least 2 for Y.
            // The mysteriously looking lines below compute the signed modulo for (m_ulOutDCShift << 1)
            cb  = ~(((rb - rg) & m_lOutDCShift) - 1) | ((rb - rg) & (m_lOutDCShift - 1));
            cr  = ~(((rr - rg) & m_lOutDCShift) - 1) | ((rr - rg) & (m_lOutDCShift - 1));
            y   = ((rg + ((cb + cr) >> 2)) & m_lOutMax) << 1;
            cb += (m_lOutDCShift << 1);
            cr += (m_lOutDCShift << 1);
            assert(y >= 0  && y  < (1 << 17));
            assert(cb >= 0 && cb < (1 << 17));
            assert(cr >= 0 && cr < (1 << 17));
            y  = APPLY_LUT(m_plCreatingLUT[0] ,((m_lOutMax + 1) << 1) - 1,y );
            cb = APPLY_LUT(m_plCreatingLUT[1] ,((m_lOutMax + 1) << 1) - 1,cb);
            cr = APPLY_LUT(m_plCreatingLUT[2] ,((m_lOutMax + 1) << 1) - 1,cr);
            break;
          case MergingSpecBox::Identity:
            if (oc & ClampFlag) {
              rr = APPLY_LUT(m_plCreating2LUT[0],((m_lOutMax + 1) << 1) - 1,rr);
              rg = APPLY_LUT(m_plCreating2LUT[1],((m_lOutMax + 1) << 1) - 1,rg);
              rb = APPLY_LUT(m_plCreating2LUT[2],((m_lOutMax + 1) << 1) - 1,rb);
              y  = APPLY_LUT(m_plCreatingLUT[0] ,((m_lOutMax + 1) << COLOR_BITS) - 1,rr);
              cb = APPLY_LUT(m_plCreatingLUT[1] ,((m_lOutMax + 1) << COLOR_BITS) - 1,rg);
              cr = APPLY_LUT(m_plCreatingLUT[2] ,((m_lOutMax + 1) << COLOR_BITS) - 1,rb);
            } else {
              y  = APPLY_LUT(m_plCreatingLUT[0] ,m_lOutMax,rr & m_lOutMax);
              cb = APPLY_LUT(m_plCreatingLUT[1] ,m_lOutMax,rg & m_lOutMax);
              cr = APPLY_LUT(m_plCreatingLUT[2] ,m_lOutMax,rb & m_lOutMax);
            }
            break;
          default:
            assert(!"Unsupported R transformation");
            y = cb = cr = 0; // shut up the compiler
          }
          *crdst++ = cr;
          *cbdst++ = cb;
          *ydst++  = y;
          break;
        case 2:
          rg += m_lCreating2Shift;
          if (oc & ClampFlag) {
            rg  = APPLY_LUT(m_plCreating2LUT[1],((m_lOutMax + 1) << 1) - 1,rg);
            rg  = APPLY_LUT(m_plCreatingLUT[1] ,((m_lOutMax + 1) << COLOR_BITS) - 1,rg);
          } else {
            rg  = APPLY_LUT(m_plCreatingLUT[1] ,m_lOutMax,rg & m_lOutMax);
          }
          *cbdst++ = rg;
          // fall through
        case 1:
          rr += m_lCreating2Shift;
          if (oc & ClampFlag) {
            rr  = APPLY_LUT(m_plCreating2LUT[0],((m_lOutMax + 1) << 1) - 1,rr);
            rr  = APPLY_LUT(m_plCreatingLUT[0] ,((m_lOutMax + 1) << COLOR_BITS) - 1,rr);
          } else {
            rr  = APPLY_LUT(m_plCreatingLUT[0] ,m_lOutMax,rr & m_lOutMax);
          }
          *ydst++ = rr;
        }
      } // of loop over x
      switch(count) {
      case 3:
        bptr  = (const external *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
        // fall through
      case 2:
        gptr  = (const external *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
        // fall through
      case 1:
        rptr  = (const external *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
      }
    } // of loop over y
  } 
}
///
 
/// YCbCrTrafo::YCbCr2RGB
// Inverse transform a block from YCbCr to RGB, including a clipping operation and a dc level
// shift.
template<typename external,int count,UBYTE oc,int trafo,int rtrafo>
void YCbCrTrafo<external,count,oc,trafo,rtrafo>::YCbCr2RGB(const RectAngle<LONG> &r,
                                                           const struct ImageBitMap *const *dest,
                                                           Buffer source,Buffer residual)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  
  assert(source);
  
  if (m_lOutMax > TypeTrait<external>::Max) {
    JPG_THROW(OVERFLOW_PARAMETER,"YCbCrTrafo::YCbCr2RGB",
              "RGB maximum intensity for pixel type does not fit into the type");
  }

  {
    external *rptr,*gptr,*bptr,*kptr;
    switch(count) {
    case 4:
      kptr = (external *)(dest[3]->ibm_pData);
      // fall through
    case 3:
      bptr = (external *)(dest[2]->ibm_pData);
      // fall through
    case 2:
      gptr = (external *)(dest[1]->ibm_pData);
      // fall through
    case 1:
      rptr = (external *)(dest[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ysrc,*cbsrc,*crsrc,*ksrc;
      LONG *rysrc = NULL,*rcbsrc = NULL,*rcrsrc = NULL;
      external *r,*g,*b,*k;
      switch(count) {
      case 4:
        ksrc     = source[3]   + xmin + (y << 3);
        k        = kptr;
        assert(!residual); // No residual coding with four components.
        // fall through
      case 3:
        crsrc    = source[2]   + xmin + (y << 3);       
        b        = bptr;
        if (residual) {
          rcrsrc = residual[2] + xmin + (y << 3);
        }
        // fall through
      case 2:
        cbsrc    = source[1]   + xmin + (y << 3);
        g        = gptr;
        if (residual) {
          rcbsrc = residual[1] + xmin + (y << 3);
        }
        // fall through
      case 1:
        ysrc     = source[0]   + xmin + (y << 3);       
        r        = rptr;
        if (residual) {
          rysrc  = residual[0] + xmin + (y << 3);
        }
      }

      for(x = xmin;x <= xmax;x++) {
        LONG cr,y,cb,rv,gv,bv,kv;
        LONG rx,gx,bx;
        LONG rr = m_lOutDCShift;
        LONG rg = m_lOutDCShift;
        LONG rb = m_lOutDCShift;

        if (oc & Residual) {
          // Compute the residual. Note that the LUT is here applied *first*, then
          // followed by the transformation.
          switch(count) {
          case 4:
            assert(!"residual coding is not supported with four components");
            // fall through
          case 3:
            switch(rtrafo) {
            case MergingSpecBox::RCT: // Perform the RCT.
              // Everything has one extra bit
              y   = *rysrc++;
              cb  = *rcbsrc++;
              cr  = *rcrsrc++;
              y   = APPLY_LUT(m_plResidualLUT[0],m_lRMax,y );
              cb  = APPLY_LUT(m_plResidualLUT[1],m_lRMax,cb);
              cr  = APPLY_LUT(m_plResidualLUT[2],m_lRMax,cr);
              y   = y >> 1; // Remove the one bit preshift
              cb  = cb - (m_lOutDCShift << 1);
              cr  = cr - (m_lOutDCShift << 1);
              rg  = (y  - ((cb + cr) >> 2)) & m_lOutMax;
              rr  = (cr + rg)               & m_lOutMax;
              rb  = (cb + rg)               & m_lOutMax;
              break;
            case MergingSpecBox::YCbCr:
              // Input data is here preshifted.
              y   = *rysrc++;
              cb  = *rcbsrc++;
              cr  = *rcrsrc++;
              y   = APPLY_LUT(m_plResidualLUT[0],((m_lRMax + 1) << COLOR_BITS) - 1,y );
              cb  = APPLY_LUT(m_plResidualLUT[1],((m_lRMax + 1) << COLOR_BITS) - 1,cb);
              cr  = APPLY_LUT(m_plResidualLUT[2],((m_lRMax + 1) << COLOR_BITS) - 1,cr);
              cb -= (m_lOutDCShift << COLOR_BITS);
              cr -= (m_lOutDCShift << COLOR_BITS);
              rr  = FIX_COLOR_TO_INTCOLOR(QUAD(y) * m_lR[0] + QUAD(cb) * m_lR[1] + QUAD(cr) * m_lR[2]);
              rg  = FIX_COLOR_TO_INTCOLOR(QUAD(y) * m_lR[3] + QUAD(cb) * m_lR[4] + QUAD(cr) * m_lR[5]);
              rb  = FIX_COLOR_TO_INTCOLOR(QUAD(y) * m_lR[6] + QUAD(cb) * m_lR[7] + QUAD(cr) * m_lR[8]);
              // Apply the secondary LUT.
              rr  = APPLY_LUT(m_plResidual2LUT[0],((m_lOutMax + 1) << COLOR_BITS) - 1,rr);
              rg  = APPLY_LUT(m_plResidual2LUT[1],((m_lOutMax + 1) << COLOR_BITS) - 1,rg);
              rb  = APPLY_LUT(m_plResidual2LUT[2],((m_lOutMax + 1) << COLOR_BITS) - 1,rb);
              break;
            case MergingSpecBox::Identity:
              y   = *rysrc++;
              cb  = *rcbsrc++;
              cr  = *rcrsrc++;
              if (oc & ClampFlag) {
                rr  = APPLY_LUT(m_plResidualLUT[0] ,((m_lRMax + 1) << COLOR_BITS) - 1,y );
                rg  = APPLY_LUT(m_plResidualLUT[1] ,((m_lRMax + 1) << COLOR_BITS) - 1,cb);
                rb  = APPLY_LUT(m_plResidualLUT[2] ,((m_lRMax + 1) << COLOR_BITS) - 1,cr);
                // Apply the secondary LUT.
                rr  = APPLY_LUT(m_plResidual2LUT[0],((m_lOutMax + 1) << COLOR_BITS) - 1,rr);
                rg  = APPLY_LUT(m_plResidual2LUT[1],((m_lOutMax + 1) << COLOR_BITS) - 1,rg);
                rb  = APPLY_LUT(m_plResidual2LUT[2],((m_lOutMax + 1) << COLOR_BITS) - 1,rb);
              } else {
                rr  = APPLY_LUT(m_plResidualLUT[0],m_lRMax,y );
                rg  = APPLY_LUT(m_plResidualLUT[1],m_lRMax,cb);
                rb  = APPLY_LUT(m_plResidualLUT[2],m_lRMax,cr);
              }
              break;
            default:
              assert(!"Illegal R transformation found");
            }
            // R-transformation done.
            break;
          case 2:
            assert(!"residual coding is not supported with two components");
            // fall through
          case 1: 
            y  = *rysrc++;
            if (oc & ClampFlag) {
              rr = APPLY_LUT(m_plResidualLUT[0] ,((m_lRMax   + 1) << COLOR_BITS) - 1,y);
              rr = APPLY_LUT(m_plResidual2LUT[0],((m_lOutMax + 1) << COLOR_BITS) - 1,rr);
            } else {
              rr = APPLY_LUT(m_plResidualLUT[0],m_lRMax,y );
            }
            break;
          }
        }
        //
        // Residual done. Now go for the legacy stream.
        // Note that here unlike for the residual,
        // the L-transformation is applied first, then
        // comes the LUT.
        switch(count) {
        case 4:
          kv = COLOR_TO_INT(*ksrc++);
          // No residual supported.
          assert(!(oc & Extended));
          // fall through
        case 3:
          switch(trafo) {
          case MergingSpecBox::YCbCr:
            // Data arrives preshifted here.
            cr = *crsrc++ - (m_lDCShift << COLOR_BITS);
            cb = *cbsrc++ - (m_lDCShift << COLOR_BITS);
            y  = *ysrc++;
            rv = FIX_COLOR_TO_INT(QUAD(y) * m_lL[0] + QUAD(cb) * m_lL[1] + QUAD(cr) * m_lL[2]);
            gv = FIX_COLOR_TO_INT(QUAD(y) * m_lL[3] + QUAD(cb) * m_lL[4] + QUAD(cr) * m_lL[5]);
            bv = FIX_COLOR_TO_INT(QUAD(y) * m_lL[6] + QUAD(cb) * m_lL[7] + QUAD(cr) * m_lL[8]);
            break;
          case MergingSpecBox::Identity:
            rv = COLOR_TO_INT(*ysrc++);
            gv = COLOR_TO_INT(*cbsrc++);
            bv = COLOR_TO_INT(*crsrc++);
            break;
          default:
            assert(!"Invalid L transformation specified");
          }
          //
          // Only if there is something to merge.
          if (oc & Extended) {
            // Apply the L-Lut.
            rv = APPLY_LUT(m_plDecodingLUT[0],m_lMax,rv);
            gv = APPLY_LUT(m_plDecodingLUT[1],m_lMax,gv);
            bv = APPLY_LUT(m_plDecodingLUT[2],m_lMax,bv);
            //
            // Apply the C-Transformation.
            rx = FIX_TO_INT(QUAD(rv) * m_lC[0] + QUAD(gv) * m_lC[1] + QUAD(bv) * m_lC[2]);
            gx = FIX_TO_INT(QUAD(rv) * m_lC[3] + QUAD(gv) * m_lC[4] + QUAD(bv) * m_lC[5]);
            bx = FIX_TO_INT(QUAD(rv) * m_lC[6] + QUAD(gv) * m_lC[7] + QUAD(bv) * m_lC[8]);
            //
            // There is no clamping here.
            //
            // Merge LDR and HDR
            rv = rx + rr - m_lOutDCShift;
            gv = gx + rg - m_lOutDCShift;
            bv = bx + rb - m_lOutDCShift;
          }
          break;
        case 2:
          gv = COLOR_TO_INT(*cbsrc++);
          if (oc & Extended) {
            gv = APPLY_LUT(m_plDecodingLUT[1],m_lMax,gv) + rg - m_lOutDCShift;
          }
          // fall through
        case 1: 
          // Simple for one component.
          rv = COLOR_TO_INT(*ysrc++);
          if (oc & Extended) {
            rv = APPLY_LUT(m_plDecodingLUT[0],m_lMax,rv) + rr - m_lOutDCShift;
          }
          break;
        }
        // 
        // Write the output, clamp or round to range. Only necessary if there is a residual,
        // but does not hurt otherwise.
        if (oc & ClampFlag) {
          if (oc & Float) {
            // Avoid NANs. For that, compute the value of +INF and -INF.
            LONG pinf = (m_lOutMax >> 1) - (m_lOutMax >> 6) - 1;
            // The representation of -INF.
            LONG minf = INVERT_NEGS(pinf | 0x8000); 
            // Also, convert from complement representation to sign
            // magnitude representation.
            switch(count) {
            case 4:
              assert(!"floating point not supported for four components");
              // fall through
            case 3:
              bv = (bv > pinf)?(pinf):((bv < minf)?(minf):(bv));
              bv = INVERT_NEGS(bv);
              // fall through
            case 2:
              gv = (gv > pinf)?(pinf):((gv < minf)?(minf):(gv));
              gv = INVERT_NEGS(gv);
              // fall through
            case 1:
              rv = (rv > pinf)?(pinf):((rv < minf)?(minf):(rv));
              rv = INVERT_NEGS(rv);
            }
          } else {
            // For integers, clamp.
            switch(count) {
            case 4:
              kv = CLAMP(m_lOutMax,kv);
              // fall through
            case 3:
              bv = CLAMP(m_lOutMax,bv);
              // fall through
            case 2:
              gv = CLAMP(m_lOutMax,gv);
              // fall through
            case 1:
              rv = CLAMP(m_lOutMax,rv);
            }
          }
        } else {
          if (oc & Float) {
            // Always 16 bit. Convert from complement to sign-
            // magnitude representation.
            switch(count) {
            case 4:
              assert(!"floating point not supported for four components");
              // fall through
            case 3:
              bv = INVERT_NEGS(bv);
              // fall through
            case 2:
              gv = INVERT_NEGS(gv);
              // fall through
            case 1:
              rv = INVERT_NEGS(rv);
              // fall through
            }
          } else {
            // For integers, implement the wrap-around
            // logic.
            switch(count) {
            case 4:
              kv = WRAP(m_lOutMax,kv);
              // fall through
            case 3:
              bv = WRAP(m_lOutMax,bv);
              // fall through
            case 2:
              gv = WRAP(m_lOutMax,gv);
              // fall through
            case 1:
              rv = WRAP(m_lOutMax,rv);
            }
          }
        }
        //
        // Advance pointers and write results.
        switch(count) {
        case 4:
          if (k) *k = kv;
          k  = (external *)((UBYTE *)(k) + dest[3]->ibm_cBytesPerPixel);
          // fall through
        case 3:
          if (b) *b = bv;
          b  = (external *)((UBYTE *)(b) + dest[2]->ibm_cBytesPerPixel);
          // fall through
        case 2:
          if (g) *g = gv;
          g  = (external *)((UBYTE *)(g) + dest[1]->ibm_cBytesPerPixel);
          // fall through
        case 1:
          if (r) *r = rv;
          r  = (external *)((UBYTE *)(r) + dest[0]->ibm_cBytesPerPixel);
        }
      } // Of loop over x
      switch(count) {
      case 4:
        kptr  = (external *)((UBYTE *)(kptr) + dest[3]->ibm_lBytesPerRow);
        // fall through
      case 3:
        bptr  = (external *)((UBYTE *)(bptr) + dest[2]->ibm_lBytesPerRow);
        // fall through
      case 2:
        gptr  = (external *)((UBYTE *)(gptr) + dest[1]->ibm_lBytesPerRow);
        // fall through
      case 1:
        rptr  = (external *)((UBYTE *)(rptr) + dest[0]->ibm_lBytesPerRow);
      }
    }
  }
}
///

/// Explicit instantiations
// One component
template class YCbCrTrafo<UBYTE,1,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,1,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UWORD,1,ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,1,ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,1,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::Identity>;

template class YCbCrTrafo<UBYTE,1,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Identity>;

template class YCbCrTrafo<UWORD,1,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,1,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Identity>;

// Two components
template class YCbCrTrafo<UBYTE,2,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,2,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UWORD,2,ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,2,ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,2,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::Identity>;

template class YCbCrTrafo<UBYTE,2,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Identity>;

template class YCbCrTrafo<UWORD,2,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,2,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Identity>;

// Three components
template class YCbCrTrafo<UBYTE,3,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UBYTE,3,ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::YCbCr,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,3,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UBYTE,3,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::Zero>;

template class YCbCrTrafo<UWORD,3,ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::YCbCr,MergingSpecBox::Zero>;

template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::YCbCr,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::YCbCr,MergingSpecBox::Identity>;

template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::Identity>;


template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::YCbCr,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::Identity>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::YCbCr,MergingSpecBox::Identity>;


template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::YCbCr>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::YCbCr>;
template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::YCbCr>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag,MergingSpecBox::YCbCr,MergingSpecBox::YCbCr>;


template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::YCbCr>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::ClampFlag | ColorTrafo::Float,MergingSpecBox::YCbCr,MergingSpecBox::YCbCr>;


template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::RCT>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::Identity,MergingSpecBox::RCT>;
template class YCbCrTrafo<UBYTE,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::YCbCr,MergingSpecBox::RCT>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended,MergingSpecBox::YCbCr,MergingSpecBox::RCT>;

template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::Identity,MergingSpecBox::RCT>;
template class YCbCrTrafo<UWORD,3,ColorTrafo::Residual | ColorTrafo::Extended | ColorTrafo::Float,MergingSpecBox::YCbCr,MergingSpecBox::RCT>;

// Four components
template class YCbCrTrafo<UBYTE,4,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;
template class YCbCrTrafo<UWORD,4,ColorTrafo::ClampFlag,MergingSpecBox::Identity,MergingSpecBox::Zero>;



