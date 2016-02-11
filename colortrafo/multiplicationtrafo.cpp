/*
** This file provides the transformation from RGB to YCbCr in the floating
** point profiles, profiles A and B of 18477-7.
**
** $Id: multiplicationtrafo.cpp,v 1.15 2016/01/19 15:46:26 thor Exp $
**
*/

/// Includes
#include "boxes/mergingspecbox.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/floattrafo.hpp"
#include "colortrafo/multiplicationtrafo.hpp"
#include "tools/numerics.hpp"
#include "std/assert.hpp"
#if ISO_CODE
///

/// Defines
// Apply a lookup table to the argument x, clamp x to range. If the
// LUT is not there, return x identitical.
#define APPLY_LUT(lut,max,x) ((lut)?((lut)[((x) >= 0)?(((x) <= LONG(max))?(x):(LONG(max))):(0)]):(x))
// Clamp a coefficient in range 0,max
#define CLAMP(max,x) (((x) >= 0)?(((x) <= LONG(max))?(x):(LONG(max))):(0))
///

/// MultiplicationTrafo::MultiplicationTrafo
template<int count,int trafo,int rtrafo,bool diagonal>
MultiplicationTrafo<count,trafo,rtrafo,diagonal>::MultiplicationTrafo(class Environ *env,
								      LONG dcshift,LONG max,
								      LONG rdcshift,LONG rmax,
								      LONG outshift,LONG outmax)
  : FloatTrafo(env,dcshift,max,rdcshift,rmax,outshift,outmax), m_TrivialHelper(env,outshift,outmax)
{
}
///

/// MultiplicationTrafo::~MultiplicationTrafo
template<int count,int trafo,int rtrafo,bool diagonal>
MultiplicationTrafo<count,trafo,rtrafo,diagonal>::~MultiplicationTrafo(void)
{
}
///

/// MultiplicationTrafo::RGB2YCbCr
// Transform an HDR block directly to an LDR block with the given TMOs.
// Do not depend on an LDR version itself. This makes actually only sense
// for profile B.
template<int count,int trafo,int rtrafo,bool diagonal>
void MultiplicationTrafo<count,trafo,rtrafo,diagonal>::RGB2YCbCr(const RectAngle<LONG> &r,
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
      case 3:
	target[2][x] = m_lDCShift << ColorTrafo::COLOR_BITS;
	target[1][x] = m_lDCShift << ColorTrafo::COLOR_BITS;
      case 1:
	target[0][x] = m_lDCShift << ColorTrafo::COLOR_BITS;
      }
    }
  }
  //
  // Test whether we have the L-Trafo here.
  if ((count >= 3 && m_pDecoding[2] == NULL) ||
      (count >= 2 && m_pDecoding[1] == NULL) ||
      (count >= 1 && m_pDecoding[0] == NULL)) {
    JPG_THROW(NOT_IMPLEMENTED,"MultiplicationTrafo::RGB2YCbCr",
	      "cannot encode - no LDR transformation given and no forward tone mapping provided");
  }
  
  {
    const FLOAT *rptr,*gptr,*bptr;
    switch(count) {
    case 3:
      bptr = (const FLOAT *)(source[2]->ibm_pData);
      gptr = (const FLOAT *)(source[1]->ibm_pData);
    case 1:
      rptr = (const FLOAT *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst;
      const FLOAT *r,*g,*b;
      switch(count) {
      case 3:
	crdst   = target[2] + xmin + (y << 3);
	b       = bptr;
	cbdst   = target[1] + xmin + (y << 3);
	g       = gptr;
      case 1:
	ydst    = target[0] + xmin + (y << 3);
	r       = rptr;
      }
      //
      for(x = xmin;x <= xmax;x++) {
	DOUBLE gc,bc,rc;
	DOUBLE rf = 0.0,gf =0.0,bf = 0.0;
	LONG rv,gv,bv;
	// Step one: Fetch the HDR data and apply the inverse of the
	// output transformation. The output transformation is always
	// unscaled.
	switch(count) {
	case 3:
	  gc = m_pOutputTrafo[1]->ApplyInverseCurve(*g,1,0,1,0);
	  bc = m_pOutputTrafo[2]->ApplyInverseCurve(*b,1,0,1,0);
	case 1:
	  rc = m_pOutputTrafo[0]->ApplyInverseCurve(*r,1,0,1,0);
	}
	//
	// Apply now the inverse of the secondary curve if we have one.
	// In case of profile B, this will create a lot of saturations
	// (hopefully), which are the points where data needs to go into
	// the residual.
	if (!diagonal) {
	  // These parameters are all unscaled, i.e. there are no COLOR_BITS left.
	  // They are all scaled to 8+Rb, i.e. m_lOutMax.
	  switch(count) {
	  case 3:
	    gc = m_pSecondBase[1]->ApplyInverseCurve(gc,m_lOutMax,0,m_lOutMax,0);
	    bc = m_pSecondBase[2]->ApplyInverseCurve(bc,m_lOutMax,0,m_lOutMax,0);
	  case 1:
	    rc = m_pSecondBase[0]->ApplyInverseCurve(rc,m_lOutMax,0,m_lOutMax,0);
	  }
	}
	//
	// Inverse C transformation, transform into the 601 primary color space.
	// Again, this may saturate.
	if (count == 3) {
	  rf = m_fInvC[0] * rc + m_fInvC[1] * gc + m_fInvC[2] * bc;
	  gf = m_fInvC[3] * rc + m_fInvC[4] * gc + m_fInvC[5] * bc;
	  bf = m_fInvC[6] * rc + m_fInvC[7] * gc + m_fInvC[8] * bc;
	} else {
	  rf = rc;
	}
	//
	// Apply the inverse of the L transformation. This scales from OutMax to Max.
	// No fractional bits here, yet, because this mimics conventional JPEG. Also make sure
	// to fully clamp the result (the curve should do, but some rounding is required
	// in additionl).
	switch(count) {
	case 3:
	  gf = m_pDecoding[1]->ApplyInverseCurve(gf,m_lOutMax,0,m_lMax,0);
	  gv = CLAMP(m_lMax,gf);
	  bf = m_pDecoding[2]->ApplyInverseCurve(bf,m_lOutMax,0,m_lMax,0);
	  bv = CLAMP(m_lMax,bf);
	case 1:
	  rf = m_pDecoding[0]->ApplyInverseCurve(rf,m_lOutMax,0,m_lMax,0);
	  rv = CLAMP(m_lMax,rf);
	}
	//
	// Transform to YCbCr according to the selected transformation.
	switch(count) {
	case 3:
	  switch(trafo) {
	  case MergingSpecBox::YCbCr:
	    // Offset data such that it is preshifted by COLOR_BITS. This is
	    // always the ICT.
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
	    assert(*ydst  <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*cbdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*crdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    break;
	  case MergingSpecBox::Identity:
	    *ydst  = INT_TO_COLOR(rv);
	    *cbdst = INT_TO_COLOR(gv);
	    *crdst = INT_TO_COLOR(bv);
	    assert(*ydst  <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*cbdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*crdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    break;
	  }
	  ydst++,cbdst++,crdst++;
	  r  = (const FLOAT *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
	  g  = (const FLOAT *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
	  b  = (const FLOAT *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
	  break;
	case 1: 
	  *ydst = INT_TO_COLOR(rv);
	  ydst++;
	  r  = (const FLOAT *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
	  break;
	}
      } // Of loop over X
      switch(count) {
      case 3:
	bptr  = (const FLOAT *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
	gptr  = (const FLOAT *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
      case 1:
	rptr  = (const FLOAT *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
      }
    } // Of loop over Y
  }
}
///

/// MultiplicationTrafo::LDRRGB2YCbCr
// Transform a block from RGB to YCbCr. Input are the three image bitmaps
// already clipped to the rectangle to transform, the coordinate rectangle to use
// and the level shift.
template<int count,int trafo,int rtrafo,bool diagonal>
void MultiplicationTrafo<count,trafo,rtrafo,diagonal>::LDRRGB2YCbCr(const RectAngle<LONG> &r,
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
      case 3:
	target[2][x] = m_lDCShift << ColorTrafo::COLOR_BITS;
	target[1][x] = m_lDCShift << ColorTrafo::COLOR_BITS;
      case 1:
	target[0][x] = m_lDCShift << ColorTrafo::COLOR_BITS;
	}
    }
  }
  
  {
    const UBYTE *rptr,*gptr,*bptr;
    switch(count) {
    case 3:
      bptr = (const UBYTE *)(source[2]->ibm_pData);
      gptr = (const UBYTE *)(source[1]->ibm_pData);
    case 1:
      rptr = (const UBYTE *)(source[0]->ibm_pData);
    }
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst;
      const UBYTE *r,*g,*b;
      switch(count) {
      case 3:
	crdst   = target[2] + xmin + (y << 3);
	b       = bptr;
	cbdst   = target[1] + xmin + (y << 3);
	g       = gptr;
      case 1:
	ydst    = target[0] + xmin + (y << 3);
	r       = rptr;
      }
      //
      // No tables used at all, user already supplies a tone mapped image.
      for(x = xmin;x <= xmax;x++) { 
	UBYTE rv,gv,bv;
	
	switch(count) {
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
	    assert(*ydst  <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*cbdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*crdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    break;
	  case MergingSpecBox::Identity:
	    *ydst  = INT_TO_COLOR(rv);
	    *cbdst = INT_TO_COLOR(gv);
	    *crdst = INT_TO_COLOR(bv);
	    assert(*ydst  <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*cbdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    assert(*crdst <= ((m_lMax + 1) << ColorTrafo::COLOR_BITS) - 1);
	    break;
	  }
	  ydst++,cbdst++,crdst++;
	  r += source[0]->ibm_cBytesPerPixel;
	  g += source[1]->ibm_cBytesPerPixel;
	  b += source[2]->ibm_cBytesPerPixel;
	  break;
	case 1: 
	  *ydst = INT_TO_COLOR(*r);
	  ydst++;
	  r  += source[0]->ibm_cBytesPerPixel;
	  break;
	}
      }
      switch(count) {
      case 3:
	bptr += source[2]->ibm_lBytesPerRow;
	gptr += source[1]->ibm_lBytesPerRow;
      case 1:
	rptr += source[0]->ibm_lBytesPerRow;
      }
    }
  }
}
///

/// MultiplicationTrafo::RGB2Residual
// Compute the reisual from the original image and the decoded LDR image, place the
// result in the output buffer. This is part of the encoder.
template<int count,int trafo,int rtrafo,bool diagonal>
void MultiplicationTrafo<count,trafo,rtrafo,diagonal>::RGB2Residual(const RectAngle<LONG> &r,
								    const struct ImageBitMap *const *source,
								    Buffer reconstructed,Buffer residual)
{
  LONG x,y;
  LONG xmin = r.ra_MinX & 7;
  LONG ymin = r.ra_MinY & 7;
  LONG xmax = r.ra_MaxX & 7;
  LONG ymax = r.ra_MaxY & 7;
  
  if (xmax < 7 || ymax < 7 || xmin > 0 || ymin > 0) {
    for(x = 0;x < 64;x++) {
      // The R-transformation is either the YCbCr to RGB transformation
      // or the free-form transformation. In both cases, the result
      // is expected to be preshifted by COLOR_BITS.
      switch(count) {
      case 3:
	residual[2][x] = m_lRDCShift << ColorTrafo::COLOR_BITS;
	residual[1][x] = m_lRDCShift << ColorTrafo::COLOR_BITS;
      case 1:
	residual[0][x] = m_lRDCShift << ColorTrafo::COLOR_BITS;
      }
      break;
    }
  }
  
  {
    const FLOAT *rptr,*gptr,*bptr;
    switch(count) {
    case 3:
      gptr = (const FLOAT *)(source[1]->ibm_pData);
      bptr = (const FLOAT *)(source[2]->ibm_pData);
    case 1:
      rptr = (const FLOAT *)(source[0]->ibm_pData);
    }
    
    for(y = ymin;y <= ymax;y++) {
      LONG *ydst,*cbdst,*crdst;
      LONG *yrec,*cbrec,*crrec;
      const FLOAT *r,*g,*b;
      //
      switch(count) {
      case 3:
        crdst   = residual[2]      + xmin + (y << 3);
        crrec   = reconstructed[2] + xmin + (y << 3);
        b       = bptr;
        cbdst   = residual[1]      + xmin + (y << 3);
        cbrec   = reconstructed[1] + xmin + (y << 3);
        g       = gptr;
      case 1:
        ydst    = residual[0]      + xmin + (y << 3);
        yrec    = reconstructed[0] + xmin + (y << 3);
        r       = rptr;
      }
      //
      for(x = xmin;x <= xmax;x++) {
	LONG  rv,gv,bv,ry;
        DOUBLE rf,gf,bf;
	DOUBLE rc,gc,bc;
	DOUBLE rp,gp,bp;
	LONG   y,cb,cr;
	DOUBLE mu = 1.0;
	DOUBLE nu = 1.0;
	//
        // First reconstruct the legacy with the L-Transformation and the L-Lut.
        // The reconstructed data goes into av,rv,gv,bv.
        switch(count) {
        case 3:
          switch(trafo) {
          case MergingSpecBox::YCbCr:
            // Data arrives preshifted by COLOR_BITS here.
	    // Note that this is preshifted by our convention (not by the standard).
            cr = *crrec - (m_lDCShift << ColorTrafo::COLOR_BITS);
            cb = *cbrec - (m_lDCShift << ColorTrafo::COLOR_BITS);
            rv = FIX_COLOR_TO_INT((*yrec << FIX_BITS) + cr *  TO_FIX(1.40200));
            gv = FIX_COLOR_TO_INT((*yrec << FIX_BITS) + cr * -TO_FIX(0.7141362859) + cb * -TO_FIX(0.3441362861));
            bv = FIX_COLOR_TO_INT((*yrec << FIX_BITS) + cb *  TO_FIX(1.772));
            break;
          case MergingSpecBox::Identity:
	    rv = COLOR_TO_INT(*yrec);
	    gv = COLOR_TO_INT(*cbrec);
	    bv = COLOR_TO_INT(*crrec);
	    break;
          default:
            assert(!"Unsupported L transformation type");
          }
	  //
	  // Apply the L-Lut. This is either a parametric curve or a LUT. It is here
	  // always expected to be a LUT.
	  rf = APPLY_LUT(m_pfDecodingLUT[0],m_lMax,rv);
	  gf = APPLY_LUT(m_pfDecodingLUT[1],m_lMax,gv);
	  bf = APPLY_LUT(m_pfDecodingLUT[2],m_lMax,bv);
	  //
	  // Followed by the C-transformation. (This came in new).
	  rc = rf * m_fC[0] + gf * m_fC[1] + bf * m_fC[2];
	  gc = rf * m_fC[3] + gf * m_fC[4] + bf * m_fC[5];
	  bc = rf * m_fC[6] + gf * m_fC[7] + bf * m_fC[8];
	  // 
	  // Legacy decoding done for three components.
	  break;
	case 1:
	  rv = COLOR_TO_INT(*yrec);
	  rc = APPLY_LUT(m_pfDecodingLUT[0],m_lMax,rv);
	  // Legacy decoding done for three components.
	  break;
	}
	//
	// For profile B, apply the nonlinearities now before adding. Actually, I could
	// use a gamma/division, but I want to try whether all this works, hence....
	if (!diagonal) {
	  // These parameters are all unscaled, i.e. there are no COLOR_BITS left.
	  // They are all scaled to 8+Rb, i.e. m_lOutMax.
	  switch(count) {
	  case 3:
	    gc = m_pSecondBase[1]->ApplyCurve(gc,m_lOutMax,0,m_lOutMax,0);
	    bc = m_pSecondBase[2]->ApplyCurve(bc,m_lOutMax,0,m_lOutMax,0);
	  case 1:
	    rc = m_pSecondBase[0]->ApplyCurve(rc,m_lOutMax,0,m_lOutMax,0);
	  }
	}
	//
	// At this point, the legacy data is reconstructed fine. 
	//
	// Now go for the original.
	// Run the inverse of the output transformation on the input data to
	// get the pre-processed data. This is always unscaled.
	switch(count) {
	case 3:
	  gp = m_pOutputTrafo[1]->ApplyInverseCurve(*g,1,0,1,0);
	  bp = m_pOutputTrafo[2]->ApplyInverseCurve(*b,1,0,1,0);
	case 1:
	  rp = m_pOutputTrafo[0]->ApplyInverseCurve(*r,1,0,1,0);
	}
	//
	// The output of the L/C transformation is now in rc,gc,bc or rc.
	// If we have a diagonal transformation, apply it now.
	if (diagonal) {
	  DOUBLE p = 0.0,q = 1.0;
	  //
	  switch(count) {
	  case 3:
	    // The p-value comes from the original image, the q value from the
	    // reconstructed value. Use the prescaling linear trafo to compute p and q.
	    p = (rp * m_fP[0] + gp * m_fP[1] + bp * m_fP[2]);
	    q = (rc * m_fP[0] + gc * m_fP[1] + bc * m_fP[2]); 
	    // The luma value is now in q. It is not scaled. Apply the output LUT.
	    // This must be parametric and linear.
	    // This adds the noise floor.
	    // The parameters are the input scale, input scale bits, output scale and output scale bits.
	    nu  = m_pPrescalingLUT->ApplyCurve(q,m_lOutMax,0,1,0);
	    break;
	  case 1:
	    // Use the one and only component unaltered.
	    // There is no nu in case we don't have chroma.
	    p = rp;
	    q = rc;
	    break;
	  }
	  //
	  // Compute the luminance of the residual signal from the quotient p/q. The maximum is
	  // m_lRMax. Also preshift by COLOR_BITS.
	  // There is no prescaling here, rtrafo must be YCbCr or freeform in this case.
	  // Input parameters are p/q, R_w, R_e
	  ry = m_pDiagonalLUT->InverseOfQuotient(p,q,m_lRMax,COLOR_BITS);
	  // From this the decoder computes the scale value mu. Again, residual, thus no
	  // preshift, and identity transformation.
	  mu = APPLY_LUT(m_pfDecodingDiagonalLUT,((m_lRMax + 1) << COLOR_BITS) - 1,ry);
	}
	//
	//
	if (diagonal) {
	  if (count == 3) {
	    DOUBLE dr,dg,db;
	    DOUBLE dcb,dcr;
	    // Compute from that the Cb and Cr components, up to scaling.
	    dr  = (rp / mu - rf) / nu;
	    dg  = (gp / mu - gf) / nu;
	    db  = (bp / mu - bf) / nu;
#if CHECK_LEVEL > 0
	    DOUBLE dy;
	    // Forwards transform with R. Dy should hopefully be close to zero, but may not.
	    // I want to look at it either way for debugging.
	    dy  = m_fRInv[0] * dr + m_fRInv[1] * dg + m_fRInv[2] * db;
	    NOREF(dy);
#endif	    
	    dcb = m_fRInv[3] * dr + m_fRInv[4] * dg + m_fRInv[5] * db + m_lOutDCShift;
	    dcr = m_fRInv[6] * dr + m_fRInv[7] * dg + m_fRInv[8] * db + m_lOutDCShift;
	    // Apply the inverse of the R-lut, which is also a linear scaling that extends the
	    // range to maximum. This is a scaling transformation with m_lRMax as scale factor
	    // and COLOR_BITS as fractional bits (by convention).
	    // Parameters are input scale, fractional bits, output scale, output fractional bits.
	    cb  = m_pResidualLUT[1]->ApplyInverseCurve(dcb,m_lOutMax,0,m_lRMax,COLOR_BITS);
	    cr  = m_pResidualLUT[2]->ApplyInverseCurve(dcr,m_lOutMax,0,m_lRMax,COLOR_BITS);
	    y   = ry; // This is already ready.
	  } else {
	    // There is no other component but ry derived from mu.
	    y   = ry;
	  }
	} else {
	  DOUBLE dr,dg,db;
	  DOUBLE d2r = 0.0,d2g = 0.0,d2b = 0.0;
	  // Here it is the differential in the logarithmic space.
	  // Also apply the second residual inverse to transform back into the linear space.
	  // Then apply the intermediate NLT which goes to the residual space and scale accordingly.
	  // There is potentially an additional color transformation here which I do not yet apply.
	  //
	  // One of the tricks here to avoid unnecessary noise is to set the residual to the
	  // maximum value (hence, 1.0) in case the legacy is not overflowing. This helps to
	  // avoid noise in the residual channel when the legacy channel is not saturated.
	  // Where exactly this happens is the matter of the exposure value.
	  // The condition for that is rc,gc,bc >= 0.0 because log(1) = 0.
	  // Unfortunately, this is a bad idea on closed-loop coding because then the output near
	  // contrast edges might not be exactly the maximal value, hence rc,gc,bc may become
	  // smaller than zero even though we would profit from a correction from the residual.
	  // Instead, it is then forcably set to +1, causing artifacts near the edges.
	  //
	  // The alternative trick is here to check whether rv,gv,bv > 0. If it is 0, then reconstructed
	  // logarithm in the base domain is -inf, hence the reconstruction is +0. If the original pixel value
	  // is not exactly zero (which can very well happen), then the residual tries to compensate for that
	  // and hence creates an output value of +inf. On reconstruction, this creates a massive problem
	  // because +inf - inf = nan, so we get nothing useful at all.
	  //
	  // The trick is now to avoid setting the residual to +1 in case the reconstructed original
	  // is too small, i.e. simply 0. This happens if the input value of rv,gv,bv is zero, 
	  // leading to a log of -inf.
	  switch(count) {
	  case 3:
	    if (gv > 0) {
	      dg = m_pSecondResidual[1]->ApplyInverseCurve(gp - gc + m_lOutDCShift,m_lOutMax,0,m_lOutMax,0);
	    } else {
	      dg = m_lOutMax;
	    }
	    if (bv > 0) {
	      db = m_pSecondResidual[2]->ApplyInverseCurve(bp - bc + m_lOutDCShift,m_lOutMax,0,m_lOutMax,0);
	    } else {
	      db = m_lOutMax;
	    }
	  case 1:
	    if (rv > 0) {
	      dr = m_pSecondResidual[0]->ApplyInverseCurve(rp - rc + m_lOutDCShift,m_lOutMax,0,m_lOutMax,0);
	    } else {
	      dr = m_lOutMax;
	    }
	  }
	  //
	  // Apply the D-transformation. By construction this is identical to the C-transformation.
	  if (count == 3) {
	    d2r = m_fInvC[0] * dr + m_fInvC[1] * dg + m_fInvC[2] * db;
	    d2g = m_fInvC[3] * dr + m_fInvC[4] * dg + m_fInvC[5] * db;	    
	    d2b = m_fInvC[6] * dr + m_fInvC[7] * dg + m_fInvC[8] * db;
	  } else {
	    d2r = dr;
	  }
	  //
	  switch(count) {
	  case 3:
	    dg = m_pIntermediateResidual[1]->ApplyInverseCurve(d2g,m_lOutMax,0,m_lOutMax,0);
	    db = m_pIntermediateResidual[2]->ApplyInverseCurve(d2b,m_lOutMax,0,m_lOutMax,0);
	  case 1:
	    dr = m_pIntermediateResidual[0]->ApplyInverseCurve(d2r,m_lOutMax,0,m_lOutMax,0);
	  }
	  //
	  // Now transform to YCbCr.
	  {
	    DOUBLE dy,dcb,dcr;
	    switch(count) {
	    case 3:
	      dy  = m_fRInv[0] * dr + m_fRInv[1] * dg + m_fRInv[2] * db;
	      dcb = m_fRInv[3] * dr + m_fRInv[4] * dg + m_fRInv[5] * db + m_lOutDCShift;
	      dcr = m_fRInv[6] * dr + m_fRInv[7] * dg + m_fRInv[8] * db + m_lOutDCShift;
	      break;
	    case 1:
	      dy  = dr;
	      break;
	    }
	    //
	    // Run the residualLUT to downscale the data to the final range.
	    // These are preshifted by COLOR_BITs by convention and must be adjusted to range.
	    switch(count) {
	    case 3:
	      cb  = m_pResidualLUT[1]->ApplyInverseCurve(dcb,m_lOutMax,0,m_lRMax,COLOR_BITS);
	      cr  = m_pResidualLUT[2]->ApplyInverseCurve(dcr,m_lOutMax,0,m_lRMax,COLOR_BITS);
	    case 1:
	      y   = m_pResidualLUT[0]->ApplyInverseCurve(dy,m_lOutMax,0,m_lRMax,COLOR_BITS);
	    }
	  }
	}
	//
	// Insert results and advance source pointers.
	switch(count) {
	case 3:
	  *cbdst++ = cb;
	  *crdst++ = cr;
	  cbrec++;
	  crrec++;
	  g  = (const FLOAT *)((const UBYTE *)(g) + source[1]->ibm_cBytesPerPixel);
	  b  = (const FLOAT *)((const UBYTE *)(b) + source[2]->ibm_cBytesPerPixel);
	case 1:
	  *ydst++  = y;
	  yrec++;
	  r  = (const FLOAT *)((const UBYTE *)(r) + source[0]->ibm_cBytesPerPixel);
	}
      } // of loop over x. 
      //
      // Advance row pointers.
      switch(count) {
      case 3:
        bptr  = (const FLOAT *)((const UBYTE *)(bptr) + source[2]->ibm_lBytesPerRow);
        gptr  = (const FLOAT *)((const UBYTE *)(gptr) + source[1]->ibm_lBytesPerRow);
      case 1:
        rptr  = (const FLOAT *)((const UBYTE *)(rptr) + source[0]->ibm_lBytesPerRow);
      }
    } // of loop over y
  }
}
///

/// MultiplicationTrafo::YCbCr2RGB
// Compute the residual from the original image and the decoded LDR image, place result in
// the output buffer. This depends rather on the coding model.
// This is part of the decoder.
template<int count,int trafo,int rtrafo,bool diagonal>
void MultiplicationTrafo<count,trafo,rtrafo,diagonal>::YCbCr2RGB(const RectAngle<LONG> &r,
								 const struct ImageBitMap *const *dest,
								 Buffer reconstructed,Buffer residual)
{
  LONG x,y;
  LONG xmin = r.ra_MinX & 7;
  LONG ymin = r.ra_MinY & 7;
  LONG xmax = r.ra_MaxX & 7;
  LONG ymax = r.ra_MaxY & 7; 
  //
  // This should only be called with residual data present. Otherwise,
  // the simpler YCbCr transformation class is sufficient.
  assert(residual);
  assert(dest);

  for(x = 0;x < count;x++) {
    if (dest[0]->ibm_ucPixelType != dest[x]->ibm_ucPixelType) {
      JPG_THROW(INVALID_PARAMETER,"YCbCrTrafo::YCbCr2RGB",
		"pixel types of all three components in a YCbCr to RGB conversion must be identical");
    }
  }
  {
    FLOAT *rptr,*gptr,*bptr;
    //
    // Get the pointer to the external data.
    switch(count) {
    case 3:
      bptr = (FLOAT *)(dest[2]->ibm_pData);
      gptr = (FLOAT *)(dest[1]->ibm_pData);
    case 1:
      rptr = (FLOAT *)(dest[0]->ibm_pData);
    }
    //
    // Loop over y, load the source pointers.
    for(y = ymin;y <= ymax;y++) {
      LONG *ysrc,*cbsrc,*crsrc;
      LONG *rysrc,*rcbsrc,*rcrsrc;
      FLOAT *r,*g,*b;
      switch(count) {
      case 3:
        crsrc    = reconstructed[2] + xmin + (y << 3);
        cbsrc    = reconstructed[1] + xmin + (y << 3);
        b        = bptr;
        g        = gptr;
        rcrsrc   = residual[2]      + xmin + (y << 3);
        rcbsrc   = residual[1]      + xmin + (y << 3);
      case 1:
        ysrc     = reconstructed[0] + xmin + (y << 3);
        r        = rptr;
        rysrc    = residual[0]      + xmin + (y << 3);
      }
      //
      // Loop over X.
      for(x = xmin;x <= xmax;x++) {
	DOUBLE mu = 1.0;
	DOUBLE nu = 1.0;
	DOUBLE dry,drcb,drcr;
	LONG   cb,cr,rv,gv,bv;
        DOUBLE rf,gf,bf;
	DOUBLE rc,gc,bc;
	DOUBLE rx,gx,bx;
	DOUBLE rr,rg,rb;
	DOUBLE r2r,r2g,r2b;
	//
	// First reconstruct the legacy with the L-Transformation and the L-Lut.
        // The reconstructed data goes into rc,gc,bc.
        switch(count) {
        case 3:
          switch(trafo) {
          case MergingSpecBox::YCbCr:
            // Data arrives preshifted by COLOR_BITS here.
	    // Note that this is preshifted by our convention (not by the standard).
            cr = *crsrc - (m_lDCShift << ColorTrafo::COLOR_BITS);
            cb = *cbsrc - (m_lDCShift << ColorTrafo::COLOR_BITS);
            rv = FIX_COLOR_TO_INT((*ysrc << FIX_BITS) + cr *  TO_FIX(1.40200));
            gv = FIX_COLOR_TO_INT((*ysrc << FIX_BITS) + cr * -TO_FIX(0.7141362859) + cb * -TO_FIX(0.3441362861));
            bv = FIX_COLOR_TO_INT((*ysrc << FIX_BITS) + cb *  TO_FIX(1.772));
            break;
          case MergingSpecBox::Identity:
	    rv = COLOR_TO_INT(*ysrc);
	    gv = COLOR_TO_INT(*cbsrc);
	    bv = COLOR_TO_INT(*crsrc);
	    break;
          default:
            assert(!"Unsupported L transformation type");
          }
	  //
	  // Apply the L-Lut. This is either a parametric curve or a LUT. It is here
	  // always expected to be a LUT.
	  rf = APPLY_LUT(m_pfDecodingLUT[0],m_lMax,rv);
	  gf = APPLY_LUT(m_pfDecodingLUT[1],m_lMax,gv);
	  bf = APPLY_LUT(m_pfDecodingLUT[2],m_lMax,bv);
	  assert(!isnan(rf));
	  assert(!isnan(gf));
	  assert(!isnan(bf));
	  //
	  // Followed by the C-transformation. (This came in new).
	  rc = rf * m_fC[0] + gf * m_fC[1] + bf * m_fC[2];
	  gc = rf * m_fC[3] + gf * m_fC[4] + bf * m_fC[5];
	  bc = rf * m_fC[6] + gf * m_fC[7] + bf * m_fC[8];
	  // 
	  // Legacy decoding done for three components.
	  break;
	case 1:
	  rv = COLOR_TO_INT(*ysrc);
	  rc = APPLY_LUT(m_pfDecodingLUT[0],m_lMax,rv);
	  assert(!isnan(rc));
	  // Legacy decoding done for three components.
	  break;
	}
	//
	// For profile B, apply the nonlinearities now before adding. Actually, I could
	// use a gamma/division, but I want to try whether all this works, hence....
	if (!diagonal) {
	  // These parameters are all unscaled, i.e. there are no COLOR_BITS left.
	  // They are all scaled to 8+Rb, i.e. m_lOutMax.
	  switch(count) {
	  case 3:
	    gc = m_pSecondBase[1]->ApplyCurve(gc,m_lOutMax,0,m_lOutMax,0);
	    bc = m_pSecondBase[2]->ApplyCurve(bc,m_lOutMax,0,m_lOutMax,0);
	    assert(!isnan(gc));
	    assert(!isnan(bc));
	  case 1:
	    rc = m_pSecondBase[0]->ApplyCurve(rc,m_lOutMax,0,m_lOutMax,0);
	    assert(!isnan(rc));
	  }
	}
	//
	// At this point, the legacy data has been reconstructed. Now advance to the residual.
	//
	// Apply the Q-Luts to perform the scaling. These are always parametric.
	// It sounds strange that Y is also used here even though this is a constant. It
	// mainly sitting here to test the proper choice of the parameters.
	// This already scales to full range.
	switch(count) {
	case 3:
	  drcb = m_pResidualLUT[1]->ApplyCurve(*rcbsrc,m_lRMax,COLOR_BITS,m_lOutMax,0);
	  drcr = m_pResidualLUT[2]->ApplyCurve(*rcrsrc,m_lRMax,COLOR_BITS,m_lOutMax,0);
	case 1:
	  dry  = m_pResidualLUT[0]->ApplyCurve(*rysrc ,m_lRMax,COLOR_BITS,m_lOutMax,0);
	}
	//
	if (diagonal) {
	  // First get the scaling parameter from the first component of the residual.
	  // This happens before the RTrafo, and the rtrafo is always a scaled one (ICT or freeform)
	  mu = APPLY_LUT(m_pfDecodingDiagonalLUT,((m_lRMax + 1) << COLOR_BITS) - 1,*rysrc);
	}
	//
	// The output of the L/C transformation is now in rc,gc,bc or rc.
	// Compute now the scale factor nu if we have to.
	switch(count) {
	case 3:
	  if (diagonal) {
	    DOUBLE q;
	    //
	    // Compute the linear prescaling transformation.
	    // The p-value comes from the original image, the q value from the
	    // reconstructed value. Use the prescaling linear trafo to compute p and q.
	    q    = (rc * m_fP[0] + gc * m_fP[1] + bc * m_fP[2]);
	    //
	    // This is now the entry into the prescaling NLT.
	    nu   = m_pPrescalingLUT->ApplyCurve(q,m_lOutMax,0,1,0);
	  }
	  //
	  // Apply now the R-transformation. This can be either free-form or ICT.
	  // Preshift is already removed.
	  // Perform the DC removal, no scaling required here.
	  drcb = nu * (drcb - m_lOutDCShift);
	  drcr = nu * (drcr - m_lOutDCShift);
	  rr   = m_fR[0] * dry + m_fR[1] * drcb + m_fR[2] * drcr;
	  rg   = m_fR[3] * dry + m_fR[4] * drcb + m_fR[5] * drcr;
	  rb   = m_fR[6] * dry + m_fR[7] * drcb + m_fR[8] * drcr;
	  //
	  break;
	case 1:
	  // There is only one component, ry. 
	  rr = dry;
	  break;
	}
	//
	// For profile B, apply the nonlinearities now before adding. Actually, I could
	// use a gamma/division, but I want to try whether all this works, hence....
	if (!diagonal) {
	  // These parameters are all unscaled, i.e. there are no COLOR_BITS left.
	  // Apply the intermediate and the second residual LUT.
	  // They are all scaled to 8+Rb, i.e. m_lOutMax.
	  switch(count) {
	  case 3:
	    r2g = m_pIntermediateResidual[1]->ApplyCurve(rg,m_lOutMax,0,m_lOutMax,0);
	    r2b = m_pIntermediateResidual[2]->ApplyCurve(rb,m_lOutMax,0,m_lOutMax,0);
	  case 1:
	    r2r = m_pIntermediateResidual[0]->ApplyCurve(rr,m_lOutMax,0,m_lOutMax,0);
	  }
	  //
	  // Apply the D transformation which is identical to the C transformation.
	  if (count == 3) {
	    rr = m_fC[0] * r2r + m_fC[1] * r2g + m_fC[2] * r2b;
	    rg = m_fC[3] * r2r + m_fC[4] * r2g + m_fC[5] * r2b;
	    rb = m_fC[6] * r2r + m_fC[7] * r2g + m_fC[8] * r2b;
	  } else {
	    rr = r2r;
	  }
	  //
	  switch(count) {
	  case 3:
	    rg = m_pSecondResidual[1]->ApplyCurve(r2g,m_lOutMax,0,m_lOutMax,0);
	    rb = m_pSecondResidual[2]->ApplyCurve(r2b,m_lOutMax,0,m_lOutMax,0);
	  case 1:
	    rr = m_pSecondResidual[0]->ApplyCurve(r2r,m_lOutMax,0,m_lOutMax,0);
	  }
	}
	//
	// Now add the residual to the legacy data and scale by mu.
	// This has to include the shift by the DC value.
	switch(count) {
	case 3:
	  gx = mu * (gc + rg - m_lOutDCShift);
	  bx = mu * (bc + rb - m_lOutDCShift);
	case 1:
	  rx = mu * (rc + rr - m_lOutDCShift);
	}
	//
	// Finally, perform output mapping. This is always unscaled as there
	// is no natural scale anymore.
	switch(count) {
	case 3:
	  assert(!isnan(gx));
	  assert(!isnan(bx));
	  gc = m_pOutputTrafo[1]->ApplyCurve(gx,1,0,1,0);
	  bc = m_pOutputTrafo[2]->ApplyCurve(bx,1,0,1,0);
	  assert(!isnan(gc));
	  assert(!isnan(bc));
	case 1:
	  assert(!isnan(rx));
	  rc = m_pOutputTrafo[0]->ApplyCurve(rx,1,0,1,0);
	  assert(!isnan(rc));
	}
	//
	// Save the output, advance pointers.
	switch(count) {
	case 3:
	  *g = gc;
	  *b = bc;
	  g  = (FLOAT *)((UBYTE *)(g) + dest[1]->ibm_cBytesPerPixel);
	  b  = (FLOAT *)((UBYTE *)(b) + dest[2]->ibm_cBytesPerPixel);
	  cbsrc++;
	  crsrc++;
	  rcbsrc++;
	  rcrsrc++;
	case 1:
	  *r = rc;
	  r  = (FLOAT *)((UBYTE *)(r) + dest[0]->ibm_cBytesPerPixel);
	  ysrc++;
	  rysrc++;
	}
      } // Of loop over x
      switch(count) {
      case 3:
        bptr  = (FLOAT *)((UBYTE *)(bptr) + dest[2]->ibm_lBytesPerRow);
        gptr  = (FLOAT *)((UBYTE *)(gptr) + dest[1]->ibm_lBytesPerRow);
      case 1:
        rptr  = (FLOAT *)((UBYTE *)(rptr) + dest[0]->ibm_lBytesPerRow);
      }
    } // Of loop over y
  }
}
///

/// Explicit instanciations
template class MultiplicationTrafo<1,MergingSpecBox::Identity,MergingSpecBox::Identity,false>;
template class MultiplicationTrafo<3,MergingSpecBox::Identity,MergingSpecBox::Identity,false>;
template class MultiplicationTrafo<3,MergingSpecBox::YCbCr,MergingSpecBox::Identity,false>;
template class MultiplicationTrafo<1,MergingSpecBox::Identity,MergingSpecBox::Identity,true>;
template class MultiplicationTrafo<3,MergingSpecBox::Identity,MergingSpecBox::Identity,true>;
template class MultiplicationTrafo<3,MergingSpecBox::YCbCr,MergingSpecBox::Identity,true>;
template class MultiplicationTrafo<3,MergingSpecBox::Identity,MergingSpecBox::YCbCr,false>;
template class MultiplicationTrafo<3,MergingSpecBox::YCbCr,MergingSpecBox::YCbCr,false>;
template class MultiplicationTrafo<3,MergingSpecBox::Identity,MergingSpecBox::YCbCr,true>;
template class MultiplicationTrafo<3,MergingSpecBox::YCbCr,MergingSpecBox::YCbCr,true>;
///

///
#endif
