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
** Floating point DCT operation plus scaled quantization.
**
** $Id: fdct.cpp,v 1.5 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "dct/fdct.hpp"
#include "std/string.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "interface/types.hpp"
#include "tools/traits.hpp"
///

/// FDCT::FDCT
FDCT::FDCT(class Environ *env)
  : DCT(env)
{
}
///

/// FDCT::~FDCT
FDCT::~FDCT(void)
{
}
///

/// FDCT::DefineQuant
void FDCT::DefineQuant(const UWORD *table)
{
  FLOAT *itarget = m_pfInvQuant;
  FLOAT *target  = m_pfQuant;
  int x,y;
  
  static const double dctscale[8] = {
    1.0, 1.387039845, 1.306562965, 1.175875602,
    1.0, 0.785694958, 0.541196100, 0.275899379
  };

  for(y = 0;y < 8;y++) {
    for(x = 0;x < 8;x++) {
      *itarget = 0.125 / (*table * dctscale[x] * dctscale[y]);
      *target  = 0.125 * *table * dctscale[x] * dctscale[y];
      target++,itarget++,table++;
    }
  }
}
///

/// FDCT::TransformBlock
// Run the DCT on a 8x8 block on the input data, giving the output table.
void FDCT::TransformBlock(const LONG *source,LONG *target,LONG offset)
{  
  FLOAT d[64];
  FLOAT *dp    = d;
  FLOAT *dpend = d + 64;
  LONG *tend          = target + 8;
  const FLOAT *qp     = m_pfInvQuant;
  //
  // DCT scaling;
  FLOAT dcoffset = offset * 64.0;
  //
  // Process rows.
  for(dp = d;dp < dpend;dp += 8,source += 8) {
    FLOAT tmp0  = source[0] + source[7];
    FLOAT tmp7  = source[0] - source[7];
    FLOAT tmp1  = source[1] + source[6];
    FLOAT tmp6  = source[1] - source[6];
    FLOAT tmp2  = source[2] + source[5];
    FLOAT tmp5  = source[2] - source[5];
    FLOAT tmp3  = source[3] + source[4];
    FLOAT tmp4  = source[3] - source[4];

    // Phase 2
    FLOAT tmp10 = tmp0 + tmp3;
    FLOAT tmp13 = tmp0 - tmp3;
    FLOAT tmp11 = tmp1 + tmp2;
    FLOAT tmp12 = tmp1 - tmp2;

    // Phase 3
    dp[0]       = tmp10 + tmp11; // the DC part.
    dp[4]       = tmp10 - tmp11;

    // Phase 4
    FLOAT z1    = (tmp12 + tmp13) * 0.707106781;

    // Phase 5
    dp[2]       = tmp13 + z1;
    dp[6]       = tmp13 - z1;

    // Odd part
    tmp10       = tmp4 + tmp5;
    tmp11       = tmp5 + tmp6;
    tmp12       = tmp6 + tmp7;

    FLOAT z5 = (tmp10 - tmp12) * 0.382683433;
    FLOAT z2 = 0.541196100 * tmp10 + z5;
    FLOAT z4 = 1.306562965 * tmp12 + z5;
    FLOAT z3 = tmp11 * 0.707106781;
    
    FLOAT z11 = tmp7 + z3;
    FLOAT z13 = tmp7 - z3;

    dp[5] = z13 + z2;
    dp[3] = z13 - z2;
    dp[1] = z11 + z4;
    dp[7] = z11 - z4;
  }

  // Process columns
  for (dp = d;target < tend;dp++,target++,qp++) {
    FLOAT tmp0 = dp[0 << 3] + dp[7 << 3];
    FLOAT tmp7 = dp[0 << 3] - dp[7 << 3];
    FLOAT tmp1 = dp[1 << 3] + dp[6 << 3];
    FLOAT tmp6 = dp[1 << 3] - dp[6 << 3];
    FLOAT tmp2 = dp[2 << 3] + dp[5 << 3];
    FLOAT tmp5 = dp[2 << 3] - dp[5 << 3];
    FLOAT tmp3 = dp[3 << 3] + dp[4 << 3];
    FLOAT tmp4 = dp[3 << 3] - dp[4 << 3];

    // Phase 2
    FLOAT tmp10 = tmp0 + tmp3;
    FLOAT tmp13 = tmp0 - tmp3;
    FLOAT tmp11 = tmp1 + tmp2;
    FLOAT tmp12 = tmp1 - tmp2;

    // Phase 3
    target[0 << 3] = Quantize(tmp10 + tmp11 - ((dp == d)?(dcoffset):(0.0)),qp[0 << 3]);
    target[4 << 3] = Quantize(tmp10 - tmp11,qp[4 << 3]);

    // Phase 4
    FLOAT z1 = (tmp12 + tmp13) * 0.707106781;

    // Phase 5
    target[2 << 3] = Quantize(tmp13 + z1,qp[2 << 3]);
    target[6 << 3] = Quantize(tmp13 - z1,qp[6 << 3]);

    // Odd part
    tmp10 = tmp4 + tmp5;
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    FLOAT z5  = (tmp10 - tmp12) * 0.382683433;
    FLOAT z2  = 0.541196100 * tmp10 + z5;
    FLOAT z4  = 1.306562965 * tmp12 + z5;
    FLOAT z3  = tmp11 * 0.707106781;
    FLOAT z11 = tmp7 + z3;
    FLOAT z13 = tmp7 - z3;

    target[5 << 3] = Quantize(z13 + z2,qp[5 << 3]);
    target[3 << 3] = Quantize(z13 - z2,qp[3 << 3]);
    target[1 << 3] = Quantize(z11 + z4,qp[1 << 3]);
    target[7 << 3] = Quantize(z11 - z4,qp[7 << 3]);
  }
}
///

/// FDCT::InverseTransformBlock
// Run the inverse DCT on an 8x8 block reconstructing the data.
void FDCT::InverseTransformBlock(LONG *target,const LONG *source,LONG offset)
{ 
  const FLOAT *qnt = m_pfQuant;
  FLOAT d[64];
  FLOAT *dptr = d;
  FLOAT *dend = d + 8;
  FLOAT dcoffset = offset;

  if (source) {
    for(;dptr < dend;dptr++,source++,qnt++) {
      FLOAT tmp0 = source[0 << 3] * qnt[0 << 3] + ((dptr == d)?(dcoffset):(0.0));
      FLOAT tmp1 = source[2 << 3] * qnt[2 << 3];    
      FLOAT tmp2 = source[4 << 3] * qnt[4 << 3];
      FLOAT tmp3 = source[6 << 3] * qnt[6 << 3];
      
      FLOAT tmp10 = tmp0 + tmp2;	
      FLOAT tmp11 = tmp0 - tmp2;
      
      FLOAT tmp13 = tmp1 + tmp3;
      FLOAT tmp12 = (tmp1 - tmp3) * 1.414213562 - tmp13;
      
      tmp0 = tmp10 + tmp13;
      tmp3 = tmp10 - tmp13;
      tmp1 = tmp11 + tmp12;
      tmp2 = tmp11 - tmp12;
      
      FLOAT tmp4 = source[1 << 3] * qnt[1 << 3];
      FLOAT tmp5 = source[3 << 3] * qnt[3 << 3];
      FLOAT tmp6 = source[5 << 3] * qnt[5 << 3];
      FLOAT tmp7 = source[7 << 3] * qnt[7 << 3];
      
      FLOAT z13 = tmp6 + tmp5;		
      FLOAT z10 = tmp6 - tmp5;
      FLOAT z11 = tmp4 + tmp7;
      FLOAT z12 = tmp4 - tmp7;
      
      tmp7  = z11 + z13;		
      tmp11 = (z11 - z13) * 1.414213562;
      FLOAT z5 = (z10 + z12) * 1.847759065;
      tmp10 = 1.082392200 * z12 - z5;
      tmp12 = -2.613125930 * z10 + z5;
      
      tmp6 = tmp12 - tmp7;
      tmp5 = tmp11 - tmp6;
      tmp4 = tmp10 + tmp5;
      
      dptr[0 << 3] = tmp0 + tmp7;
      dptr[7 << 3] = tmp0 - tmp7;
      dptr[1 << 3] = tmp1 + tmp6;
      dptr[6 << 3] = tmp1 - tmp6;
      dptr[2 << 3] = tmp2 + tmp5;
      dptr[5 << 3] = tmp2 - tmp5;
      dptr[4 << 3] = tmp3 + tmp4;
      dptr[3 << 3] = tmp3 - tmp4;
    }
    
    // After transforming over the columns, now transform over the rows.
    dptr = d;dend = d + 8*8;
    for(;dptr < dend;dptr+=8,target+=8) {
      FLOAT tmp0 = dptr[0];
      FLOAT tmp1 = dptr[2];
      FLOAT tmp2 = dptr[4];
      FLOAT tmp3 = dptr[6];
      
      FLOAT tmp10 = tmp0 + tmp2;	
      FLOAT tmp11 = tmp0 - tmp2;
      
      FLOAT tmp13 = tmp1 + tmp3;
      FLOAT tmp12 = (tmp1 - tmp3) * 1.414213562 - tmp13;
      
      tmp0 = tmp10 + tmp13;
      tmp3 = tmp10 - tmp13;
      tmp1 = tmp11 + tmp12;
      tmp2 = tmp11 - tmp12;
      
      FLOAT tmp4 = dptr[1];
      FLOAT tmp5 = dptr[3];
      FLOAT tmp6 = dptr[5];
      FLOAT tmp7 = dptr[7];
      
      FLOAT z13 = tmp6 + tmp5;		
      FLOAT z10 = tmp6 - tmp5;
      FLOAT z11 = tmp4 + tmp7;
      FLOAT z12 = tmp4 - tmp7;
      
      tmp7  = z11 + z13;		
      tmp11 = (z11 - z13) * 1.414213562;
      FLOAT z5 = (z10 + z12) * 1.847759065;
      tmp10 = 1.082392200 * z12 - z5;
      tmp12 = -2.613125930 * z10 + z5;
      
      tmp6 = tmp12 - tmp7;
      tmp5 = tmp11 - tmp6;
      tmp4 = tmp10 + tmp5;
      
      target[0] = tmp0 + tmp7;
      target[7] = tmp0 - tmp7;
      target[1] = tmp1 + tmp6;
      target[6] = tmp1 - tmp6;
      target[2] = tmp2 + tmp5;
      target[5] = tmp2 - tmp5;
      target[4] = tmp3 + tmp4;
      target[3] = tmp3 - tmp4;
    }
  } else {
    memset(target,0,sizeof(LONG) * 64);
  }
}
///
