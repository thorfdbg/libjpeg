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
** An implementation of the random access stream:
** Use user call-back hooks to perform the file IO. This is
** the back-end class to perform the real job getting bytes in and
** out of the library.
**
** $Id: iostream.hpp,v 1.8 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef IOSTREAM_HPP
#define IOSTREAM_HPP

/// Includes
#include "randomaccessstream.hpp"
///

/// Design
/** Design
******************************************************************
** class IOStream                                               **
** Super Class: RandomAccessStream                              **
** Sub Classes: none                                            **
** Friends:     none                                            **
******************************************************************

A direct descendant from the ByteStream, this class implements
IO over a hook function. This is in this case the user-supplied
IOHook call back function which has to perform reading and
writing of buffers. Hence, most requests of the IOHook are
forwarded to the hook callback function.

* */
///

/// IOStream: Implementation of RandomAccessStream
// In an ideal world, I would simply derive the Hook class.
// However, the hook has C linkage and I don't want to derive it
// here. This is an implementation of the ByteStream class.
// It reads and writes data to an "external" stream defined by 
// a hook.
class IOStream : public RandomAccessStream {
  //
  // The hook we get data from/send data to 
  struct JPG_Hook m_Hook;
  //
  // The thing the client uses for the IO management
  APTR            m_pHandle;
  //
  // Number of bytes to seek to we have cached
  ULONG           m_ulCachedSeek;
  //
  // For internal management of the user, yes really a value.
  LONG            m_lUserData;
  //
  // if no user supplied buffer, this one is used. It is
  // m_ulBufSize + 1 (!) bytes large.
  APTR            m_pSystemBuffer;
  //
  // The user provided buffer, if any.
  APTR            m_pUserBuffer;
  //
  // true in case the stream accepts seeks.
  bool            m_bSeekable;    
  //
  // Advance the file position on the underlying hook, truely,
  // Skip bytes by first trying to seek over and then by trying to continuously
  // read over the bytes. Returns false in case seeking did not
  // work and we must advance the file pointer manually by reading
  // data.
  bool AdvanceFilePointer(ULONG distance);
  //
  // A dummy hook entry that is used when the user provided no hook.
  static LONG DefaultEntry(struct JPG_Hook *hk,struct JPG_TagItem *tag);
  //
public:
  //
  // Constructor: build an IOhook structure from an ordinary hook,
  // keep track of the buffer size, but do not yet allocate the buffer
  // (due to the standard problem of exceptions in constructors).
  // An optional user-provided buffer can be passed in which is used
  // instead of the system buffer if non-NULL for custom buffering.
  IOStream(class Environ *env,
           struct JPG_Hook *in,APTR stream,ULONG bufsize = 2048,
           ULONG userdata = 0,UBYTE *buffer = NULL);
  //
  // Does the same from a taglist.
  IOStream(class Environ *env,const struct JPG_TagItem *tags);
  //
  // Destructor: This just gets rid of the buffer, but does not flush
  // the buffer itself. If it had to, I'd to keep track whether this 
  // stream was used for reading or writing, and had to flush it
  // for writing, only. Ok, I could do so, but I'd to keep one additional
  // flag I currently don't want to invest. 
  // Just flush it before you close it, a'right?
  virtual ~IOStream(void);
  //
  // Implementation of the abstract functions:
  virtual LONG Fill(void);
  virtual void Flush(void);
  virtual LONG Query(void);
  //
  // Peek the next word in the stream, deliver the marker without
  // advancing the file pointer. Deliver JPG_EOF in case we run into
  // the end of the stream. This stream requires a specific implementation
  // of the primitive.
  virtual LONG PeekWord(void);
  // 
  // SkipBytes skips bytes by either seeking over or, if that should turn
  // out to be impossible, by continuously pulling nonsense out of the
  // buffer.
  virtual void SkipBytes(ULONG skip);
  //
  // Set the file pointer to the indicated position (read only!). This may
  // seek within the stream. Note that this implements an absolute 
  // seek relative to the start of the file.
  virtual void SetFilePointer(UQUAD newpos);
  //
  // An unbuffered file seeking method that works on reading and writing,
  // but requires proper buffer flushing to make it work.
  void Seek(QUAD newpos,LONG mode = JPGFLAG_OFFSET_BEGINNING);
  //
  // Return the number of internal bytes that still wait in the buffer
  // and that haven't been read yet.
  ULONG RemainingBytes(void) const
  {
    return m_pucBufEnd - m_pucBufPtr;
  }
};
///

///
#endif
