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

