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
** This class represents multiple boxes that all contain four non-linear
** transformation indices by referencing to either a parametric curve
** or a lookup table. It is used for multiple boxes.
**
** $Id: nonlineartrafobox.hpp,v 1.3 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef BOXES_NONLINEARTRAFOBOX_HPP
#define BOXES_NONLINEARTRAFOBOX_HPP

/// Includes
#include "boxes/box.hpp"
#include "std/string.hpp"
///

/// class NonlinearTrafoBox
// This class represents multiple boxes that all contain four non-linear
// transformation indices by referencing to either a parametric curve
// or a lookup table. It is used for multiple boxes.
class NonlinearTrafoBox : public Box {
  //
  // This box consists of four fields, one index per non-linear transformation.
  // These are called td_i in the specs.
  UBYTE m_ucTrafoIndex[4];
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box. Returns true in case the contents is
  // parsed and the stream can go away.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize);
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  // Returns whether the box content is already complete and the stream
  // can go away.
  virtual bool CreateBoxContent(class MemoryStream *target);
  //
public:
  enum {
    Base_Type        = MAKE_ID('L','P','T','S'), // base non-linear transformation
    Residual_Type    = MAKE_ID('Q','P','T','S'), // residual non-linear transformation
    Base2_Type       = MAKE_ID('C','P','T','S'), // secondary base non-linear
    Residual2_Type   = MAKE_ID('R','P','T','S'), // secondary residual non-linear
    Prescaling_Type  = MAKE_ID('S','P','T','S'), // prescaling non-linearity
    Postscaling_Type = MAKE_ID('P','P','T','S'), // postscaling non-linearity
    ResidualI_Type   = MAKE_ID('D','P','T','S')  // residual intermediate non-linearity
  };
  //
  // Create a non-linear transformation box. This also requires a type since there are
  // multiple boxes that all share the same syntax.
  NonlinearTrafoBox(class Environ *env,class Box *&boxlist,ULONG type)
    : Box(env,boxlist,type)
  { 
    memset(m_ucTrafoIndex,0,sizeof(m_ucTrafoIndex));
  }
  //
  virtual ~NonlinearTrafoBox(void)
  { }
  //
  // Return the index of the non-linear transformation for component i, i = 0..3.
  // The fourth field is currently reserved and unused.
  UBYTE TransformationIndexOf(UBYTE comp) const
  {
    assert(comp <= 3);

    return m_ucTrafoIndex[comp];
  }
  //
  // Define the transformation index of component comp.
  void DefineTransformationIndex(UBYTE comp,UBYTE idx)
  {
    assert(comp <= 3);
    assert(idx <= 15);

    m_ucTrafoIndex[comp] = idx;
  }
};
///

///
#endif

    
