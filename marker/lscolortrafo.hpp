/*
** This class defines the arbitrary color transformation defined
** in JPEG-LS part-2. It is - in a sense - a special case of the 
** JPEG 2000 part-2 reversible color transformation.
**
** $Id: lscolortrafo.hpp,v 1.7 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef MARKER_LSCOLORTRAFO_HPP
#define MARKER_LSCOLORTRAFO_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class LSColorTrafo
// This class defines the arbitrary color transformation defined
// in JPEG-LS part-2. It is - in a sense - a special case of the 
// JPEG 2000 part-2 reversible color transformation.
class LSColorTrafo : public JKeeper {
  //
  // Number of components that are transformed here:
  UBYTE  m_ucDepth;
  //
  // Near value, if known.
  UWORD  m_usNear;
  //
  // The maximum value of the transformed components.
  UWORD  m_usMaxTrans;
  //
  // Labels of the input components. An array of m_ucDepth
  // indices.
  UBYTE *m_pucInputLabels;
  //
  // The division/shift to be applied after the linear transformation.
  UBYTE *m_pucShift;
  //
  // The center flags which identify whether the components are zero-
  // centered or centered mid-way.
  bool  *m_pbCentered;
  //
  // The transformation matrix as m_ucDepth^2 matrix of multipliers.
  // Interestingly, the specs say these are unsigned. Hmmm?
  // The fast coordinate is here over the input components for
  // reconstrution.
  UWORD *m_pusMatrix;
  //
public:
  LSColorTrafo(class Environ *env);
  //
  ~LSColorTrafo(void);
  //
  // Write the marker contents to a LSE marker.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the marker contents of a LSE marker.
  // marker length and ID are already parsed off.
  void ParseMarker(class ByteStream *io,UWORD len);
  //
  // Return the maximum sample value.
  UWORD MaxTransOf(void) const
  {
    return m_usMaxTrans;
  }
  // 
  // The number of components this transformation
  // handles.
  UBYTE DepthOf(void) const
  {
    return m_ucDepth;
  }
  //
  // Return the labels of the components. Note that
  // this are not component indices!
  const UBYTE *LabelsOf(void) const
  {
    return m_pucInputLabels;
  }
  //
  // Return the right-shift that is applied before the final
  // modulo addition/subtraction.
  const UBYTE *RightShiftOf(void) const
  {
    return m_pucShift;
  }
  //
  // Return the array of the centered flags.
  const bool *CenteredFlagsOf(void) const
  {
    return m_pbCentered;
  }
  //
  // Return the transformation matrix as a pointer to
  // Depth^2 entries, the fast direction iterates over
  // the input components.
  const UWORD *MatrixOf(void) const
  {
    return m_pusMatrix;
  }
  //
  // Return the near value - not stored in the marker,
  // only recorded on encoding. This is the per-component
  // l-infinity error bound for JPEG-LS.
  UWORD NearOf(void) const
  {
    return m_usNear;
  }
  //
  // Install the defaults for a given sample count. This
  // installs the example pseudo-RCT given in the specs.
  void InstallDefaults(UBYTE bpp,UBYTE near);
};
///

///
#endif
