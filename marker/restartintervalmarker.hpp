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
** This class keeps the restart interval size in MCUs.
**
** $Id: restartintervalmarker.hpp,v 1.8 2021/09/08 10:30:06 thor Exp $
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
  ULONG     m_ulRestartInterval;
  //
  // This boolean is set in case the restart interval marker is allowed
  // to be 5 or 6 bytes long as well. This only holds for JPEG LS.
  bool      m_bExtended;
  //
public:
  RestartIntervalMarker(class Environ *env,bool extended);
  //
  ~RestartIntervalMarker(void)
  {
  }
  //
  // Install the defaults, namely the interval.
  void InstallDefaults(ULONG inter)
  {
    if (inter > 0xffff && !m_bExtended)
      JPG_THROW(OVERFLOW_PARAMETER,"RestartIntervalMarker::InstallDefaults",
                "the restart interval is allowed to be at most 65535 for JPEG (ISO/IEC 10918-1)");
    m_ulRestartInterval = inter;
  }
  //
  // Return the currently active restart interval.
  ULONG RestartIntervalOf(void) const
  {
    return m_ulRestartInterval;
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
