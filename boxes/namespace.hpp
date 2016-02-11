/*
** This class keeps the namespaces together and finds boxes according
** to the priorities defined in the standard.
**
** $Id: namespace.hpp,v 1.4 2014/11/11 09:49:12 thor Exp $
**
*/

#ifndef BOXES_NAMESPACE_HPP
#define BOXES_NAMESPACE_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "boxes/parametrictonemappingbox.hpp"
///

/// Forwards
class Box;
class MatrixBox;
class ToneMapperBox;
///

/// class NameSpace
// This class keeps the namespaces together and finds boxes according
// to the priorities defined in the standard.
class NameSpace : public JKeeper {
  //
  // Pointers to box lists for searching objects.
  // These objects are not maintained here, the list is
  // just a list pointer.
  //
  // The primary search path for objects: This is the box list of the merging spec list.
  class Box **m_ppPrimaryList;
  //
  // The secondary lookup target. This is the global name space.
  class Box **m_ppSecondaryList;
  //
public:
  NameSpace(class Environ *env)
  : JKeeper(env), m_ppPrimaryList(NULL), m_ppSecondaryList(NULL)
  { }
  //
  ~NameSpace(void)
  { }
  //
  // Define the primary lookup namespace.
  void DefinePrimaryLookup(class Box **boxlist)
  {
    assert(m_ppPrimaryList == NULL);
    m_ppPrimaryList = boxlist;
  }
  //
  // Define the secondary lookup namespace.
  void DefineSecondaryLookup(class Box **boxlist)
  {
    assert(m_ppSecondaryList == NULL);
    m_ppSecondaryList = boxlist;
  }
  //
  // Find the tone mapping box of the given table index, or NULL
  // if this box is missing.
  class ToneMapperBox *FindNonlinearity(UBYTE tabidx) const;
  //
  // Find the transformation matrix of the given matrix index.
  class MatrixBox *FindMatrix(UBYTE idx) const;
  //
  // Check whether there is a duplicate nonlinearity of the given Id.
  bool isUniqueNonlinearity(UBYTE tabidx) const;
  //
  // Check whether there is a duplicate matrix of the given Id.
  bool isUniqueMatrix(UBYTE idx) const;
  //
  // Find a parametric curve box with the given parameters, or return NULL if such a box
  // does not yet exist.
  const class ParametricToneMappingBox *FindNonlinearity(ParametricToneMappingBox::CurveType type,
							 UBYTE rounding_mode,
							 FLOAT p1 = 0.0,FLOAT p2 = 0.0,
							 FLOAT p3 = 0.0,FLOAT p4 = 0.0) const;
  //
  // Allocate an ID for a nonlinarity.
  UBYTE AllocateNonlinearityID(void) const;
  //
  // Allocate an ID for a matrix
  UBYTE AllocateMatrixID(void) const;
};
///

///
#endif
