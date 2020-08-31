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


  
