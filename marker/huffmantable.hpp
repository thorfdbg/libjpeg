/*
** This class contains and maintains the huffman code parsers.
**
** $Id: huffmantable.hpp,v 1.16 2015/09/17 11:20:35 thor Exp $
**
*/

#ifndef MARKER_HUFFMANTABLE_HPP
#define MARKER_HUFFMANTABLE_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Forwards
class ByteStream;
class DecderTemplate;
///

/// HuffmanTable
class HuffmanTable : public JKeeper {
  //
  // Table specification. 4 DC and 4 AC tables.
  class HuffmanTemplate *m_pCoder[8];
  //
public:
  HuffmanTable(class Environ *env);
  //
  ~HuffmanTable(void);
  //
  // Check whether the tables are empty or not
  // In such a case, do not write the tables.
  bool isEmpty(void) const;
  //
  // Write the marker contents to a DHT marker.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the marker contents of a DHT marker.
  void ParseMarker(class ByteStream *io);
  //
  // Get the template for the indicated DC table or NULL if it doesn't exist.
  class HuffmanTemplate *DCTemplateOf(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden);
  //
  // Get the template for the indicated AC table or NULL if it doesn't exist.
  class HuffmanTemplate *ACTemplateOf(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden);
  //
  // Adjust all coders in here to the statistics collected before, i.e.
  // find optimal codes.
  void AdjustToStatistics(void);
};
///

///
#endif
