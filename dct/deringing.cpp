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
** This class implements a simple deblocking filter to avoid
** DCT artifacts (Gibbs Phenomenon)
**
** $Id: deringing.cpp,v 1.2 2016/10/28 13:58:54 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/frame.hpp"
#include "dct/dct.hpp"
#include "dct/deringing.hpp"
#include "std/string.hpp"
///

/// DeRinger::DeRinger
// Create a new deblocker for a given frame and component.
DeRinger::DeRinger(class Frame *frame,class DCT *dct)
  : JKeeper(frame->EnvironOf()), m_pDCT(dct)
{ 
  int preshift = dct->PreshiftOf();
  //
  // Min and max are the minimum and maximum sample values that
  // would be reconstructed to the extreme values.
  m_lMin   = (1L << preshift) - 1;
  m_lMax   = ((1L << frame->HiddenPrecisionOf()) - 1) << preshift;
  m_lDelta = (1L << preshift);
}
///

/// DeRinger::~DeRinger
DeRinger::~DeRinger(void)
{
}
///

#if ACCUSOFT_CODE
/// DeRinger::Smooth
// Run a simple Gaussian smoothing filter on the second argument, place
// the result in the first argument, or copy from the original if mask is
// not set.
void DeRinger::Smooth(LONG target[64],const LONG src[64],const LONG mask[64])
{
  int x,y;

  for(y = 0;y < 8;y++) {
    for(x = 0;x < 8;x++) {
      if (mask[x + (y << 3)]) {
        LONG center = src[x                   + (y << 3)];
        LONG left   = src[((x > 0)?(x - 1):x) + (y << 3)];
        LONG right  = src[((x < 7)?(x + 1):x) + (y << 3)];
        LONG top    = src[x                   + (((y > 0)?(y - 1):(y)) << 3)];
        LONG bottom = src[x                   + (((y < 7)?(y + 1):(y)) << 3)];
        LONG round  = (center << 2) + left + right + top + bottom;
        //
        // Insert the average now.
        round = ((round | 1) + 3) >> 3;
        //
        // Make sure we do not smooth the pixels out in the wrong direction,
        // we must at least preserve minimum and maximum.
        if (center <= m_lMin && round > m_lMin) {
          round = m_lMin;
        } else if (center >= m_lMax && round < m_lMax) {
          round = m_lMax;
        } 
        target[x + (y << 3)] = round;
      } else {
        // Pixels except the minimum and maximum are not affected.
        target[x + (y << 3)] = src[x + (y << 3)];
      }
    }
  }
}
///
#endif  

/// DeRinger::DeRing
// Deblock the given image block (non-DCT-transformed) by including overshooting
// in the extreme image parts, or undershooting in the dark image regions.
#if ACCUSOFT_CODE 
void DeRinger::DeRing(const LONG block[64],LONG dst[64],LONG dcshift)
#else
void DeRinger::DeRing(const LONG [64],LONG [64],LONG)
#endif  
{
#if ACCUSOFT_CODE 
  int i,c1 = 0,c2 = 0;
  LONG mask[64]; // Is set to one if we can modify this coefficient.
  LONG delta = m_pDCT->BucketSizes()[0];
  LONG sum   = 0;
  if (delta < m_lDelta)
    delta = m_lDelta;
  //
  // Check how many samples are there that could require processing by this
  // filter. These are samples that are either below min or above max.
  for(i = 0;i < 64;i++) {
    LONG v = block[i];
    if (v <= m_lMin) {
      c1++;
      mask[i] = -1;
    } else if (v >= m_lMax) {
      c2++;
      mask[i] = +1;
    } else {
      mask[i] = 0;
    }
    sum += block[i];
  }
  m_pDCT->TransformBlock(block,dst,dcshift);
  //
  // If there are no extreme samples, this makes all no sense at all.
  if ((c1 > 0 && c1 < 64) || (c2 > 0 && c2 < 64)) {
    LONG mod[64],tmp[64];
    // Compute maximum overshoot levels for minimum and maximum
    // coefficients. First, do not increase by more than two
    // DC quantization buckets, but by a minimum level that ensures
    // that most artifacts are gone.
    LONG overshoot    = (delta << 1);
    LONG minovershoot = (31 << m_pDCT->PreshiftOf());
    if (overshoot < minovershoot)
      overshoot = minovershoot;
    // The maximum overshoot moves the average grey level just
    // to the maximum or minimum level.
    LONG max = ((m_lMax << 6) - sum) >> 6;
    LONG min = (sum - (m_lMin << 6)) >> 6;
    if (max > overshoot)
      max = overshoot;
    if (min > overshoot)
      min = overshoot;
    // Compute from the overshoot the maximum and minimum amplitude
    max = m_lMax + max;
    min = m_lMin - min;
    //
    // First increase the amplitude by all overshooting coefficients.
    for(i = 0;i < 64;i++) {
      if (mask[i] > 0) {
        mod[i] = max;
      } else if (mask[i] < 0) {
        mod[i] = min;
      } else {
        mod[i] = block[i];
      }
    }
    //
    // Now run a smoothing filter over those parts we modified to
    // improve compressibility. This is a simple Gaussian filter
    // that should hopefully remove high contrast edges due to the
    // large amplitudes, and thus the high frequencies.
    Smooth(tmp,mod,mask);
    Smooth(mod,tmp,mask);
    Smooth(tmp,mod,mask);
    //
    // Forward transform the smoothed data.
    m_pDCT->TransformBlock(tmp,mod,dcshift);
    //
    // Now copy the data back into the final DCT.
    // Try to cover the corrections as good as possible, but
    // avoid changing the magnitude category to keep the
    // rate increase minimal.
    for(i = 0;i < 64;i++) {
      LONG data = dst[i];
      LONG mdat = mod[i];
      if (i == 0) {
        dst[i] = mdat;
      } else if (data) {
        LONG max = 1;
        data     = (data < 0)?(-data):(data);
        while(data >>= 1) {
          max = (max << 1) | 1;
        }
        if (mdat > max) {
          dst[i] = max;
        } else if (mdat < -max) {
          dst[i] = -max;
        } else {
          dst[i] = mdat;
        }
      } else {
        dst[i] = data;
      }
    }
  }
#else
  JPG_THROW(NOT_IMPLEMENTED,"DeRinger::DeRing",
            "the de-ringing filter is not available in this software release");
#endif  
}
///

