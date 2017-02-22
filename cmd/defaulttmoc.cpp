/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** This file provides a simple TMO that provides fine quality and natural
** look in most cases. It is mostly a global Reinhard operator:
** Erik Reinhard and Kate Devlin. Dynamic Range Reduction Inspired by
** Photoreceptor Physiology.  IEEE Transactions on Visualization and
** Computer Graphics (2004).
**
** This is the default TMO for profile C.
**
** $Id: defaulttmoc.cpp,v 1.4 2015/01/20 21:51:49 thor Exp $
**
*/

/// Includes
#include "cmd/defaulttmoc.hpp"
#include "cmd/iohelpers.hpp"
#include "std/stdio.hpp"
#include "std/math.hpp"
///

/// BuildToneMapping_C
// Make a simple attempt to find a reasonable tone mapping from HDR to LDR.
// This is by no means ideal, but seem to work well in most cases.
// The algorithm used here is a simplified version of the exrpptm tone mapper,
// found in the following paper:
// Erik Reinhard and Kate Devlin. Dynamic Range Reduction Inspired by
// Photoreceptor Physiology.  IEEE Transactions on Visualization and
// Computer Graphics (2004).
void BuildToneMapping_C(FILE *in,int w,int h,int depth,int count,UWORD tonemapping[65536],
                        bool flt,bool bigendian,bool xyz,int hiddenbits)
{
  long pos    = ftell(in);
  int x,y,i;
  int maxin   = 256 << hiddenbits;
  double max  = (1 << depth) - 1;
  double lav  = 0.0;
  double llav = 0.0;
  double minl = HUGE_VAL;
  double maxl =-HUGE_VAL;
  double miny = HUGE_VAL;
  double maxy =-HUGE_VAL;
  double m;
  long cnt = 0;

  for(y = 0;y < h;y++) {
    for(x = 0;x < w;x++) {
      int r,g,b;
      double y;

      ReadRGBTriple(in,r,g,b,y,depth,count,flt,bigendian,xyz);

      if (y > 0.0) {
        double logy = log(y);
        lav  += y;
        llav += logy;
        if (logy < minl)
          minl = logy;
        if (logy > maxl)
          maxl = logy;
        if (y < miny)
          miny = y;
        if (y > maxy)
          maxy = y;
        cnt++;
      }
    }
  }

  lav  /= cnt;
  llav /= cnt;
  if (maxl <= minl) {
    m   = 0.3;
  } else {
    double k = (maxl - llav) / (maxl - minl);
    if (k > 0.0) {
      m  = 0.3 + 0.7 * pow(k,1.4);
    } else {
      m  = 0.3;
    }
  }

  fseek(in,pos,SEEK_SET);

  for(i = 0;i < maxin;i++) {
    if (flt) {
      double out = i / double(maxin);
      double in  = pow(pow(lav,m) * out / (1.0 - out),2.2);
      if (in < 0.0) in = 0.0;
      
      tonemapping[i] = DoubleToHalf(in);
    } else {
      double out = i / double(maxin);
      double in  = max * (miny + (maxy - miny) * pow(lav,m) * out / (1.0 - out));
      if (in < 0.0) in = 0.0;
      if (in > max) in = max;
      
      tonemapping[i] = in;
    }
  }
}
///
