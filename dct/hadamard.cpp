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
** Very low-complexity Hadamard-Transform for the residuals.
**
** $Id: hadamard.cpp,v 1.1 2012-07-24 19:36:45 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "std/string.hpp"
#include "dct/hadamard.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/imagebitmap.hpp"
#include "colortrafo/colortrafo.hpp"
///

/// Hadamard::Hadamard
Hadamard::Hadamard(class Environ *env)
  : DCT(env)
{
}
///

/// Hadamard::~Hadamard
Hadamard::~Hadamard(void)
{
}
///

/// Hadamard::DefineQuant
void Hadamard::DefineQuant(const UWORD *table)
{
  int i;

  // No scaling required here.
  for(i = 0;i < 64;i++) {
    m_psQuant[i]    = table[i];
    m_plInvQuant[i] = LONG(FLOAT(1L << QUANTIZER_BITS) / table[i] + 0.5);
  }
}
///

/// Hadamard::TransformBlock
// Run the Hadamard Transform on a 8x8 block on the input data, giving the output table.
void Hadamard::TransformBlock(const LONG *source,LONG *target,LONG dcoffset)
{ 
  LONG *dpend,*dp;
  const LONG *qp = m_plInvQuant; 
  //
  dcoffset <<= 3;
  //
  // Pass over rows.
  dpend = target + 64;
  for(dp = target;dp < dpend;dp += 8,source += 8) {
    LONG a0    = source[0] + source[4];
    LONG a1    = source[1] + source[5];
    LONG a2    = source[2] + source[6];
    LONG a3    = source[3] + source[7];
    LONG a4    = (a0 >> 1) - source[4];
    LONG a5    = (a1 >> 1) - source[5];
    LONG a6    = (a2 >> 1) - source[6];
    LONG a7    = (a3 >> 1) - source[7];

    LONG b0    = a0 + a2;
    LONG b1    = a1 + a3;
    LONG b2    = (b0 >> 1) - a2;
    LONG b3    = (b1 >> 1) - a3;
    LONG b4    = a4 + a6;
    LONG b5    = a5 + a7;
    LONG b6    = (b4 >> 1) - a6;
    LONG b7    = (b5 >> 1) - a7;
    
    LONG c0    = b0 + b1;
    LONG c1    = (c0 >> 1) - b1;
    LONG c2    = b2 + b3;
    LONG c3    = (c2 >> 1) - b3;
    LONG c4    = b4 + b5;
    LONG c5    = (c4 >> 1) - b5;
    LONG c6    = b6 + b7;
    LONG c7    = (c6 >> 1) - b7;

    dp[0]      = c0; // low pass
    dp[1]      = c4; // next higher
    dp[2]      = c6;
    dp[3]      = c2;
    dp[4]      = c3;
    dp[5]      = c7;
    dp[6]      = c5;
    dp[7]      = c1;
  }
  //
  // Pass over columns and quantize.
  dpend = target + 8;
  for(dp = target;dp < dpend;dp++,qp++) { 
    LONG a0    = dp[0 << 3]  + dp[4 << 3];
    LONG a1    = dp[1 << 3]  + dp[5 << 3];
    LONG a2    = dp[2 << 3]  + dp[6 << 3];
    LONG a3    = dp[3 << 3]  + dp[7 << 3];
    LONG a4    = (a0 >> 1) - dp[4 << 3];
    LONG a5    = (a1 >> 1) - dp[5 << 3];
    LONG a6    = (a2 >> 1) - dp[6 << 3];
    LONG a7    = (a3 >> 1) - dp[7 << 3];

    LONG b0    = a0 + a2;
    LONG b1    = a1 + a3;
    LONG b2    = (b0 >> 1) - a2;
    LONG b3    = (b1 >> 1) - a3;
    LONG b4    = a4 + a6;
    LONG b5    = a5 + a7;
    LONG b6    = (b4 >> 1) - a6;
    LONG b7    = (b5 >> 1) - a7;
    
    LONG c0    = b0 + b1;
    LONG c1    = (c0 >> 1) - b1;
    LONG c2    = b2 + b3;
    LONG c3    = (c2 >> 1) - b3;
    LONG c4    = b4 + b5;
    LONG c5    = (c4 >> 1) - b5;
    LONG c6    = b6 + b7;
    LONG c7    = (c6 >> 1) - b7;

    dp[0 << 3] = Quantize(c0 - dcoffset,qp[0 << 3]);
    dp[1 << 3] = Quantize(c4,qp[1 << 3]);
    dp[2 << 3] = Quantize(c6,qp[2 << 3]);
    dp[3 << 3] = Quantize(c2,qp[3 << 3]);
    dp[4 << 3] = Quantize(c3,qp[4 << 3]);
    dp[5 << 3] = Quantize(c7,qp[5 << 3]);
    dp[6 << 3] = Quantize(c5,qp[6 << 3]);
    dp[7 << 3] = Quantize(c1,qp[7 << 3]);
  }
}
///

/// Hadamard::InverseTransformBlock
// Run the inverse DCT on an 8x8 block reconstructing the data.
void Hadamard::InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset)
{
  LONG *dp;
  LONG *dend = target + 8;
  const WORD *qp = m_psQuant;

  dcoffset <<= 3;

  if (source) {
    for(dp = target;dp < dend;dp++,source++,qp++) {
      LONG c0 = source[0 << 3] * qp[0 << 3] + dcoffset;
      LONG c4 = source[1 << 3] * qp[1 << 3];
      LONG c6 = source[2 << 3] * qp[2 << 3];
      LONG c2 = source[3 << 3] * qp[3 << 3];
      LONG c3 = source[4 << 3] * qp[4 << 3];
      LONG c7 = source[5 << 3] * qp[5 << 3];
      LONG c5 = source[6 << 3] * qp[6 << 3];
      LONG c1 = source[7 << 3] * qp[7 << 3];

      LONG b1 = (c0 >> 1) - c1;
      LONG b0 = c0 - b1;
      LONG b3 = (c2 >> 1) - c3;
      LONG b2 = c2 - b3;
      LONG b5 = (c4 >> 1) - c5;
      LONG b4 = c4 - b5;
      LONG b7 = (c6 >> 1) - c7;
      LONG b6 = c6 - b7;

      LONG a2 = (b0 >> 1) - b2;
      LONG a3 = (b1 >> 1) - b3;
      LONG a0 = b0 - a2;
      LONG a1 = b1 - a3;
      LONG a6 = (b4 >> 1) - b6;
      LONG a7 = (b5 >> 1) - b7;
      LONG a4 = b4 - a6;
      LONG a5 = b5 - a7;
      
      dp[4 << 3] = (a0 >> 1) - a4;
      dp[5 << 3] = (a1 >> 1) - a5;
      dp[6 << 3] = (a2 >> 1) - a6;
      dp[7 << 3] = (a3 >> 1) - a7;
      dp[0 << 3] = a0 - dp[4 << 3];
      dp[1 << 3] = a1 - dp[5 << 3];
      dp[2 << 3] = a2 - dp[6 << 3];
      dp[3 << 3] = a3 - dp[7 << 3];

      dcoffset   = 0;
    }
    
    // After transforming over the columns, now transform over the rows.
    dend = target + 8*8;
    for(dp = target;dp < dend;dp+=8) {
      LONG c0 = dp[0];
      LONG c4 = dp[1];
      LONG c6 = dp[2];
      LONG c2 = dp[3];
      LONG c3 = dp[4];
      LONG c7 = dp[5];
      LONG c5 = dp[6];
      LONG c1 = dp[7];

      LONG b1 = (c0 >> 1) - c1;
      LONG b0 = c0 - b1;
      LONG b3 = (c2 >> 1) - c3;
      LONG b2 = c2 - b3;
      LONG b5 = (c4 >> 1) - c5;
      LONG b4 = c4 - b5;
      LONG b7 = (c6 >> 1) - c7;
      LONG b6 = c6 - b7;

      LONG a2 = (b0 >> 1) - b2;
      LONG a3 = (b1 >> 1) - b3;
      LONG a0 = b0 - a2;
      LONG a1 = b1 - a3;
      LONG a6 = (b4 >> 1) - b6;
      LONG a7 = (b5 >> 1) - b7;
      LONG a4 = b4 - a6;
      LONG a5 = b5 - a7;
      
      dp[4] = (a0 >> 1) - a4;
      dp[5] = (a1 >> 1) - a5;
      dp[6] = (a2 >> 1) - a6;
      dp[7] = (a3 >> 1) - a7;
      dp[0] = a0 - dp[4];
      dp[1] = a1 - dp[5];
      dp[2] = a2 - dp[6];
      dp[3] = a3 - dp[7];
    }
  } else {
    memset(target,0,sizeof(LONG) * 64);
  }
}
///

