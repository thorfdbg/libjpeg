/*
** This class parses the markers and holds the decoder together.
**
** $Id: decoder.cpp,v 1.25 2014/09/30 08:33:15 thor Exp $
**
*/

/// Include
#include "codestream/decoder.hpp"
#include "io/bytestream.hpp"
#include "std/assert.hpp"
#include "codestream/tables.hpp"
#include "marker/frame.hpp"
#include "codestream/image.hpp"

///

/// Decoder::Decoder
// Construct the decoder
Decoder::Decoder(class Environ *env)
  : JKeeper(env), m_pImage(NULL)
{
}
///

/// Decoder::~Decoder
// Delete the decoder
Decoder::~Decoder(void)
{
  delete m_pImage;
}
///

/// Decoder::ParseHeader
// Parse off the header
class Image *Decoder::ParseHeader(class ByteStream *io)
{
  LONG marker;

  if (m_pImage)
    JPG_THROW(OBJECT_EXISTS,"Decoder::ParseHeader","stream parsing has already been started");
  
  marker = io->GetWord();
  if (marker != 0xffd8) // SOI
    JPG_THROW(MALFORMED_STREAM,"Decoder::ParseHeader","stream does not contain a JPEG file, SOI marker missing");

  m_pImage  = new(m_pEnviron) class Image(m_pEnviron);
  //
  // The checksum is not going over the headers but starts at the SOF.
  m_pImage->TablesOf()->ParseTables(io,NULL);
  //
  // The rest is done by the image.
  return m_pImage;
}
///

/// Decoder::ParseTags
// Accept decoder options.
void Decoder::ParseTags(const struct JPG_TagItem *tags)
{
  if (tags->GetTagData(JPGTAG_MATRIX_LTRAFO,JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR) == 
      JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE) {
    if (m_pImage) {
      m_pImage->TablesOf()->ForceColorTrafoOff();
    }
  }
}
///
