/*
** This class is the abstract superclass for all decoder templates, i.e.
** classes that generate the classes for decoding bits to symbols.
**
** $Id: decodertemplate.hpp,v 1.7 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CODING_DECODERTEMPLATE_HPP
#define CODING_DECODERTEMPLATE_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class DecoderTemplate
class DecoderTemplate : public JKeeper {
  //
public:
  DecoderTemplate(class Environ *env)
    : JKeeper(env)
  { }
  //
  virtual ~DecoderTemplate(void)
  {
  }
  //
  // Parse the huffman specific part of the DHT table.
  virtual void ParseMarker(class ByteStream *io) = 0;
};
///

///
#endif
