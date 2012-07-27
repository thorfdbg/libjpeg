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
** $Id: idct.cpp,v 1.13 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "std/string.hpp"
#include "dct/idct.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
///

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
template<int preshift>
IDCT<preshift>::IDCT(class Environ *env)
  : DCT(env)
{
}
///

/// IDCT::~IDCT
template<int preshift>
IDCT<preshift>::~IDCT(void)
{
}
///

/// IDCT::DefineQuant
template<int preshift>
void IDCT<preshift>::DefineQuant(const UWORD *table)
{
  int i;

  // No scaling required here.
  for(i = 0;i < 64;i++) {
    m_psQuant[i]    = table[i] << preshift;
    m_plInvQuant[i] = LONG(FLOAT(1L << QUANTIZER_BITS) / table[i] + 0.5);
  }
}
///

/// IDCT::TransformBlock
// Run the DCT on a 8x8 block on the input data, giving the output table.
template<int preshift>
void IDCT<preshift>::TransformBlock(const LONG *source,LONG *target,LONG dcoffset)
{ 
  LONG *dpend,*dp;
  const LONG *qp = m_plInvQuant; 
  //
  // Adjust the DC offset to the number of fractional bits.
  dcoffset <<= preshift + 3 + 3 + INTERMEDIATE_BITS; 
  // three additional bits because we still need to divide by 8.
  //
  // Pass over rows.
  dpend = target + 64;
  for(dp = target;dp < dpend;dp += 8,source += 8) {
    LONG tmp0    = source[0] + source[7];
    LONG tmp1    = source[1] + source[6];
    LONG tmp2    = source[2] + source[5];
    LONG tmp3    = source[3] + source[4];
    LONG tmp10   = tmp0      + tmp3;
    LONG tmp12   = tmp0      - tmp3;
    LONG tmp11   = tmp1      + tmp2;
    LONG tmp13   = tmp1      - tmp2;
    
    tmp0         = source[0] - source[7];
    tmp1         = source[1] - source[6];
    tmp2         = source[2] - source[5];
    tmp3         = source[3] - source[4];

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
  dpend = target + 8;
  for(dp = target;dp < dpend;dp++,qp++) { 
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
    dp[0 << 3]         = Quantize((tmp10 + tmp11 - dcoffset) << FIX_BITS,qp[0 << 3]);
    dp[4 << 3]         = Quantize((tmp10 - tmp11) << FIX_BITS,qp[4 << 3]);
    
    INTER_FIXED z1     = (tmp12 + tmp13) * TO_FIX(0.541196100);

    // complete bands 2 and 6
    dp[2 << 3]         = Quantize(z1 + tmp12 * TO_FIX(0.765366865),qp[2 << 3]);
    dp[6 << 3]         = Quantize(z1 + tmp13 *-TO_FIX(1.847759065),qp[6 << 3]);

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

    dp[1 << 3]         = Quantize(ttmp0 + ttmp10 + ttmp12,qp[1 << 3]);
    dp[3 << 3]         = Quantize(ttmp1 + ttmp11 + ttmp13,qp[3 << 3]);
    dp[5 << 3]         = Quantize(ttmp2 + ttmp11 + ttmp12,qp[5 << 3]);
    dp[7 << 3]         = Quantize(ttmp3 + ttmp10 + ttmp13,qp[7 << 3]);
    dcoffset           = 0;
  }
}
///

/// IDCT::InverseTransformBlock
// Run the inverse DCT on an 8x8 block reconstructing the data.
template<int preshift>
void IDCT<preshift>::InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset)
{
  LONG *dptr;
  LONG *dend = target + 8;
  const WORD *qnt = m_psQuant;

  dcoffset <<= preshift + 3;

  if (source) {
    for(dptr = target;dptr < dend;dptr++,source++,qnt++) {
      // Even part.
      LONG  tz2    = source[2 << 3] * qnt[2 << 3];
      LONG  tz3    = source[6 << 3] * qnt[6 << 3];
      FIXED z1     = (tz2 + tz3) *  TO_FIX(0.541196100);
      FIXED tmp2   = z1 + tz3    * -TO_FIX(1.847759065);
      FIXED tmp3   = z1 + tz2    *  TO_FIX(0.765366865);
      
      tz2          = source[0 << 3] * qnt[0 << 3] + dcoffset;
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
      dcoffset     = 0;
    }
    
    // After transforming over the columns, now transform over the rows.
    dend = target + 8*8;
    for(dptr = target;dptr < dend;dptr+=8) {
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
  } else {
    memset(target,0,sizeof(LONG) * 64);
  }
}
///

/// Instanciate the classes
template class IDCT<0>;
template class IDCT<ColorTrafo::COLOR_BITS>;
///
