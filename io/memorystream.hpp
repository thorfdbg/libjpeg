/*
 * An implementation of the ByteStream class that reads/writes bytes
 * to a "ram disk".
 *
 * $Id: memorystream.hpp,v 1.12 2014/09/30 08:33:17 thor Exp $
 *
 */

#ifndef MEMORYSTREAM_HPP
#define MEMORYSTREAM_HPP

/// Includes
#include "bytestream.hpp"
///

/// Design
/** Design
******************************************************************
** class MemoryStream						**
** Super Class:	ByteStream					**
** Sub Classes: none						**
** Friends:	none						**
******************************************************************

A direct descendant from the ByteStream, this class implements
a RAM disk like function. Data may be stored in a MemoryStream,
will be buffered here and can be read back from this stream.

The idea is that you build a memory stream, then clone this
stream to build a second stream. One of two modes are possible
here, OFFSET_CURRENT and OFFSET_BEGINNING. In the first case,
reading from the cloned memory stream will return bytes starting
at the file pointer position of the original stream at the time
it was cloned, whereas OFFSET_BEGINNING will return bytes
from the beginning of the cloned original stream in any case.

After you have a cloned stream, you may (continue to) push
data into the first original stream, and may read these bytes
back now, or later, from the clone.

You *cannot* read and write simulatenously on the same stream,
this won't work.

The libjpeg uses this type of ByteStream as output stream of
the EBCOTCoder class, and the readback clones for later on
correcting the termination position, as required in the
TruncationNode class.
* */
///

/// class MemoryStream
// This implementation of a ByteStream simply buffers
// the data it gets. It is stored as a singly linked list
// and can be flushed later on.
// This class accepts currently only writes, no reads. No seeks therefore
// either. This could be changed if we need it, though.
class MemoryStream : public ByteStream {
  //
  struct BufferNode : public JObject {
    struct BufferNode *bn_pNext;      // next buffer
    UBYTE             *bn_pucBuffer;  // the buffer, keeping the data
    //
    BufferNode(void)
      : bn_pNext(NULL), bn_pucBuffer(NULL)
    { }
  };
  //
  // The list of all buffers
  struct BufferNode   *m_pBufferList;
  //
  // The last buffer, the position where data is appended.
  struct BufferNode   *m_pLast;
  //
  // The current read-out position.
  struct BufferNode   *m_pCurrent;   // List of buffers.
  //
  // The memory stream this one reads from.
  class MemoryStream  *m_pParent;
public:
  //
  // Constructor: build a MemoryStream given a buffer size.
  MemoryStream(class Environ *env, ULONG bufsize = 2048)
    : ByteStream(env, bufsize),
      m_pBufferList(NULL), m_pLast(NULL), m_pCurrent(NULL), m_pParent(NULL)
  { }
  //
  // Destructor: Get rid of the buffered bytes
  virtual ~MemoryStream(void);
  //
  // Re-open an existing memory stream to read data from the parent starting
  // at the indicated node.
  void ReOpenFrom(class MemoryStream *parent,LONG mode);
  //
  // The next one is special: It clones a memory stream from
  // an existing stream to be able to seek backwards.
  // The cloned stream may be deleted, but it must be deleted
  // before the parent stream is deleted.
  // MODE is a seek mode from above. Either OFFSET_CURRENT or OFFSET_BEGINNING
  MemoryStream(class Environ *ev, class MemoryStream *parent,LONG mode);
  //
  // Implementation of the abstract functions:
  virtual LONG Fill(void);
  virtual void Flush(void);
  virtual LONG Query(void)
  {
    return 0;  // always success
  }
  //
  // Another member function: Given a read memory stream, write out
  // all (partial) data it contains. 
  ULONG Push(class ByteStream *dest,ULONG total);
  // 
  // Get the next two bytes without removing them from the stream.
  virtual LONG PeekWord(void);
  //
  // Push contents of a different stream into a memory stream, i.e. write
  // into the memory stream buffer by using bytes from another bytestream.
  void Append(class ByteStream *in,ULONG bytesize);
  //
  // Return the number of bytes buffered within this memory stream starting
  // at the current file position.
  ULONG BufferedBytes(void) const;
  //
  // Clean the buffered bytes.
  void Clean(void);
};
///

///
#endif
