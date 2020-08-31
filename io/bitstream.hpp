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
** This class allows to read individual bits from a stream of bytes.
** This class implements the bytestuffing as required.
**
** $Id: bitstream.hpp,v 1.33 2014/11/12 21:24:45 thor Exp $
**
*/

#ifndef IO_BITSTREAM_HPP
#define IO_BITSTREAM_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "tools/checksum.hpp"
///

/// Forwards
///

/// class BitStream
template<bool bitstuffing>
class BitStream : public JObject {
  //
  // The bit-buffer for output.
  UBYTE m_ucB;
  //
  // The bit-buffer for input.
  ULONG m_ulB;
  //
  // The number of bits left.
  UBYTE m_ucBits;
  //
  // Number of bits the next fill operation fills in.
  UBYTE m_ucNextBits;
  //
  // Set if run into a marker.
  bool  m_bMarker;
  //
  // Set if run into an EOF.
  bool  m_bEOF;
  //
  // The byte stream we read from.
  class ByteStream *m_pIO;
  //
  // The checksum we keep updating.
  class Checksum   *m_pChk;
  //
  // Fill the byte-buffer. Implements bit-stuffing and byte-stuffing
  // and detects markers and generates appropriate errors.
  void Fill(void);
  //
  // Report an error if not enough bits were available, depending on
  // the error flag.
  void ReportError(void) NORETURN;
  //
public:
  BitStream()
  {
  }
  //
  ~BitStream(void)
  {
  }
  //
  void OpenForRead(class ByteStream *io,class Checksum *chk)
  {
    m_pIO        = io;
    m_pChk       = chk;
    m_ulB        = 0;
    m_ucBits     = 0;
    m_ucNextBits = 8;
    m_bMarker    = false;
    m_bEOF       = false;
  }
  //
  void OpenForWrite(class ByteStream *io,class Checksum *chk)
  { 
    m_pIO        = io;
    m_pChk       = chk;
    m_ucB        = 0;
    m_ucBits     = 8;
    m_bMarker    = false;
    m_bEOF       = false;
  }
  //
  // Return the environment.
  class Environ *EnvironOf(void) const
  {
    return m_pIO->EnvironOf();
  }
  //
  // Return the underlying bytestream.
  class ByteStream *ByteStreamOf(void) const
  {
    return m_pIO;
  }
  // 
  // Return the checksum.
  class Checksum *ChecksumOf(void) const
  {
    return m_pChk;
  }
  //
  // Read n bits at once, read at most 16 bits at once. Return the bits.
  template<int n>
  ULONG Get(void)
  {
    ULONG v;
    
    assert(n > 0 && n <= 24);

    // The Fill method ensures that there are always at least 16 bits in the buffer.
    if (n > m_ucBits) {
      Fill();
      if (unlikely(n > m_ucBits))
        ReportError();
    }
    
    v         = m_ulB >> (32 - n);
    m_ulB   <<= n;
    m_ucBits -= n;
    
    return v;
  }
  //
  // Get "bits" bits from the stream.
  ULONG Get(UBYTE bits)
  {
    ULONG v;
    
    assert(bits > 0 && bits <= 24);

    // The Fill method ensures that there are always at least 16 bits in the buffer.
    if (bits > m_ucBits) {
      Fill();
      if (unlikely(bits > m_ucBits))
        ReportError();
    }

    v         = m_ulB >> (32 - bits);
    m_ulB   <<= bits;
    m_ucBits -= bits;
    
    return v;
  }
  //
  // Return the next 16 bits from the stream without actually removing
  // them. Bits beyond the EOF are set to zero, but the error is not
  // yet reported.
  UWORD PeekWord(void)
  {
    if (m_ucBits < 16)
      Fill();
    
    return m_ulB >> 16;
  }
  //
  // Remove n bits without reading them. Prior calls must have ensured
  // that this number of bits is actually in the stream.
  void SkipBits(UBYTE size)
  {
    if (unlikely(size > m_ucBits))
      ReportError();

    m_ulB   <<= size;
    m_ucBits -= size;
  }
  //
  // Flush the buffer out to the stream. Must be done at the end of the
  // coding to ensure that all bits are written out.
  void Flush(void)
  {
    if (m_ucBits < 8) {
      // The standard suggests (in an informative note) to fill in
      // remaining bits by 1's, which interestingly creates the likelyhood
      // of a bitstuffing case. Interestingly, the standard also says that
      // a 0xff in front of a marker is a "fill byte" that may be dropped.
      // Conclusion is that we may have a 0xff just in front of a marker without
      // the byte stuffing. Wierd.
      if (!bitstuffing)
        m_ucB   |= (1 << m_ucBits) - 1;
      m_pIO->Put(m_ucB);
      if (m_pChk)
        m_pChk->Update(m_ucB);
      m_ucBits = 8;
      if (m_ucB == 0xff) {   // stuffing case? 
        m_pIO->Put(0x00);    // stuff a zero byte
        if (m_pChk)
          m_pChk->Update(0x00);
        // Note that this must also happen if we are bitstuffing to avoid a pseudo-0xffff
        // marker (JPEG 2000 could have dropped the 0xff here, but we can't).
        // Actually, such markers are allowable, or rather might be, but
        // be conservative and avoid writing them.
      }
      m_ucB = 0;
    }
  }
  //
  // Skip the bitstuffed zero-bit at the end of a 
  // line to be able to parse for a marker segment. This covers a
  // race-condition in which a zero-byte had to be stuffed at the encoder
  // side to avoid a double-0xff appear. This zero-byte is never read
  // on the decoder side unless triggered manually since it is not
  // part of the stream. If bytestuffing is enabled (not bitstuffing)
  // the zero-byte is already removed as part of the refill of the 0xff.
  void SkipStuffing(void)
  {
    if (bitstuffing) {
      // Only in case all bits of the byte are read, and we
      // need the refill anyhow... trigger it early.
      if (m_ucBits == 0 && m_ucNextBits == 7) {
        Fill();
      }
    }
  }
  //
  template<int count>
  void Put(ULONG bitbuffer)
  { 
    int n = count;
    assert(n > 0 && n <= 32);
    
    // Do we want to output more bits than
    // there is room in the buffer?
    while(n > m_ucBits) {
      // If so, output all bits we can.
      n     -= m_ucBits;   // that many bits go away
      m_ucB |= (bitbuffer>>n) & ((1<<m_ucBits)-1); // place them into buffer
      // m_ucBits = 0; // superfluous: We've now zero bits space left. 
      m_pIO->Put(m_ucB);
      if (m_pChk)
        m_pChk->Update(m_ucB);
      m_ucBits = 8;
      if (m_ucB == 0xff) {  // byte stuffing case?
        if (bitstuffing) {
          m_ucBits = 7;
        } else {
          m_pIO->Put(0x00);    // stuff a zero byte
          if (m_pChk)
            m_pChk->Update(0x00);
        }
      }
      m_ucB = 0;
    }

    // Now, we've more bits space left than we want to put.
    // This is the easy case:
    // Here, n <= m_ucBits.
    m_ucBits   -= n;  // that many bits space left
    m_ucB |= (bitbuffer & ((1<<n)-1)) << m_ucBits;
  }
  //
  // Put "n" bits into the stream.
  void Put(UBYTE n,ULONG bitbuffer)
  {
    assert(n > 0 && n <= 32);
    
    // Do we want to output more bits than
    // there is room in the buffer?
    while(n > m_ucBits) {
      // If so, output all bits we can.
      n          -= m_ucBits;   // that many bits go away
      m_ucB |= (bitbuffer>>n) & ((1<<m_ucBits)-1); // place them into buffer
      // m_ucBits = 0; // superfluous: We've now zero bits space left. 
      m_pIO->Put(m_ucB);
      if (m_pChk)
        m_pChk->Update(m_ucB);
      m_ucBits = 8;
      if (m_ucB == 0xff) {  // byte stuffing case?
        if (bitstuffing) {
          m_ucBits = 7;
        } else {
          m_pIO->Put(0x00);    // stuff a zero byte
          if (m_pChk)
            m_pChk->Update(0x00);
        }
      }
      m_ucB = 0;
    }

    // Now, we've more bits space left than we want to put.
    // This is the easy case:
    // Here, n <= m_ucBits.
    m_ucBits   -= n;  // that many bits space left
    m_ucB |= (bitbuffer & ((1<<n)-1)) << m_ucBits;
  }
};
///

///
#endif
  
