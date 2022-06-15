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
**
** Inverse DCT operation plus scaled quantization.
** This DCT is based entirely on lifting and is hence always invertible.
** It is taken from the article "Integer DCT-II by Lifting Steps", by
** G. Plonka and M. Tasche, with a couple of corrections and adaptions.
** The A_4(1) matrix in the article rotates the wrong elements.
** Furthermore, the DCT in this article is range-extending by a factor of
** two in each dimension, which we cannot effort. Instead, the butterflies
** remain unscaled and are replaced by lifting rotations.
** Proper rounding is required as we cannot use fractional bits.
** This implementation requires approximately 191 shifts per row and column
** or 39 multiplications per row and column and 230 adds per shift and column.
**
** $Id: liftingdct.cpp,v 1.18 2016/10/28 13:58:54 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "std/string.hpp"
#include "dct/liftingdct.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
#include "marker/quantizationtable.hpp"
///

/// Multiplications by constants
#define FRACT_BITS 12
#define ROUND(x) (((x) + ((1 << FRACT_BITS) >> 1)) >> FRACT_BITS)
                                                            //                    109876543210
                                                            // Multiply by 403  = 000110010011
#define pmul_tan1(x) (t = (x) + ((x) << 1),t = t + ((x) << 4) + (t << 7),ROUND(t))

                                                            // Multiply by 1243 = 010011011011
#define pmul_tan3(x) (t = (x) + ((x) << 1),t = t + (t << 3) + (t << 6) + ((x) << 10),ROUND(t))

                                                            // Multiply by 1697 = 011010100001
#define pmul_tan4(x) (t = (x) + ((x) << 5) + ((x) << 7) + ((x) << 9) + ((x) << 10),ROUND(t))

                                                            // Multiply by 815  = 001100101111
#define pmul_tan2(x) (t = ((x) << 6) - ((x) << 4) - (x) + ((x) << 8) + ((x) << 9),ROUND(t))

                                                            // Multiply by 799  = 001100011111
#define pmul_sin1(x) (t = ((x) << 5) - (x) + ((x) << 8) + ((x) << 9),ROUND(t))

                                                            // Multiply by 2276 = 100011100100
#define pmul_sin3(x) (t = ((x) << 8) - ((x) << 5) + ((x) << 2) + ((x) << 11),ROUND(t))

                                                            // Multiply by 1567 = 011000011111
#define pmul_sin2(x) (t = ((x) << 5) - (x) + ((x) << 9) + ((x) << 10),ROUND(t))

                                                            // Multiply by 2896 = 101101010000
#define pmul_sin4(x) (t = (x) + ((x) << 2),t = ((x) << 4) + (t << 6) + (t << 9),ROUND(t))
///

/// LiftingDCT::LiftingDCT
template<int preshift,typename T,bool deadzone,bool optimize>
LiftingDCT<preshift,T,deadzone,optimize>::LiftingDCT(class Environ *env)
  : DCT(env)
{
}
///

/// LiftingtDCT::~LiftingDCT
template<int preshift,typename T,bool deadzone,bool optimize>
LiftingDCT<preshift,T,deadzone,optimize>::~LiftingDCT(void)
{
}
///

/// LiftingDCT::DefineQuant
template<int preshift,typename T,bool deadzone,bool optimize>
void LiftingDCT<preshift,T,deadzone,optimize>::DefineQuant(class QuantizationTable *table)
{
  const UWORD *delta      = table->DeltasOf();
  int i;

  // No scaling required here.
  for(i = 0;i < 64;i++) {
    m_plQuant[i]    = delta[i];
    m_plInvQuant[i] = LONG((FLOAT(1L << QUANTIZER_BITS)) / delta[i] + 0.5);
  }
}
///

/// LiftingDCT::TransformBlock
// Run the DCT on a 8x8 block on the input data, giving the output table.
template<int preshift,typename T,bool deadzone,bool optimize>
void LiftingDCT<preshift,T,deadzone,optimize>::TransformBlock(const LONG *source,LONG *target,
                                                              LONG dcoffset)
{ 
  LONG *dpend,*dp;
  int band = 0;
  const LONG *qp = m_plInvQuant; 
  T t;
  //
  // Adjust the DC offset to the number of fractional bits.
  // The lifting DCT here has a scaling factor of 2 per dimension, and prescales data by FIX_BITS
  dcoffset  <<= 3; // was 8
  // three additional bits because we still need to divide by 8.
  //
  // Pass over columns
  dpend = target + 8;
  for(dp = target;dp < dpend;dp++,source++) {
    // Compute sqrt(2) T_8(0). This is the forward butterfly.
    T x0    = source[0 << 3] >> preshift;
    T x4    = source[7 << 3] >> preshift;
    x0     += pmul_tan4(x4);
    x4     -= pmul_sin4(x0);
    x0     += pmul_tan4(x4);
    x4      = -x4;
    T x1    = source[1 << 3] >> preshift;
    T x5    = source[6 << 3] >> preshift;
    x1     += pmul_tan4(x5);
    x5     -= pmul_sin4(x1);
    x1     += pmul_tan4(x5);
    x5      = -x5;
    T x2    = source[2 << 3] >> preshift;
    T x6    = source[5 << 3] >> preshift;
    x2     += pmul_tan4(x6);
    x6     -= pmul_sin4(x2);
    x2     += pmul_tan4(x6);
    x6      = -x6;
    T x3    = source[3 << 3] >> preshift;
    T x7    = source[4 << 3] >> preshift;
    x3     += pmul_tan4(x7);
    x7     -= pmul_sin4(x3);
    x3     += pmul_tan4(x7);
    x7      = -x7;
    // Compute the bold-Z vector from x0...x3 by T_4(0)
    T zb0    = x0 + pmul_tan4(x3);
    T zb2    = x3 - pmul_sin4(zb0);
    zb0     += pmul_tan4(zb2);
    zb2      = -zb2;
    T zb1    = x1 + pmul_tan4(x2);
    T zb3    = x2 - pmul_sin4(zb1);
    zb1     += pmul_tan4(zb3);
    zb3      = -zb3;
    // Compute z0,z1,z2 from the w-vector. This applies T_4(1) by two rotations, each of which consist of three shears.
    T z00    = pmul_tan1(x7)   + x4;
    T z01    = pmul_tan3(x6)   + x5;
    T z10    = -pmul_sin1(z00) + x7;
    T z11    = -pmul_sin3(z01) + x6;
    T z20    = pmul_tan1(z10)  + z00;
    T z21    = pmul_tan3(z11)  + z01;
    // Apply now T_8(0,1,0,0).
    // The output vector is now: zb0..zb3,z20,z21,z11,-z10.
    // This is called x^(2) in the bommer paper.
    // Compute again the bold Z vector as C_II oplus C_II. This is the lower half of the T_8(0,1,0,0) matrix.
    T zc0    = z20 + pmul_tan4(z21);
    T zc1    = z21 - pmul_sin4(zc0);
    zc0     += pmul_tan4(zc1);
    zc1      = -zc1;
    T zc3    = z11 + pmul_tan4(z10);
    T zc2    = z10 - pmul_sin4(zc3);
    zc3     += pmul_tan4(zc2);
    zc2      = -zc2;
    // Compute z0,z1,z2 from the w-vector. This is the upper half of the T_8(0,1,0,0) matrix and rotates by Pi/4 and Pi/8.
    z00 = pmul_tan4(zb1)  + zb0; // rotate zc0,zc1 by Pi/4
    z01 = pmul_tan2(zb3)  + zb2; // rotate zc2,zc3 by Pi/8
    z10 = -pmul_sin4(z00) + zb1;
    z11 = -pmul_sin2(z01) + zb3;
    z20 = pmul_tan4(z10)  + z00;
    z21 = pmul_tan2(z11)  + z01;
    // Compute the x3 vector: z20,-z10,z21,-z11,zc0,zc1,zc2,zc3
    // Not needed here, instead go ahead directly.
    // Instead, compute x4 directly. The first four comonets
    // are identical to x3, x47 = x63
    // This is the I_4 part of the last matrix.
    // Upper part of A_4(1) is identity. The matrix notation in the paper has here zc1,zc2 interchanged.
    T z0  = pmul_tan4(zc3) + zc1;
    T z1  = -pmul_sin4(z0) + zc3;
    T x45 = pmul_tan4(z1)  + z0; // rot by Pi/4
    // Output permutation by the B_8 matrix.
    dp[0 << 3] = z20;
    dp[1 << 3] = zc0;
    dp[2 << 3] = z21;
    dp[3 << 3] = -z1;
    dp[4 << 3] = -z10;
    dp[5 << 3] = x45;
    dp[6 << 3] = -z11;
    dp[7 << 3] = zc2;
  }
  //
  // Pass over rows and quantize, remove the dc shift.
  dpend = target + (8 << 3);
  for(dp = target;dp < dpend;dp += 8,qp += 8) {
    // Step 1: Compute sqrt(2) T_8(0). This is the forward butterfly.
    T x0    = dp[0];
    T x4    = dp[7];
    x0     += pmul_tan4(x4);
    x4     -= pmul_sin4(x0);
    x0     += pmul_tan4(x4);
    x4      = -x4;
    T x1    = dp[1];
    T x5    = dp[6];
    x1     += pmul_tan4(x5);
    x5     -= pmul_sin4(x1);
    x1     += pmul_tan4(x5);
    x5      = -x5;
    T x2    = dp[2];
    T x6    = dp[5];
    x2     += pmul_tan4(x6);
    x6     -= pmul_sin4(x2);
    x2     += pmul_tan4(x6);
    x6      = -x6;
    T x3    = dp[3];
    T x7    = dp[4];
    x3     += pmul_tan4(x7);
    x7     -= pmul_sin4(x3);
    x3     += pmul_tan4(x7);
    x7      = -x7;
    // Compute the bold-Z vector from x0...x3 by T_4(0) 
    T zb0    = x0 + pmul_tan4(x3);
    T zb2    = x3 - pmul_sin4(zb0);
    zb0     += pmul_tan4(zb2);
    zb2      = -zb2;
    T zb1    = x1 + pmul_tan4(x2);
    T zb3    = x2 - pmul_sin4(zb1);
    zb1     += pmul_tan4(zb3);
    zb3      = -zb3;
    // Compute z0,z1,z2 from the w-vector. This applies T_4(1) by two rotations, each of which consist of three shears.
    T z00    = pmul_tan1(x7)   + x4;
    T z01    = pmul_tan3(x6)   + x5;
    T z10    = -pmul_sin1(z00) + x7;
    T z11    = -pmul_sin3(z01) + x6;
    T z20    = pmul_tan1(z10)  + z00;
    T z21    = pmul_tan3(z11)  + z01;
    // The output vector is now: zb0..zb3,z20,z21,z11,-z10.
    // This is called x^(2) in the bommer paper.
    // Compute again the bold Z vector as C_II oplus C_II
    T zc0    = z20 + pmul_tan4(z21);
    T zc1    = z21 - pmul_sin4(zc0);
    zc0     += pmul_tan4(zc1);
    zc1      = -zc1;
    T zc3    = z11 + pmul_tan4(z10);
    T zc2    = z10 - pmul_sin4(zc3);
    zc3     += pmul_tan4(zc2);
    zc2      = -zc2;
    // Compute z0,z1,z2 from the w-vector
    z00 = pmul_tan4(zb1)  + zb0;
    z01 = pmul_tan2(zb3)  + zb2;
    z10 = -pmul_sin4(z00) + zb1;
    z11 = -pmul_sin2(z01) + zb3;
    z20 = pmul_tan4(z10)  + z00;
    z21 = pmul_tan2(z11)  + z01;
    // Compute the x3 vector: z20,-z10,z21,-z11,zc0,zc1,zc2,zc3
    // Not needed here, instead go ahead directly.
    // Instead, compute x4 directly. The first four componets
    // are identical to x3, zc2 = x63
    // Upper part of last matrix is identity
    // Upper part of A_4(1) is identity
    T z0  = pmul_tan4(zc3) + zc1;
    T z1  = -pmul_sin4(z0) + zc3;
    T x45 = pmul_tan4(z1)  + z0;
    // Output permutation.
    dp[0] = Quantize(z20 - dcoffset,qp[0],band + 0);
    dp[1] = Quantize(zc0           ,qp[1],band + 1);
    dp[2] = Quantize(z21           ,qp[2],band + 2);
    dp[3] = Quantize(-z1           ,qp[3],band + 3);
    dp[4] = Quantize(-z10          ,qp[4],band + 4);
    dp[5] = Quantize(x45           ,qp[5],band + 5);
    dp[6] = Quantize(-z11          ,qp[6],band + 6);
    dp[7] = Quantize(zc2           ,qp[7],band + 7);

    dcoffset = 0;
    band    += 8;
  }
}
///

/// LiftingDCT::InverseTransformBlock
// Run the inverse DCT on an 8x8 block reconstructing the data.
template<int preshift,typename T,bool deadzone,bool optimize>
void LiftingDCT<preshift,T,deadzone,optimize>::InverseTransformBlock(LONG *target,const LONG *source,
                                                                     LONG dcoffset)
{
  const LONG *qp = m_plQuant;
  T t;

  dcoffset   <<= 3;
  
  if (source) {
    LONG *dp;
    LONG *dpend = target + (8 << 3);
    for(dp = target;dp < dpend;dp += 8,qp += 8, source += 8) {
      // Inverse output permutation.
      T z20 =  source[0] * qp[0] + dcoffset;
      T zc0 =  source[1] * qp[1];
      T z21 =  source[2] * qp[2];
      T  z1 = -source[3] * qp[3];
      T z10 = -source[4] * qp[4];
      T x45 =  source[5] * qp[5];
      T z11 = -source[6] * qp[6];
      T zc2 =  source[7] * qp[7];
      // rotation by 45 degrees of x45,x46 to zc1,zc3.
      T z0  = x45 - pmul_tan4(z1);
      T zc3 = z1  + pmul_sin4(z0);
      T zc1 = z0  - pmul_tan4(zc3);
      // Next rotation pair.
      T z00 = z20 - pmul_tan4(z10);
      T z01 = z21 - pmul_tan2(z11);
      T zb1 = z10 + pmul_sin4(z00);
      T zb3 = z11 + pmul_sin2(z01);
      T zb0 = z00 - pmul_tan4(zb1);
      T zb2 = z01 - pmul_tan2(zb3);
      // Small butterfly. 
      zc1      = -zc1;
      zc0     -= pmul_tan4(zc1);
      z21      = zc1 + pmul_sin4(zc0);
      z20      = zc0 - pmul_tan4(z21);
      zc2      = -zc2;
      zc3     -= pmul_tan4(zc2);
      z10      = zc2 + pmul_sin4(zc3);
      z11      = zc3 - pmul_tan4(z10);
      // Rotation by 3Pi/16 and 1Pi/16.
      z00      = z20 - pmul_tan1(z10);
      z01      = z21 - pmul_tan3(z11);
      T x7  = z10 + pmul_sin1(z00);
      T x6  = z11 + pmul_sin3(z01);
      T x4  = z00 - pmul_tan1(x7);
      T x5  = z01 - pmul_tan3(x6);
      // Small butterfly again to compute x0,x1,x2,x3, again only inverted up to a scale factor of 2.
      zb2      = -zb2;
      zb0     -= pmul_tan4(zb2);
      T x3  = zb2 + pmul_sin4(zb0);
      T x0  = zb0 - pmul_tan4(x3);

      zb3      = -zb3;
      zb1     -= pmul_tan4(zb3);
      T x2  = zb3 + pmul_sin4(zb1);
      T x1  = zb1 - pmul_tan4(x2);
      // Finally, the output bufferfly.
      x4       = -x4;
      x0      -= pmul_tan4(x4);
      x4      += pmul_sin4(x0);
      x0      -= pmul_tan4(x4);
      dp[0]    = x0;
      dp[7]    = x4;

      x5       = -x5;
      x1      -= pmul_tan4(x5);
      x5      += pmul_sin4(x1);
      x1      -= pmul_tan4(x5);
      dp[1]    = x1;
      dp[6]    = x5;

      x6       = -x6;
      x2      -= pmul_tan4(x6);
      x6      += pmul_sin4(x2);
      x2      -= pmul_tan4(x6);
      dp[2]    = x2;
      dp[5]    = x6;

      x7       = -x7;
      x3      -= pmul_tan4(x7);
      x7      += pmul_sin4(x3);
      x3      -= pmul_tan4(x7);
      dp[3]    = x3;
      dp[4]    = x7;
      //
      dcoffset = 0;
    }
    //
    // Finally, loop over the columns
    dpend = target + 8;
    for(dp = target;dp < dpend;dp++) {
      // Inverse output permutation.
      T z20    =  dp[0 << 3];
      T zc0    =  dp[1 << 3];
      T z21    =  dp[2 << 3];
      T z1     = -dp[3 << 3];
      T z10    = -dp[4 << 3];
      T x45    =  dp[5 << 3];
      T z11    = -dp[6 << 3];
      T zc2    =  dp[7 << 3];
      // rotation by 45 degrees of x45,x46 to zc1,zc3.
      T z0     = x45 - pmul_tan4(z1);
      T zc3    = z1  + pmul_sin4(z0);
      T zc1    = z0  - pmul_tan4(zc3);
      // Next rotation pair.
      T z00    = z20 - pmul_tan4(z10);
      T z01    = z21 - pmul_tan2(z11);
      T zb1    = z10 + pmul_sin4(z00);
      T zb3    = z11 + pmul_sin2(z01);
      T zb0    = z00 - pmul_tan4(zb1);
      T zb2    = z01 - pmul_tan2(zb3);
      // Small butterfly. This is not exactly inverted, but inverted up to a scale factor of 2.
      zc1      = -zc1;
      zc0     -= pmul_tan4(zc1);
      z21      = zc1 + pmul_sin4(zc0);
      z20      = zc0 - pmul_tan4(z21);
      zc2      = -zc2;
      zc3     -= pmul_tan4(zc2);
      z10      = zc2 + pmul_sin4(zc3);
      z11      = zc3 - pmul_tan4(z10);
      // Rotation by 3Pi/16 and 1Pi/16.
      z00      = z20 - pmul_tan1(z10);
      z01      = z21 - pmul_tan3(z11);
      T x7     = z10 + pmul_sin1(z00);
      T x6     = z11 + pmul_sin3(z01);
      T x4     = z00 - pmul_tan1(x7);
      T x5     = z01 - pmul_tan3(x6);
      // Small butterfly again to compute x0,x1,x2,x3, again only inverted up to a scale factor of 2.
      zb2      = -zb2;
      zb0     -= pmul_tan4(zb2);
      T x3     = zb2 + pmul_sin4(zb0);
      T x0     = zb0 - pmul_tan4(x3);

      zb3      = -zb3;
      zb1     -= pmul_tan4(zb3);
      T x2     = zb3 + pmul_sin4(zb1);
      T x1     = zb1 - pmul_tan4(x2);
      // Finally, the output bufferfly.
      x4       = -x4;
      x0      -= pmul_tan4(x4);
      x4      += pmul_sin4(x0);
      x0      -= pmul_tan4(x4);
      dp[0 << 3] = x0 << preshift;
      dp[7 << 3] = x4 << preshift;

      x5       = -x5;
      x1      -= pmul_tan4(x5);
      x5      += pmul_sin4(x1);
      x1      -= pmul_tan4(x5);
      dp[1 << 3] = x1 << preshift;
      dp[6 << 3] = x5 << preshift;

      x6       = -x6;
      x2      -= pmul_tan4(x6);
      x6      += pmul_sin4(x2);
      x2      -= pmul_tan4(x6);
      dp[2 << 3] = x2 << preshift;
      dp[5 << 3] = x6 << preshift;

      x7       = -x7;
      x3      -= pmul_tan4(x7);
      x7      += pmul_sin4(x3);
      x3      -= pmul_tan4(x7);
      dp[3 << 3] = x3 << preshift;
      dp[4 << 3] = x7 << preshift;
    }
  } else {
    memset(target,0,sizeof(LONG) * 64);
  }
}
///


/// LiftingDCT::EstimateCriticalSlope
// Estimate a critical slope (lambda) from the unquantized data.
// Or to be precise, estimate lambda/delta^2, the constant in front of
// delta^2.
template<int preshift,typename T,bool deadzone,bool optimize>
DOUBLE LiftingDCT<preshift,T,deadzone,optimize>::EstimateCriticalSlope(void)
{
#ifdef ESTIMATE_FROM_ENERGY
  int i;
  double energy = 0.0;
  const double s1    = pow(2.0,14.75);
  const double s2    = pow(2.0,16.5);
  const double scale = 1.0 / 8.0; 
  // the preshift does not scale coefficients or delta here
  
  assert(optimize);
  for(i = 1;i < 63;i++) {
    double val   = m_lTransform[i] / scale;
    energy      += val * val;
  }
  energy  /= 63.0;

  return (s1 / (s2 + energy));
#else
  return 0.25;
#endif  
}
///

/// Instanciate the classes
template class LiftingDCT<0,LONG,false,false>;
template class LiftingDCT<1,LONG,false,false>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,LONG,false,false>;
template class LiftingDCT<0,QUAD,false,false>;
template class LiftingDCT<1,QUAD,false,false>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,QUAD,false,false>;

template class LiftingDCT<0,LONG,true,false>;
template class LiftingDCT<1,LONG,true,false>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,LONG,true,false>;
template class LiftingDCT<0,QUAD,true,false>;
template class LiftingDCT<1,QUAD,true,false>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,QUAD,true,false>;

template class LiftingDCT<0,LONG,false,true>;
template class LiftingDCT<1,LONG,false,true>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,LONG,false,true>;
template class LiftingDCT<0,QUAD,false,true>;
template class LiftingDCT<1,QUAD,false,true>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,QUAD,false,true>;

template class LiftingDCT<0,LONG,true,true>;
template class LiftingDCT<1,LONG,true,true>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,LONG,true,true>;
template class LiftingDCT<0,QUAD,true,true>;
template class LiftingDCT<1,QUAD,true,true>;
template class LiftingDCT<ColorTrafo::COLOR_BITS,QUAD,true,true>;
///
