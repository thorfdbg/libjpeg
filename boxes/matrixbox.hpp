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
