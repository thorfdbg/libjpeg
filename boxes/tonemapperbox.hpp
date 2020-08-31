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
** This abstract box keeps the data for any type of tone mapping. It can
** be substituted by an inverse tone mapping box or an
** inverse parametric tone mapping box.
**
** $Id: tonemapperbox.hpp,v 1.11 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef BOXES_TONEMAPPERBOX_HPP
#define BOXES_TONEMAPPERBOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// class ToneMapperBox
// This abstract box keeps the data for any type of tone mapping. It can
// be substituted by an inverse tone mapping box or an
// inverse parametric tone mapping box.
class ToneMapperBox : public Box {
  //
protected:
  // Number of entries in this table
  ULONG m_ulTableEntries;
  //
  // The table index used to address tone mapping boxes.
  UBYTE m_ucTableIndex;
  //
  // Nothing private here.
public:
  //
  // A pass-through constructor.
  ToneMapperBox(class Environ *env,class Box *&boxlist,ULONG type)
    : Box(env,boxlist,type), m_ulTableEntries(0)
  { }
  //
  virtual ~ToneMapperBox(void)
  { }
  //
  // Return the size of the table in entries.
  ULONG EntriesOf(void) const
  {
    return m_ulTableEntries;
  }
  // 
  // Return the destination table index.
  UBYTE TableDestinationOf(void) const
  {
    return m_ucTableIndex;
  } 
  //
  // Return a table that maps inputs in the range 0..2^inputbits-1
  // to output bits in the range 0..2^oututbits-1, with additional
  // "frac" fractional bits. This is zero for int to int scaling as
  // for the L-transformation, but non-zero for RCT-output or color
  // transformed output as required for R and S.
  virtual const LONG *ScaledTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract) = 0;
  //
  // This is the floating point version of the above. It returns floating point sample
  // values instead of integer sample values. This is required for the floating point 
  // workflow.
  virtual const FLOAT *FloatTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract) = 0;
  //
  // Return the inverse of the table, where the first argument is the number
  // of bits in the DCT domain (the output bits) and the second argument is
  // the number of bits in the spatial (image) domain, i.e. the argument
  // order is identical to that of the backwards table generated above.
  virtual const LONG *InverseScaledTableOf(UBYTE dctbits,UBYTE spatialbits,UBYTE infract,UBYTE outfract) = 0;
};
///

///
#endif
