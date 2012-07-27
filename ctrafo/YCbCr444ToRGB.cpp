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
** Convert YCbCr to RGB, without subsampling. Includes output scaling and
** clipping.
**
** $Id: YCbCr444ToRGB.cpp,v 1.3 2012-06-02 10:27:14 thor Exp $
**
*/

/// Includes
#include "ctrafo/YCbCr444ToRGB.hpp"
///

/// YCbCr444TORGB::YCbCr444TORGB
YCbCr444TORGB::YCbCr444TORGB(class Environ *env)
  : JKeeper(env)
{
}
///

/// YCbCr444TORGB::~YCbCr444TORGB
YCbCr444TORGB::~YCbCr444TORGB(void)
{
}
///


/// YCbCr444TORGB::InverseTransform
void YCbCr444TORGB::InverseTransform(const FLOAT *y,const FLOAT *cb,const FLOAT *cr,
				     UBYTE *r,UBYTE *g,UBYTE *b,
				     ULONG modr,ULONG modg,ULONG modb,
				     int w,int h)
{
  int i,j;

  for(i = 0;i < h;i++) {
    UBYTE *rp  = r;
    UBYTE *gp  = g;
    UBYTE *bp  = b;
    const FLOAT *yp  = y;
    const FLOAT *cbp = cb;
    const FLOAT *crp = cr;

    for(j = 0;j < w;j++) {
      FLOAT rv  =  0.125 * (1.0f * *yp                   + 1.402f   * *crp) + 128.5;
      FLOAT gv  =  0.125 * (1.0f * *yp - 0.34413f * *cbp - 0.71414f * *crp) + 128.5;
      FLOAT bv  =  0.125 * (1.0f * *yp + 1.772f   * *cbp                  ) + 128.5;

      if (rv < 0) {
	*rp = 0;
      } else if (rv > 255) {
	*rp = 255;
      } else {
	*rp = rv;
      }
      
      if (gv < 0) {
	*gp = 0;
      } else if (gv > 255) {
	*gp = 255;
      } else {
	*gp = gv;
      }
      
      if (bv < 0) {
	*bp = 0;
      } else if (bv > 255) {
	*bp = 255;
      } else {
	*bp = bv;
      }
       
      rp++,gp++,bp++;
      yp++,cbp++,crp++;
    }
    r   += modr;
    g   += modg;
    b   += modb;
    yp  += 8;
    cbp += 8;
    crp += 8;
  }
}
///
