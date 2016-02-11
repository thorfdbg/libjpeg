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
