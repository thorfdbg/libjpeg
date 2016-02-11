/*
** This class parses the markers and holds the decoder together.
**
** $Id: decoder.hpp,v 1.16 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CODESTREAM_DECODER_HPP
#define CODESTREAM_DECODER_HPP

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
class Quantization;
class HuffmanTable;
class Frame;
class Image;
class Tables;
///

/// class Decoder
class Decoder : public JKeeper {
  //
  // The image object
  class Image        *m_pImage;
  //
public:
  Decoder(class Environ *env);
  //
  ~Decoder(void);
  //
  // Parse off the header.
  class Image *ParseHeader(class ByteStream *io);
  //
  // Parse off the EOI marker at the end of the image.
  // Returns true if there are more scans in the file,
  // else false.
  bool ParseTrailer(class ByteStream *io);
  //
  // Parse off decoder parameters.
  void ParseTags(const struct JPG_TagItem *tags);
};
///

///
#endif
