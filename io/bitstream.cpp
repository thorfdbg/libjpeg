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
** $Id: bitstream.cpp,v 1.9 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "io/bitstream.hpp"
///
      
/// Bitstream::Fill
// Fill the byte-buffer. Implement bit-stuffing.
template<bool bitstuffing>
void BitStream<bitstuffing>::Fill(void)
{
  assert(m_ucBits <= 24);
  
  do {
    LONG dt = m_pIO->Get();
    
    if (dt == 0xff) {
      // Possible bitstuffing or bytestuffing.
      m_pIO->LastUnDo();
      //
      if (bitstuffing) {
        if (m_pIO->PeekWord() < 0xff80) {
          // proper bitstuffing. Remove eight bits
          // for now, but...
          m_pIO->Get();
          if (m_pChk)
            m_pChk->Update(dt);
          //
          // ...the next byte has a filler-bit.
          m_ucNextBits = 7;
          m_ulB       |= ULONG(dt) << (24 - m_ucBits);
          m_ucBits    += 8;
        } else {
          m_bMarker    = true;
          m_ucBits    += 8;
          break;
        }
      } else {
        // Bytestuffing.
        if (m_pIO->PeekWord() == 0xff00) {
          // Proper bytestuffing. Remove the zero-byte
          m_pIO->GetWord();
          if (m_pChk) {
            m_pChk->Update(0xff);
            m_pChk->Update(0x00);
          }
          m_ulB       |= ULONG(dt) << (24 - m_ucBits);
          m_ucBits    += 8;
        } else {
          // A marker. Do not advance over the marker, but
          // rather stay at it so the logic upwards can fix it.
          m_bMarker    = true;
          m_ucBits    += 8;
          break;
        }
      }
    } else if (dt == ByteStream::EOF) {
      m_bEOF       = true;
      m_ucBits    += 8;
    } else if (bitstuffing) {
      assert(m_ucNextBits == 8 || dt < 128); // was checked before.
      if (m_pChk) m_pChk->Update(dt);
      m_ulB       |= ULONG(dt) << (32 - m_ucNextBits - m_ucBits);
      m_ucBits    += m_ucNextBits;
      m_ucNextBits = 8;
    } else {
      if (m_pChk) m_pChk->Update(dt);
      m_ulB       |= ULONG(dt) << (24 - m_ucBits);
      m_ucBits    += 8;
    }
  } while(m_ucBits <= 24);
}
///

/// BitStream::ReportError
// Report an error if not enough bits were available, depending on
// the error flag.
template<bool bitstuffing>
void BitStream<bitstuffing>::ReportError(void)
{
  class Environ *m_pEnviron = m_pIO->EnvironOf();
  
  if (m_bEOF)
    JPG_THROW(UNEXPECTED_EOF,"BitStream::ReportError",
              "invalid stream, found EOF within entropy coded segment");
  if (m_bMarker)
    JPG_THROW(UNEXPECTED_EOF,"BitStream::ReportError",
              "invalid stream, found marker in entropy coded segment");
  
  JPG_THROW(MALFORMED_STREAM,"BitStream::ReportError",
            "invalid stream, found invalid huffman code in entropy coded segment");
}
///

/// Explicit instantiations
template class BitStream<true>;
template class BitStream<false>;
///

