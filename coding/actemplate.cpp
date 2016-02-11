/*
** This class contains and maintains the AC conditioning
** parameters.
**
** $Id: actemplate.cpp,v 1.9 2014/09/30 08:33:16 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "coding/actemplate.hpp"
#if ACCUSOFT_CODE
///

/// ACTemplate::ACTemplate
ACTemplate::ACTemplate(class Environ *env)
  : JKeeper(env), m_ucLower(0), m_ucUpper(1), m_ucBlockEnd(5)
{
}
///

/// ACTemplate::~ACTemplate
ACTemplate::~ACTemplate(void)
{
}
///

/// ACTemplate::ParseDCMarker
// Parse off DC conditioning parameters.
void ACTemplate::ParseDCMarker(class ByteStream *io)
{
  LONG dc = io->Get();
  UBYTE l,u;

  if (dc == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"ACTemplate::ParseDCMarker",
	      "unexpected EOF while parsing off the AC conditioning parameters");

  l = dc & 0x0f;
  u = dc >> 4;

  if (u < l)
    JPG_THROW(MALFORMED_STREAM,"ACTemplate::ParseDCMarker",
	      "upper DC conditioning parameter must be larger or equal to the lower one");

  m_ucLower = l;
  m_ucUpper = u;
}
///

/// ACTemplate::ParseACMarker
// Parse off an AC conditioning parameter.
void ACTemplate::ParseACMarker(class ByteStream *io)
{ 
  LONG ac = io->Get();

  if (ac == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"ACTemplate::ParseACMarker",
	      "unexpected EOF while parsing off the AC conditioning parameters");

  if (ac < 1 || ac > 63)
    JPG_THROW(MALFORMED_STREAM,"ACTemplate::ParseACMarker",
	      "AC conditoning parameter must be between 1 and 63");

  m_ucBlockEnd = ac;
}
///

/// ACTemplate::InitDefaults
// Just install the defaults found in the standard.
void ACTemplate::InitDefaults(void)
{
  m_ucLower    = 0;
  m_ucUpper    = 1;
  m_ucBlockEnd = 5;
}
///

///
#endif
