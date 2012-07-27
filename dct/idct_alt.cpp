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
** Inverse DCT operation plus scaled quantization.
**
** $Id: idct_alt.cpp,v 1.2 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "std/string.hpp"
#include "dct/idct.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
///

//#define DO_HIST
#ifdef DO_HIST
#include "std/stdio.hpp"
static int histogram[64][16];
static int open = 0;
#endif

/// Defines
// Number of fractional bits.
#define TO_FIX(x) WORD((x * (1UL << FIX_BITS)) + 0.5)
// Actually, this is also simply a long, but prescaled by FIX_BITS
typedef LONG FIXED;
// This is also simply LONG, but with two bits prescaled
typedef LONG INTER;

// Backshift from FIX_BITS to INTERMEDIATE_BITS
#define FIXED_TO_INTERMEDIATE(x) (((x) + (1L << (FIX_BITS - INTERMEDIATE_BITS - 1))) >> (FIX_BITS - INTERMEDIATE_BITS))

// Preshifted by FIX_BITS + INTER_BITS
typedef LONG INTER_FIXED;

// Backshift from INTERMEDIATE+FIX bits to integer, plus 3 bits for the DCT scaling
#define INTER_FIXED_TO_INT(x) (((x) + (1L << (FIX_BITS + INTERMEDIATE_BITS + 3 - 1))) >> (FIX_BITS + INTERMEDIATE_BITS + 3))
///

/// IDCT::IDCT
IDCT::IDCT(class Environ *env)
  : DCT(env)
{
#ifdef DO_HIST
  open++;

#endif
}
///

/// IDCT::~IDCT
IDCT::~IDCT(void)
{
#ifdef DO_HIST
  open--;
  if (open == 0) { 
    int i;
    char filename[256];
    for(i = 0;i < 64;i++) {
      snprintf(filename,sizeof(filename),"hist_%d_%d.plot",i % 8,i >> 3);
      FILE *hist = fopen(filename,"w");
      if (hist) {
	int k;
	for(k = 0;k < 16;k++) {
	  fprintf(hist,"%d\t%d\n",k - 8,histogram[i][k]);
	}
	fclose(hist);
      }
    }
  }
#endif
}
///

/// IDCT::DefineQuant
void IDCT::DefineQuant(const UWORD *table)
{
  int i;

  // No scaling required here.
  for(i = 0;i < 64;i++) {
    m_psQuant[i]    = table[i];
    m_plInvQuant[i] = LONG(FLOAT(1L << QUANTIZER_BITS) / table[i] + 0.5);
  }
}
///

/// IDCT::Quantize
// Quantize a floating point number with a multiplier, round correctly.
// Must remove FIX_BITS + INTER_BITS + 3
LONG IDCT::Quantize(LONG no,LONG qnt,LONG &residual)
{
  LONG k   = FIXED_TO_INTERMEDIATE(no); // still 3 bits too much.
  LONG n   = (k + 4) >> 3; // the division by eight normalization of the DCT.
  residual = k - (n << 3);
  
  if (n >= 0) {
    return   (n * QUAD(qnt) + (QUAD(1) << (QUANTIZER_BITS - 1))) >> 
      (QUANTIZER_BITS);
  } else {
    // The -1 makes this the same rounding rule as a shift.
    return -((-n * QUAD(qnt) - 1 + (QUAD(1) << (QUANTIZER_BITS - 1))) >> 
	     (QUANTIZER_BITS));
  }
}
///

/// IDCT::ForwardFilterCore
// Perform the forwards filtering
void IDCT::ForwardFilterCore(LONG *d,LONG *target,LONG *residual)
{ 
  LONG *dpend,*dp;
  const LONG *qp = m_plInvQuant;
  //
  // Pass over rows.
  dpend = d + 64;
  for(dp = d;dp < dpend;dp += 8) {
    LONG tmp0    = dp[0] + dp[7];
    LONG tmp1    = dp[1] + dp[6];
    LONG tmp2    = dp[2] + dp[5];
    LONG tmp3    = dp[3] + dp[4];
    LONG tmp10   = tmp0  + tmp3;
    LONG tmp12   = tmp0  - tmp3;
    LONG tmp11   = tmp1  + tmp2;
    LONG tmp13   = tmp1  - tmp2;
    
    tmp0         = dp[0] - dp[7];
    tmp1         = dp[1] - dp[6];
    tmp2         = dp[2] - dp[5];
    tmp3         = dp[3] - dp[4];

    // complete DC and middle band.
    dp[0]        = (tmp10 + tmp11) << INTERMEDIATE_BITS;
    dp[4]        = (tmp10 - tmp11) << INTERMEDIATE_BITS;

    FIXED   z1   = (tmp12 + tmp13) * TO_FIX(0.541196100);

    // complete bands 2 and 6
    dp[2]        = FIXED_TO_INTERMEDIATE(z1 + tmp12 * TO_FIX(0.765366865));
    dp[6]        = FIXED_TO_INTERMEDIATE(z1 + tmp13 *-TO_FIX(1.847759065));

    
    tmp10        = tmp0 + tmp3;
    tmp11        = tmp1 + tmp2;
    tmp12        = tmp0 + tmp2;
    tmp13        = tmp1 + tmp3;
    z1           = (tmp12 + tmp13) * TO_FIX(1.175875602);

    FIXED ttmp0  = tmp0  * TO_FIX(1.501321110);
    FIXED ttmp1  = tmp1  * TO_FIX(3.072711026);
    FIXED ttmp2  = tmp2  * TO_FIX(2.053119869);
    FIXED ttmp3  = tmp3  * TO_FIX(0.298631336);
    FIXED ttmp10 = tmp10 *-TO_FIX(0.899976223);
    FIXED ttmp11 = tmp11 *-TO_FIX(2.562915447);
    FIXED ttmp12 = tmp12 *-TO_FIX(0.390180644) + z1;
    FIXED ttmp13 = tmp13 *-TO_FIX(1.961570560) + z1;

    dp[1]        = FIXED_TO_INTERMEDIATE(ttmp0 + ttmp10 + ttmp12);
    dp[3]        = FIXED_TO_INTERMEDIATE(ttmp1 + ttmp11 + ttmp13);
    dp[5]        = FIXED_TO_INTERMEDIATE(ttmp2 + ttmp11 + ttmp12);
    dp[7]        = FIXED_TO_INTERMEDIATE(ttmp3 + ttmp10 + ttmp13);
  }
  //
  // Pass over columns and quantize.
  dpend = d + 8;
  LONG *res = residual;
  for(dp = d;dp < dpend;dp++,target++,qp++,res++) { 
    INTER tmp0         = dp[0 << 3] + dp[7 << 3];
    INTER tmp1         = dp[1 << 3] + dp[6 << 3];
    INTER tmp2         = dp[2 << 3] + dp[5 << 3];
    INTER tmp3         = dp[3 << 3] + dp[4 << 3];
    INTER tmp10        = tmp0  + tmp3;
    INTER tmp12        = tmp0  - tmp3;
    INTER tmp11        = tmp1  + tmp2;
    INTER tmp13        = tmp1  - tmp2;
    
    tmp0               = dp[0 << 3] - dp[7 << 3];
    tmp1               = dp[1 << 3] - dp[6 << 3];
    tmp2               = dp[2 << 3] - dp[5 << 3];
    tmp3               = dp[3 << 3] - dp[4 << 3];

    // complete DC and middle band.
    target[0 << 3]     = Quantize((tmp10 + tmp11) << FIX_BITS,qp[0 << 3],res[0 << 3]);
    target[4 << 3]     = Quantize((tmp10 - tmp11) << FIX_BITS,qp[4 << 3],res[4 << 3]);
    
    INTER_FIXED z1     = (tmp12 + tmp13) * TO_FIX(0.541196100);

    // complete bands 2 and 6
    target[2 << 3]     = Quantize(z1 + tmp12 * TO_FIX(0.765366865),qp[2 << 3],res[2 << 3]);
    target[6 << 3]     = Quantize(z1 + tmp13 *-TO_FIX(1.847759065),qp[6 << 3],res[6 << 3]);

    tmp10              = tmp0 + tmp3;
    tmp11              = tmp1 + tmp2;
    tmp12              = tmp0 + tmp2;
    tmp13              = tmp1 + tmp3;
    z1                 = (tmp12 + tmp13) * TO_FIX(1.175875602);

    INTER_FIXED ttmp0  = tmp0  * TO_FIX(1.501321110);
    INTER_FIXED ttmp1  = tmp1  * TO_FIX(3.072711026);
    INTER_FIXED ttmp2  = tmp2  * TO_FIX(2.053119869);
    INTER_FIXED ttmp3  = tmp3  * TO_FIX(0.298631336);
    INTER_FIXED ttmp10 = tmp10 *-TO_FIX(0.899976223);
    INTER_FIXED ttmp11 = tmp11 *-TO_FIX(2.562915447);
    INTER_FIXED ttmp12 = tmp12 *-TO_FIX(0.390180644) + z1;
    INTER_FIXED ttmp13 = tmp13 *-TO_FIX(1.961570560) + z1;

    target[1 << 3]     = Quantize(ttmp0 + ttmp10 + ttmp12,qp[1 << 3],res[1 << 3]);
    target[3 << 3]     = Quantize(ttmp1 + ttmp11 + ttmp13,qp[3 << 3],res[3 << 3]);
    target[5 << 3]     = Quantize(ttmp2 + ttmp11 + ttmp12,qp[5 << 3],res[5 << 3]);
    target[7 << 3]     = Quantize(ttmp3 + ttmp10 + ttmp13,qp[7 << 3],res[7 << 3]);
  }
}
///

/// IDCT::ForwardFilterCore
// Perform the forwards filtering
void IDCT::ForwardFilterCore(LONG *d,LONG *target)
{ 
  LONG *dpend,*dp;
  const LONG *qp = m_plInvQuant;
  //
  // Pass over rows.
  dpend = d + 64;
  for(dp = d;dp < dpend;dp += 8) {
    LONG tmp0    = dp[0] + dp[7];
    LONG tmp1    = dp[1] + dp[6];
    LONG tmp2    = dp[2] + dp[5];
    LONG tmp3    = dp[3] + dp[4];
    LONG tmp10   = tmp0  + tmp3;
    LONG tmp12   = tmp0  - tmp3;
    LONG tmp11   = tmp1  + tmp2;
    LONG tmp13   = tmp1  - tmp2;
    
    tmp0         = dp[0] - dp[7];
    tmp1         = dp[1] - dp[6];
    tmp2         = dp[2] - dp[5];
    tmp3         = dp[3] - dp[4];

    // complete DC and middle band.
    dp[0]        = (tmp10 + tmp11) << INTERMEDIATE_BITS;
    dp[4]        = (tmp10 - tmp11) << INTERMEDIATE_BITS;

    FIXED   z1   = (tmp12 + tmp13) * TO_FIX(0.541196100);

    // complete bands 2 and 6
    dp[2]        = FIXED_TO_INTERMEDIATE(z1 + tmp12 * TO_FIX(0.765366865));
    dp[6]        = FIXED_TO_INTERMEDIATE(z1 + tmp13 *-TO_FIX(1.847759065));

    
    tmp10        = tmp0 + tmp3;
    tmp11        = tmp1 + tmp2;
    tmp12        = tmp0 + tmp2;
    tmp13        = tmp1 + tmp3;
    z1           = (tmp12 + tmp13) * TO_FIX(1.175875602);

    FIXED ttmp0  = tmp0  * TO_FIX(1.501321110);
    FIXED ttmp1  = tmp1  * TO_FIX(3.072711026);
    FIXED ttmp2  = tmp2  * TO_FIX(2.053119869);
    FIXED ttmp3  = tmp3  * TO_FIX(0.298631336);
    FIXED ttmp10 = tmp10 *-TO_FIX(0.899976223);
    FIXED ttmp11 = tmp11 *-TO_FIX(2.562915447);
    FIXED ttmp12 = tmp12 *-TO_FIX(0.390180644) + z1;
    FIXED ttmp13 = tmp13 *-TO_FIX(1.961570560) + z1;

    dp[1]        = FIXED_TO_INTERMEDIATE(ttmp0 + ttmp10 + ttmp12);
    dp[3]        = FIXED_TO_INTERMEDIATE(ttmp1 + ttmp11 + ttmp13);
    dp[5]        = FIXED_TO_INTERMEDIATE(ttmp2 + ttmp11 + ttmp12);
    dp[7]        = FIXED_TO_INTERMEDIATE(ttmp3 + ttmp10 + ttmp13);
  }
  //
  // Pass over columns and quantize.
  dpend = d + 8;
  for(dp = d;dp < dpend;dp++,target++,qp++) { 
    INTER tmp0         = dp[0 << 3] + dp[7 << 3];
    INTER tmp1         = dp[1 << 3] + dp[6 << 3];
    INTER tmp2         = dp[2 << 3] + dp[5 << 3];
    INTER tmp3         = dp[3 << 3] + dp[4 << 3];
    INTER tmp10        = tmp0  + tmp3;
    INTER tmp12        = tmp0  - tmp3;
    INTER tmp11        = tmp1  + tmp2;
    INTER tmp13        = tmp1  - tmp2;
    
    tmp0               = dp[0 << 3] - dp[7 << 3];
    tmp1               = dp[1 << 3] - dp[6 << 3];
    tmp2               = dp[2 << 3] - dp[5 << 3];
    tmp3               = dp[3 << 3] - dp[4 << 3];

    // complete DC and middle band.
    target[0 << 3]     = Quantize((tmp10 + tmp11) << FIX_BITS,qp[0 << 3]);
    target[4 << 3]     = Quantize((tmp10 - tmp11) << FIX_BITS,qp[4 << 3]);
    
    INTER_FIXED z1     = (tmp12 + tmp13) * TO_FIX(0.541196100);

    // complete bands 2 and 6
    target[2 << 3]     = Quantize(z1 + tmp12 * TO_FIX(0.765366865),qp[2 << 3]);
    target[6 << 3]     = Quantize(z1 + tmp13 *-TO_FIX(1.847759065),qp[6 << 3]);

    tmp10              = tmp0 + tmp3;
    tmp11              = tmp1 + tmp2;
    tmp12              = tmp0 + tmp2;
    tmp13              = tmp1 + tmp3;
    z1                 = (tmp12 + tmp13) * TO_FIX(1.175875602);

    INTER_FIXED ttmp0  = tmp0  * TO_FIX(1.501321110);
    INTER_FIXED ttmp1  = tmp1  * TO_FIX(3.072711026);
    INTER_FIXED ttmp2  = tmp2  * TO_FIX(2.053119869);
    INTER_FIXED ttmp3  = tmp3  * TO_FIX(0.298631336);
    INTER_FIXED ttmp10 = tmp10 *-TO_FIX(0.899976223);
    INTER_FIXED ttmp11 = tmp11 *-TO_FIX(2.562915447);
    INTER_FIXED ttmp12 = tmp12 *-TO_FIX(0.390180644) + z1;
    INTER_FIXED ttmp13 = tmp13 *-TO_FIX(1.961570560) + z1;

    target[1 << 3]     = Quantize(ttmp0 + ttmp10 + ttmp12,qp[1 << 3]);
    target[3 << 3]     = Quantize(ttmp1 + ttmp11 + ttmp13,qp[3 << 3]);
    target[5 << 3]     = Quantize(ttmp2 + ttmp11 + ttmp12,qp[5 << 3]);
    target[7 << 3]     = Quantize(ttmp3 + ttmp10 + ttmp13,qp[7 << 3]);
  }
}
///

/// IDCT::InverseFilterCore
// Inverse DCT filter core.
void IDCT::InverseFilterCore(const LONG *source,LONG *d)
{
  LONG *dptr;
  LONG *dend = d + 8;
  const WORD *qnt = m_psQuant;

  for(dptr = d;dptr < dend;dptr++,source++,qnt++) {
    // Even part.
    LONG  tz2    = source[2 << 3] * qnt[2 << 3];
    LONG  tz3    = source[6 << 3] * qnt[6 << 3];
    FIXED z1     = (tz2 + tz3) *  TO_FIX(0.541196100);
    FIXED tmp2   = z1 + tz3    * -TO_FIX(1.847759065);
    FIXED tmp3   = z1 + tz2    *  TO_FIX(0.765366865);

    tz2          = source[0 << 3] * qnt[0 << 3];
    tz3          = source[4 << 3] * qnt[4 << 3];
      
    FIXED tmp0   = (tz2 + tz3) << FIX_BITS;
    FIXED tmp1   = (tz2 - tz3) << FIX_BITS;
    FIXED tmp10  = tmp0 + tmp3;
    FIXED tmp13  = tmp0 - tmp3;
    FIXED tmp11  = tmp1 + tmp2;
    FIXED tmp12  = tmp1 - tmp2;
    
    // Odd part.
    LONG ttmp0   = source[7 << 3] * qnt[7 << 3];
    LONG ttmp1   = source[5 << 3] * qnt[5 << 3];
    LONG ttmp2   = source[3 << 3] * qnt[3 << 3];
    LONG ttmp3   = source[1 << 3] * qnt[1 << 3];
    
    LONG tz1     = ttmp0 + ttmp3;
    tz2          = ttmp1 + ttmp2;
    tz3          = ttmp0 + ttmp2;
    LONG tz4     = ttmp1 + ttmp3;
    FIXED z5     = (tz3 + tz4) * TO_FIX(1.175875602);
    
    tmp0         = ttmp0 * TO_FIX(0.298631336);
    tmp1         = ttmp1 * TO_FIX(2.053119869);
    tmp2         = ttmp2 * TO_FIX(3.072711026);
    tmp3         = ttmp3 * TO_FIX(1.501321110);
    z1           = tz1   *-TO_FIX(0.899976223);
    FIXED z2     = tz2   *-TO_FIX(2.562915447);
    FIXED z3     = tz3   *-TO_FIX(1.961570560) + z5;
    FIXED z4     = tz4   *-TO_FIX(0.390180644) + z5;
    
    tmp0        += z1 + z3;
    tmp1        += z2 + z4;
    tmp2        += z2 + z3;
    tmp3        += z1 + z4;
    
    dptr[0 << 3] = FIXED_TO_INTERMEDIATE(tmp10 + tmp3);
    dptr[7 << 3] = FIXED_TO_INTERMEDIATE(tmp10 - tmp3);
    dptr[1 << 3] = FIXED_TO_INTERMEDIATE(tmp11 + tmp2);
    dptr[6 << 3] = FIXED_TO_INTERMEDIATE(tmp11 - tmp2);
    dptr[2 << 3] = FIXED_TO_INTERMEDIATE(tmp12 + tmp1);
    dptr[5 << 3] = FIXED_TO_INTERMEDIATE(tmp12 - tmp1);
    dptr[3 << 3] = FIXED_TO_INTERMEDIATE(tmp13 + tmp0);
    dptr[4 << 3] = FIXED_TO_INTERMEDIATE(tmp13 - tmp0);
  }
    
  // After transforming over the columns, now transform over the rows.
  dend = d + 8*8;
  for(dptr = d;dptr < dend;dptr+=8) {
    INTER tz2         = dptr[2];
    INTER tz3         = dptr[6];
    INTER_FIXED z1    = (tz2 + tz3) *  TO_FIX(0.541196100);
    INTER_FIXED tmp2  = z1 +   tz3  * -TO_FIX(1.847759065);
    INTER_FIXED tmp3  = z1 +   tz2  *  TO_FIX(0.765366865);
    INTER_FIXED tmp0  = (dptr[0] + dptr[4]) << FIX_BITS;
    INTER_FIXED tmp1  = (dptr[0] - dptr[4]) << FIX_BITS;
    INTER_FIXED tmp10 = tmp0 + tmp3;
    INTER_FIXED tmp13 = tmp0 - tmp3;
    INTER_FIXED tmp11 = tmp1 + tmp2;
    INTER_FIXED tmp12 = tmp1 - tmp2;
    // Odd parts.
    INTER ttmp0       = dptr[7];
    INTER ttmp1       = dptr[5];
    INTER ttmp2       = dptr[3];
    INTER ttmp3       = dptr[1];
    INTER tz1         = ttmp0 + ttmp3;
    tz2               = ttmp1 + ttmp2;
    tz3               = ttmp0 + ttmp2;
    INTER tz4         = ttmp1 + ttmp3;
    INTER_FIXED z5    = (tz3 + tz4) * TO_FIX(1.175875602);
    tmp0              = ttmp0 * TO_FIX(0.298631336);
    tmp1              = ttmp1 * TO_FIX(2.053119869);
    tmp2              = ttmp2 * TO_FIX(3.072711026);
    tmp3              = ttmp3 * TO_FIX(1.501321110);
    z1                = tz1   *-TO_FIX(0.899976223);
    INTER_FIXED z2    = tz2   *-TO_FIX(2.562915447);
    INTER_FIXED z3    = tz3   *-TO_FIX(1.961570560) + z5;
    INTER_FIXED z4    = tz4   *-TO_FIX(0.390180644) + z5;
    tmp0             += z1 + z3;
    tmp1             += z2 + z4;
    tmp2             += z2 + z3;
    tmp3             += z1 + z4;
    
    dptr[0]           = INTER_FIXED_TO_INT(tmp10 + tmp3);
    dptr[7]           = INTER_FIXED_TO_INT(tmp10 - tmp3);
    dptr[1]           = INTER_FIXED_TO_INT(tmp11 + tmp2);
    dptr[6]           = INTER_FIXED_TO_INT(tmp11 - tmp2);
    dptr[2]           = INTER_FIXED_TO_INT(tmp12 + tmp1);
    dptr[5]           = INTER_FIXED_TO_INT(tmp12 - tmp1);
    dptr[3]           = INTER_FIXED_TO_INT(tmp13 + tmp0);
    dptr[4]           = INTER_FIXED_TO_INT(tmp13 - tmp0);
  }
}
///

/// IDCT::InverseFilterCore
// Inverse DCT filter core.
void IDCT::InverseFilterCore(const LONG *source,LONG *d,const LONG *res)
{
  LONG *dptr;
  LONG *dend = d + 8;
  const WORD *qnt = m_psQuant;
  const LONG *residual = res;

  for(dptr = d;dptr < dend;dptr++,source++,qnt++,residual++) {
    // Even part.
    LONG  tz2    = Dequantize(source[2 << 3],qnt[2 << 3],residual[2 << 3]);
    LONG  tz3    = Dequantize(source[6 << 3],qnt[6 << 3],residual[6 << 3]);
    FIXED z1     = (tz2 + tz3) *  TO_FIX(0.541196100);
    FIXED tmp2   = z1 + tz3    * -TO_FIX(1.847759065);
    FIXED tmp3   = z1 + tz2    *  TO_FIX(0.765366865);

    tz2          = Dequantize(source[0 << 3],qnt[0 << 3],residual[0 << 3]);
    tz3          = Dequantize(source[4 << 3],qnt[4 << 3],residual[4 << 3]);
      
    FIXED tmp0   = (tz2 + tz3) << FIX_BITS;
    FIXED tmp1   = (tz2 - tz3) << FIX_BITS;
    FIXED tmp10  = tmp0 + tmp3;
    FIXED tmp13  = tmp0 - tmp3;
    FIXED tmp11  = tmp1 + tmp2;
    FIXED tmp12  = tmp1 - tmp2;
    
    // Odd part.
    LONG ttmp0   = Dequantize(source[7 << 3],qnt[7 << 3],residual[7 << 3]);
    LONG ttmp1   = Dequantize(source[5 << 3],qnt[5 << 3],residual[5 << 3]);
    LONG ttmp2   = Dequantize(source[3 << 3],qnt[3 << 3],residual[3 << 3]);
    LONG ttmp3   = Dequantize(source[1 << 3],qnt[1 << 3],residual[1 << 3]);
    
    LONG tz1     = ttmp0 + ttmp3;
    tz2          = ttmp1 + ttmp2;
    tz3          = ttmp0 + ttmp2;
    LONG tz4     = ttmp1 + ttmp3;
    FIXED z5     = (tz3 + tz4) * TO_FIX(1.175875602);
    
    tmp0         = ttmp0 * TO_FIX(0.298631336);
    tmp1         = ttmp1 * TO_FIX(2.053119869);
    tmp2         = ttmp2 * TO_FIX(3.072711026);
    tmp3         = ttmp3 * TO_FIX(1.501321110);
    z1           = tz1   *-TO_FIX(0.899976223);
    FIXED z2     = tz2   *-TO_FIX(2.562915447);
    FIXED z3     = tz3   *-TO_FIX(1.961570560) + z5;
    FIXED z4     = tz4   *-TO_FIX(0.390180644) + z5;
    
    tmp0        += z1 + z3;
    tmp1        += z2 + z4;
    tmp2        += z2 + z3;
    tmp3        += z1 + z4;
    
    dptr[0 << 3] = INTER_FIXED_TO_INT(tmp10 + tmp3);
    dptr[7 << 3] = INTER_FIXED_TO_INT(tmp10 - tmp3);
    dptr[1 << 3] = INTER_FIXED_TO_INT(tmp11 + tmp2);
    dptr[6 << 3] = INTER_FIXED_TO_INT(tmp11 - tmp2);
    dptr[2 << 3] = INTER_FIXED_TO_INT(tmp12 + tmp1);
    dptr[5 << 3] = INTER_FIXED_TO_INT(tmp12 - tmp1);
    dptr[3 << 3] = INTER_FIXED_TO_INT(tmp13 + tmp0);
    dptr[4 << 3] = INTER_FIXED_TO_INT(tmp13 - tmp0);
  }
    
  // After transforming over the columns, now transform over the rows.
  dend = d + 8*8;
  for(dptr = d;dptr < dend;dptr+=8) {
    INTER tz2         = dptr[2];
    INTER tz3         = dptr[6];
    INTER_FIXED z1    = (tz2 + tz3) *  TO_FIX(0.541196100);
    INTER_FIXED tmp2  = z1 +   tz3  * -TO_FIX(1.847759065);
    INTER_FIXED tmp3  = z1 +   tz2  *  TO_FIX(0.765366865);
    INTER_FIXED tmp0  = (dptr[0] + dptr[4]) << FIX_BITS;
    INTER_FIXED tmp1  = (dptr[0] - dptr[4]) << FIX_BITS;
    INTER_FIXED tmp10 = tmp0 + tmp3;
    INTER_FIXED tmp13 = tmp0 - tmp3;
    INTER_FIXED tmp11 = tmp1 + tmp2;
    INTER_FIXED tmp12 = tmp1 - tmp2;
    // Odd parts.
    INTER ttmp0       = dptr[7];
    INTER ttmp1       = dptr[5];
    INTER ttmp2       = dptr[3];
    INTER ttmp3       = dptr[1];
    INTER tz1         = ttmp0 + ttmp3;
    tz2               = ttmp1 + ttmp2;
    tz3               = ttmp0 + ttmp2;
    INTER tz4         = ttmp1 + ttmp3;
    INTER_FIXED z5    = (tz3 + tz4) * TO_FIX(1.175875602);
    tmp0              = ttmp0 * TO_FIX(0.298631336);
    tmp1              = ttmp1 * TO_FIX(2.053119869);
    tmp2              = ttmp2 * TO_FIX(3.072711026);
    tmp3              = ttmp3 * TO_FIX(1.501321110);
    z1                = tz1   *-TO_FIX(0.899976223);
    INTER_FIXED z2    = tz2   *-TO_FIX(2.562915447);
    INTER_FIXED z3    = tz3   *-TO_FIX(1.961570560) + z5;
    INTER_FIXED z4    = tz4   *-TO_FIX(0.390180644) + z5;
    tmp0             += z1 + z3;
    tmp1             += z2 + z4;
    tmp2             += z2 + z3;
    tmp3             += z1 + z4;
    
    dptr[0]           = INTER_FIXED_TO_INT(tmp10 + tmp3);
    dptr[7]           = INTER_FIXED_TO_INT(tmp10 - tmp3);
    dptr[1]           = INTER_FIXED_TO_INT(tmp11 + tmp2);
    dptr[6]           = INTER_FIXED_TO_INT(tmp11 - tmp2);
    dptr[2]           = INTER_FIXED_TO_INT(tmp12 + tmp1);
    dptr[5]           = INTER_FIXED_TO_INT(tmp12 - tmp1);
    dptr[3]           = INTER_FIXED_TO_INT(tmp13 + tmp0);
    dptr[4]           = INTER_FIXED_TO_INT(tmp13 - tmp0);
  }
}
///

/// IDCT::TransformBlock
// Run the DCT on a 8x8 block on the input data, giving the output table.
void IDCT::TransformBlock(const struct ImageBitMap *source,const RectAngle<LONG> &r,
			  LONG *target,LONG max,LONG offset)
{  
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  LONG d[64];
  //
  switch(source->ibm_ucPixelType) {
  case CTYP_UBYTE:
    if (max > 255) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::TransformBlock",
		"cannot encode samples deeper than 8 bits from UBYTES");
    } else {
      const UBYTE *srcp = (const UBYTE *)(source->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *dst = d + xmin + (y << 3);
	const UBYTE *src = srcp;
	for(x = xmin;x <= xmax;x++) {
	  *dst = *src - offset;
	  dst++;
	  src += source->ibm_cBytesPerPixel;
	}
	srcp += source->ibm_lBytesPerRow;
      }
    }
    break;
  case CTYP_UWORD:  
    if (max > 65535) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::TransformBlock",
		"cannot encode samples deeper than 16 bits from UWORDS");
    } else {
      const UWORD *srcp = (const UWORD *)(source->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *dst = d + xmin + (y << 3);
	const UWORD *src = srcp;
	for(x = xmin;x <= xmax;x++) {
	  *dst = *src - offset;
	  dst++;
	  src += source->ibm_cBytesPerPixel;
	}
	srcp += source->ibm_lBytesPerRow;
      }
    }
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"IDCT::TransformBlock",
	      "sample type unknown");
    break;
  }
  //
  ForwardFilterCore(d,target);
}
///

/// IDCT::TransformBlockWithResidual
// Run the DCT on an 8x8 block, store the error in a separate block.
void IDCT::TransformBlockWithResidual(const struct ImageBitMap *source,const RectAngle<LONG> &r,
				      LONG *target,LONG max,LONG offset,LONG *residual)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  LONG d[64];
  int i;
  //
  switch(source->ibm_ucPixelType) {
  case CTYP_UBYTE:
    if (max > 255) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::TransformBlock",
		"cannot encode samples deeper than 8 bits from UBYTES");
    } else {
      const UBYTE *srcp = (const UBYTE *)(source->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *dst = d + xmin + (y << 3);
	const UBYTE *src = srcp;
	for(x = xmin;x <= xmax;x++) {
	  *dst = *src - offset;
	  dst++;
	  src += source->ibm_cBytesPerPixel;
	}
	srcp += source->ibm_lBytesPerRow;
      }
    }
    break;
  case CTYP_UWORD:  
    if (max > 65535) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::TransformBlock",
		"cannot encode samples deeper than 16 bits from UWORDS");
    } else {
      const UWORD *srcp = (const UWORD *)(source->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *dst = d + xmin + (y << 3);
	const UWORD *src = srcp;
	for(x = xmin;x <= xmax;x++) {
	  *dst = *src - offset;
	  dst++;
	  src += source->ibm_cBytesPerPixel;
	}
	srcp += source->ibm_lBytesPerRow;
      }
    }
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"IDCT::TransformBlock",
	      "sample type unknown");
    break;
  }
  //
  /*
  // Keep the original
  memcpy(residual,d,sizeof(d));
  //
  */
  // Now transform into target.
  ForwardFilterCore(d,target,residual);
  //
  /*
  // Backwards transform, creating the residual.
  InverseFilterCore(target,d);
  //
  for(i = 0;i < 64;i++) {
    residual[i] -= d[i];
  }
  */
  //
#ifdef DO_HIST
  for(i = 0;i < 64;i++) {
    histogram[i][residual[i] + 8]++;
  }
#endif

  for(i = 0;i < 64;i++) {
    if ((i & 0x07) != 0 && (i >> 3) != 0) {
      residual[i] = residual[i] >> 1;
    }
  }
}
///

/// IDCT::InverseTransformBlock
// Run the inverse DCT on an 8x8 block reconstructing the data.
void IDCT::InverseTransformBlock(const struct ImageBitMap *target,const RectAngle<LONG> &r,
				 const LONG *source,LONG max,LONG offset)
{ 
  LONG d[64];
  LONG xmin  = r.ra_MinX & 7;
  LONG ymin  = r.ra_MinY & 7;
  LONG xmax  = r.ra_MaxX & 7;
  LONG ymax  = r.ra_MaxY & 7;
  LONG x,y;

  if (source) {
    InverseFilterCore(source,d);
  } else {
    memset(d,0,sizeof(d));
  }
  
  switch(target->ibm_ucPixelType) {
  case CTYP_UBYTE:
    if (max > 255) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::InverseTransformBlock",
		"cannot reconstruct samples deeper than 8 bits into UBYTES");
    } else {
      UBYTE *dstp = (UBYTE *)(target->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *src  = d + xmin + (y << 3);
	UBYTE *dst = dstp;
	for(x = xmin;x <= xmax;x++) {
	  LONG s = *src + offset;
	  if (s < 0) {
	    *dst = 0;
	  } else if (s > max) {
	    *dst = max;
	  } else {
	    *dst = UBYTE(s);
	  }
	  src++;
	  dst += target->ibm_cBytesPerPixel;
	}
	dstp += target->ibm_lBytesPerRow;
      }
    }
    break;
  case CTYP_UWORD:  
    if (max > 65535) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::InverseTransformBlock",
		"cannot reconstruct samples deeper than 16 bits into UWORDS");
    } else {
      UWORD *dstp = (UWORD *)(target->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *src  = d + xmin + (y << 3);
	UWORD *dst = dstp;
	for(x = xmin;x <= xmax;x++) {
	  LONG s = *src + offset;
	  if (s < 0) {
	    *dst = 0;
	  } else if (s > max) {
	    *dst = max;
	  } else {
	    *dst = UBYTE(s);
	  }
	  src++;
	  dst = (UWORD *)((UBYTE *)(dst) + target->ibm_cBytesPerPixel);
	}
	dstp += target->ibm_lBytesPerRow;
      }
    }
    break;
  case 0: // Do not reconstruct
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"IDCT::InverseTransformBlock",
	      "pixel type is unsupported, currently UBYTE and UWORD only");
  }
}
///

/// IDCT::InverseTransformBlockWithResidual
// Run the inverse DCT on an 8x8 block reconstructing the data.
void IDCT::InverseTransformBlockWithResidual(const struct ImageBitMap *target,const RectAngle<LONG> &r,
					     const LONG *source,LONG max,LONG offset,const LONG *residual)
{
  LONG d[64],rs[64];
  LONG xmin  = r.ra_MinX & 7;
  LONG ymin  = r.ra_MinY & 7;
  LONG xmax  = r.ra_MaxX & 7;
  LONG ymax  = r.ra_MaxY & 7;
  LONG x,y;

  if (source) {
    int i;

    for(i = 0;i < 64;i++) {
      if ((i & 0x07) != 0 && (i >> 3) != 0) {
	rs[i] = residual[i] << 1;
      } else {
	rs[i] = residual[i];
      }
    }
    InverseFilterCore(source,d,rs);
    
    /*
    for(i = 0;i < 64;i++) {
      d[i] += residual[i];
    }
    */
  } else {
    memset(d,0,sizeof(d));
  }
  
  switch(target->ibm_ucPixelType) {
  case CTYP_UBYTE:
    if (max > 255) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::InverseTransformBlock",
		"cannot reconstruct samples deeper than 8 bits into UBYTES");
    } else {
      UBYTE *dstp = (UBYTE *)(target->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *src  = d + xmin + (y << 3);
	UBYTE *dst = dstp;
	for(x = xmin;x <= xmax;x++) {
	  LONG s = *src + offset;
	  if (s < 0) {
	    *dst = 0;
	  } else if (s > max) {
	    *dst = max;
	  } else {
	    *dst = UBYTE(s);
	  }
	  src++;
	  dst += target->ibm_cBytesPerPixel;
	}
	dstp += target->ibm_lBytesPerRow;
      }
    }
    break;
  case CTYP_UWORD:  
    if (max > 65535) {
      JPG_THROW(OVERFLOW_PARAMETER,"IDCT::InverseTransformBlock",
		"cannot reconstruct samples deeper than 16 bits into UWORDS");
    } else {
      UWORD *dstp = (UWORD *)(target->ibm_pData);
      for(y = ymin;y <= ymax;y++) {
	LONG *src  = d + xmin + (y << 3);
	UWORD *dst = dstp;
	for(x = xmin;x <= xmax;x++) {
	  LONG s = *src + offset;
	  if (s < 0) {
	    *dst = 0;
	  } else if (s > max) {
	    *dst = max;
	  } else {
	    *dst = UBYTE(s);
	  }
	  src++;
	  dst = (UWORD *)((UBYTE *)(dst) + target->ibm_cBytesPerPixel);
	}
	dstp += target->ibm_lBytesPerRow;
      }
    }
    break;
  case 0: // Do not reconstruct
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"IDCT::InverseTransformBlock",
	      "pixel type is unsupported, currently UBYTE and UWORD only");
  }
}
///
