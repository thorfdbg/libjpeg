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
**
** A random access IO stream that allows forwards and backwards
** seeking. This is an abstraction of the known "IOHook".
**
** $Id: randomaccessstream.hpp,v 1.8 2014/09/30 08:33:17 thor Exp $
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
** class RandomAccessStream                                     **
** Super Class: ByteStream                                      **
** Sub Classes: IOStream,FragmentedStream                       **
** Friends:     none                                            **
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
