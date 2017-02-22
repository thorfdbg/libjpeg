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
** This box keeps a linear matrix of unknown elements. It is a base class
** for either the fix or floating point transformation.
**
** $Id: matrixbox.hpp,v 1.3 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef BOXES_MATRIXBOX_HPP
#define BOXES_MATRIXBOX_HPP

/// Includes
#include "std/string.hpp"
#include "std/assert.hpp"
#include "boxes/box.hpp"
///

/// class MatrixBox
// This box keeps a linear transformation that can be used either as L
// or C transformation.
class MatrixBox : public Box {
  //
protected:
  //
  // The ID of this matrix.
  UBYTE m_ucID;
  //
  //
  // Set if the inverse matrix is valid. Otherwise, it must be computed.
  bool  m_bInverseValid;
  //
public:
  //
  // The constructor.
  MatrixBox(class Environ *env,class Box *&boxlist,ULONG type)
    : Box(env,boxlist,type), m_ucID(0), m_bInverseValid(false)
  { }
  //
  // Create a matrix from an ID and a coefficient list.
  MatrixBox(class Environ *env,class Box *&boxlist,ULONG type,UBYTE id)
    : Box(env,boxlist,type), m_ucID(id), m_bInverseValid(false)
  {
    assert(id >= 5 && id <= 15);
  }
  //
  virtual ~MatrixBox(void)
  { }
  //
  // Return the ID of this matrix. This is a number between 5 and 15.
  UBYTE IdOf(void) const
  {
    return m_ucID;
  }
};
///


///
#endif
