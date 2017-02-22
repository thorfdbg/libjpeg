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
** This class keeps a simple checksum over the data in the legacy codestream
** thus enabling decoders to check whether the data has been tampered with.
**
** $Id: checksumbox.hpp,v 1.5 2014/09/30 08:33:14 thor Exp $
**
*/

#ifndef BOXES_CHECKSUMBOX_HPP
#define BOXES_CHECKSUMBOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// Forwards
class MemoryStream;
class ByteStream;
class Checksum;
///

/// Class CheckumBox
class ChecksumBox : public Box {
  //
  // The checksum bytes so far.
  ULONG m_ulCheck;
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box. Returns true in case the contents is
  // parsed and the stream can go away.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize);
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  // Returns whether the box content is already complete and the stream
  // can go away.
  virtual bool CreateBoxContent(class MemoryStream *target);
  //
  //
public:
  enum {
    Type = MAKE_ID('L','C','H','K')
  };
  //
  ChecksumBox(class Environ *env,class Box *&boxlist)
    : Box(env,boxlist,Type)
  { }
  //
  ~ChecksumBox(void)
  { }
  //
  // Set the contents of the checksum box from the checksum class.
  void InstallChecksum(const class Checksum *check);
  //
  // Return the value of the checksum as stored in this file.
  ULONG ValueOf(void) const
  {
    return m_ulCheck;
  }
};
///

///
#endif

  
  
