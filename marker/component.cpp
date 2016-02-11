/*
**
** This class represents a single component.
**
** $Id: component.cpp,v 1.14 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "marker/component.hpp"
#include "io/bytestream.hpp"
///

/// Component::Component
Component::Component(class Environ *env,UBYTE idx,UBYTE prec,UBYTE subx,UBYTE suby)
  : JKeeper(env), m_ucIndex(idx), m_ucID(idx), m_ucSubX(subx), m_ucSubY(suby), m_ucPrecision(prec)
{
}
///

/// Component::~Component
Component::~Component(void)
{
}
///

/// Component::WriteMarker
// Write the component information to the bytestream.
void Component::WriteMarker(class ByteStream *io)
{
  // Write the component identifier.
  io->Put(m_ucID);

  // Write the dimensions of the MCU
  assert(m_ucMCUWidth  < 16);
  assert(m_ucMCUHeight < 16);

  io->Put((m_ucMCUWidth << 4) | m_ucMCUHeight);

  // Write the quantization table index.
  io->Put(m_ucQuantTable);
}
///

/// Component::ParseMarker
void Component::ParseMarker(class ByteStream *io)
{
  LONG data;

  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","frame marker incomplete, no component identifier found");

  m_ucID = data;

  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","frame marker incomplete, subsamling information missing");

  m_ucMCUWidth  = data >> 4;
  m_ucMCUHeight = data & 0x0f;

  data = io->Get();
  if (data < 0 || data > 3)
    JPG_THROW(MALFORMED_STREAM,"Component::ParseMarker","quantization table identifier corrupt, must be >= 0 and <= 3");

  m_ucQuantTable = data;
}
///
