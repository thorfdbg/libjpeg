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
 * An implementation of the ByteStream class that reads/writes bytes
 * to a "ram disk".
 *
 * $Id: memorystream.hpp,v 1.6 2012-09-09 15:53:51 thor Exp $
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
