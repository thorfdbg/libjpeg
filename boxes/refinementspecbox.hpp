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
** This class represents the refinement specification box, carrying the
** number of refinement scans in the base and residual layer of the
** image.
**
** $Id: refinementspecbox.hpp,v 1.3 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef BOXES_REFINEMENTSPECBOX_HPP
#define BOXES_REFINEMENTSPECBOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// class RefinementSpecBox
// This class represents the refinement specification box, carrying the
// number of refinement scans in the base and residual layer of the
// image.
class RefinementSpecBox : public Box {
  //
  // Number of refinement scans in the base image. This is called R_h
  // in the standard.
  UBYTE m_ucBaseRefinementScans;
  //
  // Number of refinement scans in the residual image. This is called
  // R_r in the standard.
  UBYTE m_ucResidualRefinementScans;
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
    Type = MAKE_ID('R','S','P','C')
  };
  //
  RefinementSpecBox(class Environ *env,class Box *&boxlist)
    : Box(env,boxlist,Type), m_ucBaseRefinementScans(0), m_ucResidualRefinementScans(0)
  { }
  //
  virtual ~RefinementSpecBox(void)
  { }
  //
  // Return the number of refinement scans in the base image.
  UBYTE BaseRefinementScansOf(void) const
  {
    return m_ucBaseRefinementScans;
  }
  //
  // Return the number of refinement scans in the extension image.
  UBYTE ResidualRefinementScansOf(void) const
  {
    return m_ucResidualRefinementScans;
  }
  //
  // Define the number of base refinement scans.
  void DefineBaseRefinementScans(UBYTE scans)
  {
    assert(scans <= 4);
    m_ucBaseRefinementScans = scans;
  }
  //
  // Define the number of refinement scans in the residual image.
  void DefineResidualRefinementScans(UBYTE scans)
  {
    assert(scans <= 4);
    m_ucResidualRefinementScans = scans;
  }
};
///

///
#endif

