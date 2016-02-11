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
