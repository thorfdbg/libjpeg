/*
** This class contains the marker that defines the JPEG LS thresholds.
**
** $Id: thresholds.hpp,v 1.9 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef MARKER_THRESHOLDS_HPP
#define MARKER_THRESHOLDS_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// Thresholds
class Thresholds : public JKeeper {
  //
  // Parameters of the JPEG LS coder as defined by C.2.4.1.1
  //
  // Maximum sample value.
  UWORD m_usMaxVal;
  //
  // Bucket threshold 1
  UWORD m_usT1;
  //
  // Bucket threshold 2
  UWORD m_usT2;
  //
  // Bucket threshold 3
  UWORD m_usT3;
  //
  // The statistics reset value.
  UWORD m_usReset;
  //
public:
  Thresholds(class Environ *env);
  //
  ~Thresholds(void);
  //
  // Write the marker contents to a LSE marker.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the marker contents of a LSE marker.
  // marker length and ID are already parsed off.
  void ParseMarker(class ByteStream *io,UWORD len);
  //
  // Return the maximum sample value.
  UWORD MaxValOf(void) const
  {
    return m_usMaxVal;
  }
  //
  // Return the T1 value.
  UWORD T1Of(void) const
  {
    return m_usT1;
  }
  //
  // Return the T2 value.
  UWORD T2Of(void) const
  {
    return m_usT2;
  }
  //
  // Return the T3 value.
  UWORD T3Of(void) const
  {
    return m_usT3;
  }
  //
  // Return the Reset interval.
  UWORD ResetOf(void) const
  {
    return m_usReset;
  }
  //
  // Install the defaults for a given bits per pixel value
  // and the quality/near value.
  void InstallDefaults(UBYTE bpp,UWORD near);
};
///

///
#endif
