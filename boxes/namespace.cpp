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
** $Id: namespace.cpp,v 1.4 2014/11/11 09:49:11 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "boxes/namespace.hpp"
#include "boxes/matrixbox.hpp"
#include "boxes/tonemapperbox.hpp"
#include "boxes/mergingspecbox.hpp"
///

/// NameSpace::FindNonlinearity
// Find the tone mapping box of the given table index, or NULL
// if this box is missing.
class ToneMapperBox *NameSpace::FindNonlinearity(UBYTE tabidx) const
{
  class Box *box;

  if (m_ppPrimaryList) {
    box = *m_ppPrimaryList;
    while(box) {
      class ToneMapperBox *tmo = dynamic_cast<ToneMapperBox *>(box);
      if (tmo) {
        if (tmo->TableDestinationOf() == tabidx)
          return tmo;
      }
      box = box->NextOf();
    }
  }

  if (m_ppSecondaryList) {
    box = *m_ppSecondaryList;
    while(box) {
      class ToneMapperBox *tmo = dynamic_cast<ToneMapperBox *>(box);
      if (tmo) {
        if (tmo->TableDestinationOf() == tabidx)
          return tmo;
      }
      box = box->NextOf();
    }
  }

  return NULL;
}
///

/// NameSpace::FindMatrix
// Find the transformation matrix of the given matrix index.
class MatrixBox *NameSpace::FindMatrix(UBYTE idx) const
{
  class Box *box;

  if (m_ppPrimaryList) {
    box = *m_ppPrimaryList;
    while(box) {
      class MatrixBox *matrix = dynamic_cast<MatrixBox *>(box);
      if (matrix) {
        if (matrix->IdOf() == idx)
          return matrix;
      }
      box = box->NextOf();
    }
  }

  if (m_ppSecondaryList) {
    box = *m_ppSecondaryList;
    while(box) {
      class MatrixBox *matrix = dynamic_cast<MatrixBox *>(box);
      if (matrix) {
        if (matrix->IdOf() == idx)
          return matrix;
      }
      box = box->NextOf();
    }
  }

  return NULL;
}
///

/// NameSpace::AllocateNonlinearityID
// Allocate an ID for a nonlinarity.
UBYTE NameSpace::AllocateNonlinearityID(void) const
{
  UBYTE idx = 0;

  if (m_ppPrimaryList) {
    const class Box *box = *m_ppPrimaryList;
    while(box) {
      const class ToneMapperBox *tmo = dynamic_cast<const ToneMapperBox *>(box);
      if (tmo) {
        if (tmo->TableDestinationOf() >= idx)
          idx = tmo->TableDestinationOf() + 1;
      }
      box = box->NextOf();
    }
  }

  if (m_ppSecondaryList) {
    const class Box *box = *m_ppSecondaryList;
    while(box) {
      const class ToneMapperBox *tmo = dynamic_cast<const ToneMapperBox *>(box);
      if (tmo) {
        if (tmo->TableDestinationOf() >= idx)
          idx = tmo->TableDestinationOf() + 1;
      }
      box = box->NextOf();
    }
  }

  if (idx > 15)
    JPG_THROW(OVERFLOW_PARAMETER,"NameSpace::AllocateNonlinearityID",
              "cannot create more than 16 nonlinear point transformations");
  
  return idx;
}
///

/// NameSpace::AllocateMatrixID
// Allocate an ID for a nonlinarity.
UBYTE NameSpace::AllocateMatrixID(void) const
{ 
  UBYTE idx = MergingSpecBox::FreeForm;

  if (m_ppPrimaryList) {
    const class Box *box = *m_ppPrimaryList;
    while(box) {
      const class MatrixBox *matrix = dynamic_cast<const MatrixBox *>(box);
      if (matrix) {
        if (matrix->IdOf() >= idx)
          idx = matrix->IdOf() + 1;
      }
      box = box->NextOf();
    }
  }

  if (m_ppSecondaryList) {
    const class Box *box = *m_ppSecondaryList;
    while(box) {
      const class MatrixBox *matrix = dynamic_cast<const MatrixBox *>(box);
      if (matrix) {
        if (matrix->IdOf() >= idx)
          idx = matrix->IdOf() + 1;
      }
      box = box->NextOf();
    }
  }

  if (idx > 15)
    JPG_THROW(OVERFLOW_PARAMETER,"NameSpace::AllocateNonlinearityID",
              "cannot create more than 11 linear transformations");
  
  return idx;
}
///

/// NameSpace::FindNonlinearity
// Find a parametric curve box with the given parameters, or return NULL if such a box
// does not yet exist.
const class ParametricToneMappingBox *NameSpace::FindNonlinearity(ParametricToneMappingBox::CurveType type,
                                                                  UBYTE e,
                                                                  FLOAT p1,FLOAT p2,FLOAT p3,FLOAT p4) const
{
  const class Box *box;

  if (m_ppPrimaryList) {
    box = *m_ppPrimaryList;
    while(box) {
      const class ParametricToneMappingBox *tmo = dynamic_cast<const ParametricToneMappingBox *>(box);
      if (tmo) {
        if (tmo->CompareCurve(type,e,p1,p2,p3,p4))
          return tmo;
      }
      box = box->NextOf();
    }
  }

  if (m_ppSecondaryList) {
    box = *m_ppSecondaryList;
    while(box) {
      const class ParametricToneMappingBox *tmo = dynamic_cast<const ParametricToneMappingBox *>(box);
      if (tmo) {
        if (tmo->CompareCurve(type,e,p1,p2,p3,p4))
          return tmo;
      }
      box = box->NextOf();
    }
  }

  return NULL;
}
///

/// NameSpace::isUniqueNonlinearity
// Check whether there is a duplicate nonlinearity of the given Id.
bool NameSpace::isUniqueNonlinearity(UBYTE tabidx) const
{
  const class Box *box;
  int count = 0;

  //
  // Note that each of the two sources may not contain a duplicate Id,
  // but the primary source may overload a box in the secondary list.
  if (m_ppPrimaryList) {
    box = *m_ppPrimaryList;
    while(box) {
      const class ToneMapperBox *tmo = dynamic_cast<const ToneMapperBox *>(box);
      if (tmo) {
        if (tmo->TableDestinationOf() == tabidx) {
          count++;
          if (count > 1)
            return false;
        }
      }
      box = box->NextOf();
    }
  }

  count = 0;
  if (m_ppSecondaryList) {
    box = *m_ppSecondaryList;
    while(box) {
      const class ToneMapperBox *tmo = dynamic_cast<const ToneMapperBox *>(box);
      if (tmo) {
        if (tmo->TableDestinationOf() == tabidx) {
          count++;
          if (count > 1)
            return false;
        }
      }
      box = box->NextOf();
    }
  }

  return true;
}
///

/// NameSpace::isUniqueMatrix
// Check whether there is a duplicate matrix of the given Id.
bool NameSpace::isUniqueMatrix(UBYTE idx) const
{  
  const class Box *box;
  int count = 0;

  if (m_ppPrimaryList) {
    box = *m_ppPrimaryList;
    while(box) {
      const class MatrixBox *matrix = dynamic_cast<const MatrixBox *>(box);
      if (matrix) {
        if (matrix->IdOf() == idx) {
          count++;
          if (count > 1)
            return false;
        }
      }
      box = box->NextOf();
    }
  }

  count = 0;
  if (m_ppSecondaryList) {
    box = *m_ppSecondaryList;
    while(box) {
      const class MatrixBox *matrix = dynamic_cast<const MatrixBox *>(box);
      if (matrix) {
        if (matrix->IdOf() == idx) {
          count++;
          if (count > 1)
            return false;
        }
      }
      box = box->NextOf();
    }
  }
  
  return true;
}
///
