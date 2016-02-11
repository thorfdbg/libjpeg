/*
** This class allows to read individual bits from a stream of bytes.
** This class implements the bytestuffing as required.
**
** $Id: bitstream.cpp,v 1.8 2014/11/12 14:16:17 thor Exp $
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

/// Explicit instanciations
template class BitStream<true>;
template class BitStream<false>;
///

