/*
** This class updates a checksum from bytes read or written over an
** arbitrary IO stream. It links an IO stream with a checksum.
**
** $Id: checksumadapter.hpp,v 1.6 2014/09/30 08:33:17 thor Exp $
**
*/

#ifndef IO_CHECKSUMADAPTER_HPP
#define IO_CHECKSUMADAPTER_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
///

/// Class ChecksumAdapter
// This class updates a checksum from bytes read or written over an
// arbitrary IO stream. It links an IO stream with a checksum.
class ChecksumAdapter : public ByteStream {
  //
  // The checksum that is updated by this stream.
  class Checksum   *m_pChecksum;
  //
  // The stream that does the real job.
  class ByteStream *m_pStream;
  //
  // A flag whether write-only or read-only is attempted.
  // This is actually only for debugging purposes.
  bool              m_bWriting;
  //
  // Fill up the buffer and flush it.
  // these two have to be replaced by the corresponding
  // member functions of the inherited classses
  virtual LONG Fill(void);
  //
  // Flush the IO buffer. This must be defined by the instances of
  // this class.
  virtual void Flush(void);
  //
public:
  ChecksumAdapter(class ByteStream *parent,class Checksum *sum,bool writing);
  //
  // The destructor. This updates the checksum. It cannot flush the buffer
  // on writing since this may throw.
  ~ChecksumAdapter(void);
  //
  // Peek the next word in the stream, deliver the marker without
  // advancing the file pointer. Deliver EOF in case we run into
  // the end of the stream.
  virtual LONG PeekWord(void);
  //
  // read stream buffer status. Also to be overloaded.
  virtual LONG Query(void);
  //
  // On reading & writing, flush the checksum and prepare to go.
  void Close(void);
  //
  // Return the checksum we are updating.
  class Checksum *ChecksumOf(void) const
  {
    return m_pChecksum;
  }
};
///

///
#endif


  
