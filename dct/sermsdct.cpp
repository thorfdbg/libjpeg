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
** Integer to integer DCT operation plus scaled quantization.
**
** $Id: sermsdct.cpp,v 1.7 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "dct/sermsdct.hpp"
#include "std/string.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "interface/types.hpp"
#include "tools/traits.hpp"
///

/// Defines
// Number of fractional bits.
#define F(x) WORD((x * (1UL << FIX_BITS)) + 0.5)
// Actually, this is also simply a long, but prescaled by FIX_BITS
typedef LONG FIXED;

// Backshift from FIX_BITS
#define FIXED_TO_INT(x) (((x) + (1L << (FIX_BITS - 1))) >> (FIX_BITS))
///

/// SERMSDCT::SERMSDCT
SERMSDCT::SERMSDCT(class Environ *env)
  : DCT(env)
{
}
///

/// SERMSDCT::~SERMSDCT
SERMSDCT::~SERMSDCT(void)
{
}
///

/// SERMSDCT::DefineQuant
void SERMSDCT::DefineQuant(const UWORD *table)
{
  int i;

  // No scaling required here.
  for(i = 0;i < 64;i++) {
    m_psQuant[i]    = table[i];
    m_plInvQuant[i] = LONG(FLOAT(1L << QUANTIZER_BITS) / table[i] + 0.5);
  }
}
///


/// SERMSDCT::FwdSerms
// The forward lifting on a single row or column.
void SERMSDCT::FwdSerms(LONG &d0,LONG &d1,LONG &d2,LONG &d3,LONG &d4,LONG &d5,LONG &d6,LONG &d7)
{
  // Forward permutation.
  LONG x0 = d2;
  LONG x1 = d7;
  LONG x2 = d4;
  LONG x3 = d3;
  LONG x4 = d6;
  LONG x5 = d0;
  LONG x6 = d1;
  LONG x7 = d5;

  x7 = FIXED_TO_INT(x0*F(1.1648)+x1*-F(2.8234)+x2*F(0.5375)+x3*-F(0.6058)+x4*F(1.2228)+x5*-F(0.3805)+x6*F(0.0288)) - x7;
  x0+= FIXED_TO_INT(x1*-F(1.1129)+x2*F(0.0570)+x3*-F(0.4712)+x4*F(0.1029)+x5*F(0.0156)+x6*-F(0.4486)+x7*-F(0.4619));
  x1+= FIXED_TO_INT(x0*-F(0.0685)+x2*F(0.2708)+x3*-F(0.2708)+x4*-F(0.2235)+x5*F(0.2568)+x6*-F(0.3205)+x7*F(0.3841));
  x2+= FIXED_TO_INT(x0*-F(0.0364)+x1*-F(1.7104)+x3*-F(1.0000)+x4*F(0.3066)+x5*F(0.6671)+x6*-F(0.5953)+x7*F(0.2039));
  x3+= FIXED_TO_INT(x0*F(0.7957)+x1*F(0.9664)+x2*F(0.4439)+x4*F(0.6173)+x5*-F(0.1422)+x6*F(1.0378)+x7*-F(0.1700));
  x4+= FIXED_TO_INT(x0*F(0.4591)+x1*F(0.4108)+x2*-F(0.2073)+x3*-F(1.0824)+x5*F(0.7071)+x6*F(0.8873)+x7*-F(0.2517));
  x5+= FIXED_TO_INT(x0*-F(0.6573)+x1*F(0.5810)+x2*-F(0.2931)+x3*-F(0.5307)+x4*-F(0.8730)+x6*-F(0.1594)+x7*-F(0.3560));
  x6+= FIXED_TO_INT(x0*F(1.0024)+x1*-F(0.7180)+x2*-F(0.0928)+x3*-F(0.0318)+x4*F(0.4170)+x5*F(1.1665)+x7*F(0.4904));
  x7+= FIXED_TO_INT(x0*F(1.1020)+x1*-F(2.0306)+x2*-F(0.3881)+x3*F(0.6561)+x4*F(1.2405)+x5*F(1.6577)+x6*-F(1.1914));

  // Output permutation
  d0 = x3;
  d1 = x6;
  d2 = x4;
  d3 = x2;
  d4 = x5;
  d5 = x7;
  d6 = x0;
  d7 = x1;
}
///

/// SERMSDCT::InvSerms
// The forward lifting on a single row or column.
void SERMSDCT::InvSerms(LONG &d0,LONG &d1,LONG &d2,LONG &d3,LONG &d4,LONG &d5,LONG &d6,LONG &d7)
{
  LONG x0 = d6;
  LONG x1 = d7;
  LONG x2 = d3;
  LONG x3 = d0;
  LONG x4 = d2;
  LONG x5 = d4;
  LONG x6 = d1;
  LONG x7 = d5;
  
  x7-= FIXED_TO_INT(x0*F(1.1020)+x1*-F(2.0306)+x2*-F(0.3881)+x3*F(0.6561)+x4*F(1.2405)+x5*F(1.6577)+x6*-F(1.1914));
  x6-= FIXED_TO_INT(x0*F(1.0024)+x1*-F(0.7180)+x2*-F(0.0928)+x3*-F(0.0318)+x4*F(0.4170)+x5*F(1.1665)+x7*F(0.4904));
  x5-= FIXED_TO_INT(x0*-F(0.6573)+x1*F(0.5810)+x2*-F(0.2931)+x3*-F(0.5307)+x4*-F(0.8730)+x6*-F(0.1594)+x7*-F(0.3560));
  x4-= FIXED_TO_INT(x0*F(0.4591)+x1*F(0.4108)+x2*-F(0.2073)+x3*-F(1.0824)+x5*F(0.7071)+x6*F(0.8873)+x7*-F(0.2517));
  x3-= FIXED_TO_INT(x0*F(0.7957)+x1*F(0.9664)+x2*F(0.4439)+x4*F(0.6173)+x5*-F(0.1422)+x6*F(1.0378)+x7*-F(0.1700));
  x2-= FIXED_TO_INT(x0*-F(0.0364)+x1*-F(1.7104)+x3*-F(1.0000)+x4*F(0.3066)+x5*F(0.6671)+x6*-F(0.5953)+x7*F(0.2039));
  x1-= FIXED_TO_INT(x0*-F(0.0685)+x2*F(0.2708)+x3*-F(0.2708)+x4*-F(0.2235)+x5*F(0.2568)+x6*-F(0.3205)+x7*F(0.3841));
  x0-= FIXED_TO_INT(x1*-F(1.1129)+x2*F(0.0570)+x3*-F(0.4712)+x4*F(0.1029)+x5*F(0.0156)+x6*-F(0.4486)+x7*-F(0.4619));
  x7 = FIXED_TO_INT(x0*F(1.1648)+x1*-F(2.8234)+x2*F(0.5375)+x3*-F(0.6058)+x4*F(1.2228)+x5*-F(0.3805)+x6*F(0.0288)) - x7;

  d0 = x5;
  d1 = x6;
  d2 = x0;
  d3 = x3;
  d4 = x2;
  d5 = x7;
  d6 = x4;
  d7 = x1;
}
///

/// SERMSDCT::TransformBlock
// Run the DCT on a 8x8 block on the input data, giving the output table.
void SERMSDCT::TransformBlock(const LONG *source,LONG *target,LONG dcoffset)
{
  LONG *dpend,*dp;
  const LONG *qp = m_plInvQuant;
  //
  dcoffset <<= 3;
  // Pass over rows.
  dpend = target + 64;
  for(dp = target;dp < dpend;dp += 8,source += 8) {
    dp[0] = source[0];
    dp[1] = source[1];
    dp[2] = source[2];
    dp[3] = source[3];
    dp[4] = source[4];
    dp[5] = source[5];
    dp[6] = source[6];
    dp[7] = source[7];
    FwdSerms(dp[0],dp[1],dp[2],dp[3],dp[4],dp[5],dp[6],dp[7]);
  }

  // Pass over columns and quantize.
  dpend = target + 8;
  for(dp = target;dp < dpend;dp++,target++,qp++) {
    FwdSerms(dp[0 << 3],dp[1 << 3],dp[2 << 3],dp[3 << 3],dp[4 << 3],dp[5 << 3],dp[6 << 3],dp[7 << 3]);
    target[0 << 3] = Quantize(dp[0 << 3] - dcoffset,qp[0 << 3]);
    target[1 << 3] = Quantize(dp[1 << 3],qp[1 << 3]);
    target[2 << 3] = Quantize(dp[2 << 3],qp[2 << 3]);
    target[3 << 3] = Quantize(dp[3 << 3],qp[3 << 3]);
    target[4 << 3] = Quantize(dp[4 << 3],qp[4 << 3]);
    target[5 << 3] = Quantize(dp[5 << 3],qp[5 << 3]);
    target[6 << 3] = Quantize(dp[6 << 3],qp[6 << 3]);
    target[7 << 3] = Quantize(dp[7 << 3],qp[7 << 3]);
    dcoffset = 0;
  }
}
///

/// SERMSDCT::InverseTransformBlock
// Run the inverse DCT on an 8x8 block reconstructing the data.
void SERMSDCT::InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset)
{
  LONG *dp;
  LONG *dend = target + 8;
  const WORD *qnt = m_psQuant;

  dcoffset <<= 3;
  
  if (source) {
    for(dp = target;dp < dend;dp++,source++,qnt++) {
      dp[0 << 3] = source[0 << 3] * qnt[0 << 3] + dcoffset;
      dp[1 << 3] = source[1 << 3] * qnt[1 << 3];
      dp[2 << 3] = source[2 << 3] * qnt[2 << 3];
      dp[3 << 3] = source[3 << 3] * qnt[3 << 3];
      dp[4 << 3] = source[4 << 3] * qnt[4 << 3];
      dp[5 << 3] = source[5 << 3] * qnt[5 << 3];
      dp[6 << 3] = source[6 << 3] * qnt[6 << 3];
      dp[7 << 3] = source[7 << 3] * qnt[7 << 3];
      InvSerms(dp[0 << 3],dp[1 << 3],dp[2 << 3],dp[3 << 3],dp[4 << 3],dp[5 << 3],dp[6 << 3],dp[7 << 3]);
      dcoffset   = 0;
    }
    
    // After transforming over the columns, now transform over the rows.
    dend = target + 8*8;
    for(dp = target;dp < dend;dp+=8) {
      InvSerms(dp[0],dp[1],dp[2],dp[3],dp[4],dp[5],dp[6],dp[7]);
    }
  } else {
    memset(target,0,sizeof(LONG) * 64);
  }
}
///
