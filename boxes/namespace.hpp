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
** This class keeps the namespaces together and finds boxes according
** to the priorities defined in the standard.
**
** $Id: namespace.hpp,v 1.5 2024/03/25 18:42:06 thor Exp $
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
    assert(m_ppPrimaryList == NULL || boxlist == m_ppPrimaryList);
    m_ppPrimaryList = boxlist;
  }
  //
  // Define the secondary lookup namespace.
  void DefineSecondaryLookup(class Box **boxlist)
  {
    assert(m_ppSecondaryList == NULL || boxlist == m_ppSecondaryList);
    m_ppSecondaryList = boxlist;
  }
  //
  // Check whether the primary namespace (aka merging spec box)
  // is already present.
  bool hasPrimaryLookup(void) const
  {
    return (m_ppPrimaryList != NULL)?true:false;
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
