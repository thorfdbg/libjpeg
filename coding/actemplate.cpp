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
