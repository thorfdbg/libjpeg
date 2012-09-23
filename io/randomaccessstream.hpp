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
**
** A random access IO stream that allows forwards and backwards
** seeking. This is an abstraction of the known "IOHook".
**
** $Id: randomaccessstream.hpp,v 1.3 2012-09-09 15:53:51 thor Exp $
**
*/

#ifndef RANDOMACCESSSTREAM_HPP
#define RANDOMACCESSSTREAM_HPP

/// Includes
#include "tools/environment.hpp"
#include "bytestream.hpp"
///

/// Design
/** Design
******************************************************************
** class RandomAccessStream					**
** Super Class:	ByteStream					**
** Sub Classes: IOStream,FragmentedStream			**
** Friends:	none						**
******************************************************************

A direct descendant from the ByteStream, this abstract class
defines an advanced interface that additionally allows forwards
and backwards seeking in the stream.

* */
///

/// RandomAccessStream: An advanced interface of the ByteStream.
class RandomAccessStream : public ByteStream {
  //
  // No servicable parts in here.
  //
public:
  //
  // Constructor of a Random Access stream requires only a
  // buffer size and the environment.
  RandomAccessStream(class Environ *env,ULONG bufsize = 2048)
    : ByteStream(env,bufsize)
  { }
  //
  // Destructor: Nothing in here, overload to do something useful.
  virtual ~RandomAccessStream(void)
  {
  }
  //
  // Abstract interface, first the same methods as the bytestream.
  virtual LONG Fill(void)  = 0;
  virtual void Flush(void) = 0;
  virtual LONG Query(void) = 0;
  //
  // Peek the next word in the stream, deliver the marker without
  // advancing the file pointer. Deliver EOF in case we run into
  // the end of the stream. This is already implemented here
  // since its working does not depend on the specific implementation
  // on top.
  virtual LONG PeekWord(void);
  // 
  // SkipBytes skips bytes by either seeking over or, if that should turn
  // out to be impossible, by continuously pulling nonsense out of the
  // buffer. We make it here virtual again since more efficient ways
  // of skipping are possible in case we have random access.
  virtual void SkipBytes(ULONG skip) = 0;
  //
  // Set the file pointer to the indicated position (read only). This may
  // seek within the stream. Note that this implements an absolute 
  // seek relative to the start of the file.
  virtual void SetFilePointer(UQUAD newpos) = 0;
  //
};
///

///
#endif
