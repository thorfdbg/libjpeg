/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** parameters.
**
** $Id: actemplate.cpp,v 1.3 2012-06-02 10:27:13 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "coding/actemplate.hpp"
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
