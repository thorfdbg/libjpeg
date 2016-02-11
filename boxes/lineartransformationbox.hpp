/*
** This box keeps a linear transformation that can be used either as L
** or C transformation.
**
** $Id: lineartransformationbox.hpp,v 1.5 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef BOXES_LINEARTRANSFORMATIONBOX_HPP
#define BOXES_LINEARTRANSFORMATIONBOX_HPP

/// Includes
#include "std/string.hpp"
#include "std/assert.hpp"
#include "boxes/box.hpp"
#include "boxes/matrixbox.hpp"
///

/// class LinearTransformationBox
// This box keeps a linear transformation that can be used either as L
// or C transformation.
class LinearTransformationBox : public MatrixBox {
  //
  // The linear transformation coefficients as fixpoint data with 13 preshifted
  // bits.
  LONG  m_lMatrix[9];
  //
  // The inverse matrix if it exists.
  LONG  m_lInverse[9];
  //
  // Compute the inverse matrix and put it into m_lInverse. If it does not
  // exist, throw.
  void InvertMatrix(void);
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize);
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  virtual bool CreateBoxContent(class MemoryStream *target);
  //
public:
  enum {
    Type = MAKE_ID('M','T','R','X')
  };
  //
  // The constructor.
  LinearTransformationBox(class Environ *env,class Box *&boxlist)
    : MatrixBox(env,boxlist,Type)
  { }
  //
  // Define a matrix from an ID and a coefficient list.
  void DefineMatrix(UBYTE id,const LONG *matrix)
  {
    m_ucID = id;
    memcpy(m_lMatrix,matrix,sizeof(m_lMatrix));
  }
  //
  virtual ~LinearTransformationBox(void)
  { }
  //
  // Return the matrix transformation of this box. The result is a pointer
  // to nine 32-bit constants.
  const LONG *MatrixOf(void) const
  {
    return m_lMatrix;
  }
  //
  // Return the inverse of the matrix. Throws if it does not exist.
  const LONG *InverseMatrixOf(void)
  {
    if (!m_bInverseValid)
      InvertMatrix();

    assert(m_bInverseValid);
    
    return m_lInverse;
  }
};
///


///
#endif
