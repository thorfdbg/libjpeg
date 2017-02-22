/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** This class contains and maintains the AC conditioning
** parameter templates.
**
** $Id: actable.hpp,v 1.10 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef MARKER_ACTABLE_HPP
#define MARKER_ACTABLE_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
class ACTemplate;
///

/// ACTable
class ACTable : public JKeeper {
  //
#if ACCUSOFT_CODE
  // Table specification. 4 DC and 4 AC tables.
  class ACTemplate *m_pParameters[8];
#endif
  //
public:
  ACTable(class Environ *env);
  //
  ~ACTable(void);
  //
  // Write the marker contents to a DHT marker.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the marker contents of a DHT marker.
  void ParseMarker(class ByteStream *io);
  //
  // Get the template for the indicated DC table or NULL if it doesn't exist.
  class ACTemplate *DCTemplateOf(UBYTE idx);
  //
  // Get the template for the indicated AC table or NULL if it doesn't exist.
  class ACTemplate *ACTemplateOf(UBYTE idx);
};
///

///
#endif
