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
** This class implements a simple deringing filter to avoid
** DCT artifacts (Gibbs Phenomenon).
**
** $Id: deringing.hpp,v 1.2 2016/10/28 13:58:54 thor Exp $
**
*/

#ifndef DCT_DERINGING_HPP
#define DCT_DERINGING_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class Frame;
class DCT;
///

/// class DeRinger
// This implements a deringing filter on top of a DCT
class DeRinger : public JKeeper {
  //
  // The DCT for the transformation.
  class DCT *m_pDCT;
  //
  // Maximum and minimum values for the current frame.
  LONG m_lMin;
  LONG m_lMax;
  LONG m_lDelta;
  //
#if ACCUSOFT_CODE
  // Run a simple Gaussian smoothing filter on the second argument, place
  // the result in the first argument, or copy from the original if mask is
  // not set.
  void Smooth(LONG target[64],const LONG src[64],const LONG mask[64]);
#endif  
  //
  //
public:
  DeRinger(class Frame *frame,class DCT *dct);
  //
  ~DeRinger(void);
  //
  // Remove Gibbs' pheonomenon artifacts from the given image block 
  // (non-DCT-transformed) by including overshooting
  // in the extreme image parts, or undershooting in the dark image regions.
  void DeRing(const LONG block[64],LONG dst[64],LONG dcshift);
  //
};
///

///
#endif
