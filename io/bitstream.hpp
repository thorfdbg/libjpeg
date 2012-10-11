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
** This class allows to read individual bits from a stream of bytes.
** This class implements the bytestuffing as required.
**
** $Id: bitstream.hpp,v 1.21 2012-10-07 11:13:33 thor Exp $
**
*/

#ifndef IO_BITSTREAM_HPP
#define IO_BITSTREAM_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
///

/// Forwards
///

/// class BitStream
template<bool bitstuffing>
class BitStream : public JObject {
  //
  // The bit-buffer.
  UBYTE m_ucB;
  //
  // The number of bits left.
  UBYTE m_ucBits;
  //
  // The number of bits in the next byte.
  UBYTE m_ucNextBits;
  //
  // The byte stream we read from.
  class ByteStream *m_pIO;
  //
  // Fill the byte-buffer. Implement bit-stuffing.
  void Fill(void)
  {
    class ByteStream *io = m_pIO;
    LONG dt = io->Get();

    if (dt == 0xff) {
      io->LastUnDo();
      if (bitstuffing) {
	if (io->PeekWord() < 0xff80) {
	  // Bitstuffing.
	  io->Get();
	  m_ucB        = dt;
	  m_ucBits     = 8;
	  m_ucNextBits = 7;
	  return;
	} else {
	  // A marker. Do not advance over the marker, but
	  // rather stay at it so the logic upwards can fix it.
	  // Be consistent with QM and fit zeros in.
	  dt = 0;
	}
      } else {
	if (io->PeekWord() == 0xff00) {
	  io->GetWord(); // Bytestuffing
	} else {
	  // A marker. Do not advance over the marker, but
	  // rather stay at it so the logic upwards can fix it.
	  // Be consistent with QM and fit zeros in.
	  // Probably a bad idea as we may have pseudo-fill-bytes.
	  dt = 0xff;
	}
      }
    } else if (dt == ByteStream::EOF) {
      class Environ *m_pEnviron = io->EnvironOf();
      JPG_THROW(UNEXPECTED_EOF,"BitStream::Fill","found EOF within bit segment");
    }
    m_ucB        = dt;
    m_ucBits     = m_ucNextBits;
    m_ucNextBits = 8;
  }
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
  void OpenForRead(class ByteStream *io)
  {
    m_pIO        = io;
    m_ucB        = 0;
    m_ucBits     = 0;
    m_ucNextBits = 8;
  }
  //
  void OpenForWrite(class ByteStream *io)
  { 
    m_pIO        = io;
    m_ucB        = 0;
    m_ucBits     = 8;
    m_ucNextBits = 8;
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
  // Read n bits at once, read at most 32 bits at once. Return the bits.
  template<int n>
  ULONG Get(void)
  {
    assert(n <= 32);
    
    if (n <= m_ucBits) {
      m_ucBits -= n;
      return (m_ucB >> m_ucBits) & ((1UL << n) - 1);
    } else if (n <= 8) {
      UBYTE rem  = n - m_ucBits;
      ULONG res  = (m_ucB & ((1UL << m_ucBits) - 1)) << rem;
      Fill();
      m_ucBits -= rem;
      return res | ((m_ucB >> m_ucBits) & ((1UL << rem) - 1));
    } else {
      ULONG res  = 0;
      UBYTE bits = n;
      do {
	if (m_ucBits == 0) {
	  Fill();
	}
	UBYTE avail = m_ucBits;
	if (avail > bits)
	  avail = bits; // do not remove more bits than requested.
	
	// remove avail bits from the byte
	m_ucBits -= avail;
	bits     -= avail;
	res       = (res << avail) | ((m_ucB >> m_ucBits) & ((1UL << avail) - 1));
      } while(bits);
      
      return res;
    }
  }
  //
  // Get "bits" bits from the stream.
  ULONG Get(UBYTE bits)
  {
    ULONG res  = 0;
    
    assert(bits > 0 && bits <= 32);

    do { 
      if (m_ucBits == 0) {
	Fill();
      }
      UBYTE avail = m_ucBits;
      if (avail > bits)
	avail = bits; // do not remove more bits than requested.
      
      // remove avail bits from the byte
      m_ucBits -= avail;
      bits     -= avail;
      res       = (res << avail) | ((m_ucB >> m_ucBits) & ((1UL << avail) - 1));
    } while(bits);
    
    return res;
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
      m_ucBits = 8;
      if (m_ucB == 0xff) {  // stuffing case? 
	m_pIO->Put(0x00);    // stuff a zero byte
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
      if (m_ucB == 0xff && m_ucBits == 0) {
	Fill();
      }
    }
  }
  //
  template<int count>
  void Put(UBYTE bitbuffer)
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
      m_ucBits = 8;
      if (m_ucB == 0xff) {  // byte stuffing case?
	if (bitstuffing) {
	  m_ucBits = 7;
	} else {
	  m_pIO->Put(0x00);    // stuff a zero byte
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
      m_ucBits = 8;
      if (m_ucB == 0xff) {  // byte stuffing case?
	if (bitstuffing) {
	  m_ucBits = 7;
	} else {
	  m_pIO->Put(0x00);    // stuff a zero byte
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
  
