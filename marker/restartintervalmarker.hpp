/*
** This class keeps the restart interval size in MCUs.
**
** $Id: restartintervalmarker.hpp,v 1.7 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef MARKER_RESTARTINTERVALMARKER_HPP
#define MARKER_RESTARTINTERVALMARKER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// RestartIntervalMarker
// This class keeps the restart interval size in MCUs.
class RestartIntervalMarker : public JKeeper {
  //
  // The restart interval size in MCUs, or zero if restart markers
  // are disabled.
  UWORD     m_usRestartInterval;
  //
public:
  RestartIntervalMarker(class Environ *env);
  //
  ~RestartIntervalMarker(void)
  {
  }
  //
  // Install the defaults, namely the interval.
  void InstallDefaults(UWORD inter)
  {
    m_usRestartInterval = inter;
  }
  //
  // Return the currently active restart interval.
  UWORD RestartIntervalOf(void) const
  {
    return m_usRestartInterval;
  }
  //
  // Write the marker (without the marker id) to the stream.
  void WriteMarker(class ByteStream *io) const;
  //
  // Parse the marker from the stream.
  void ParseMarker(class ByteStream *io);
  //
};
///

///
#endif
