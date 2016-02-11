/*
** This class contains the marker that defines the JPEG LS thresholds.
**
** $Id: thresholds.cpp,v 1.8 2014/09/30 08:33:17 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/thresholds.hpp"
#include "io/bytestream.hpp"
///

/// Thresholds::Thresholds
Thresholds::Thresholds(class Environ *env)
  : JKeeper(env), m_usMaxVal(255), 
    m_usT1(3), m_usT2(7), m_usT3(21), m_usReset(64)
{
}
///

/// Thresholds::~Thresholds
Thresholds::~Thresholds(void)
{
}
///

/// Thresholds::WriteMarker
// Write the marker contents to a LSE marker.
void Thresholds::WriteMarker(class ByteStream *io)
{
  io->PutWord(13); // length including the length byte
  io->Put(1);      // ID of the LSE marker
  io->PutWord(m_usMaxVal);
  io->PutWord(m_usT1);
  io->PutWord(m_usT2);
  io->PutWord(m_usT3);
  io->PutWord(m_usReset);
}
///

/// Thresholds::ParseMarker
// Parse the marker contents of a LSE marker.
void Thresholds::ParseMarker(class ByteStream *io,UWORD len)
{
  if (len != 13)
    JPG_THROW(MALFORMED_STREAM,"Thresholds::ParseMarker","LSE marker length is invalid");
  
  m_usMaxVal = io->GetWord();
  m_usT1     = io->GetWord();
  m_usT2     = io->GetWord();
  m_usT3     = io->GetWord();
  m_usReset  = io->GetWord();
}
///

/// Thresholds::InstallDefaults
// Install the defaults for a given bits per pixel value.
void Thresholds::InstallDefaults(UBYTE bpp,UWORD near)
{
  m_usMaxVal = (1 << bpp) - 1;
  
  if (m_usMaxVal >= 128) {
    UWORD factor = m_usMaxVal;
    if (factor > 4095)
      factor = 4095;
    factor = (factor + 128) >> 8;
    m_usT1 = factor * ( 3 - 2) + 2 + 3 * near;
    if (m_usT1 > m_usMaxVal || m_usT1 < near + 1) m_usT1 = near + 1;
    m_usT2 = factor * ( 7 - 3) + 3 + 5 * near;
    if (m_usT2 > m_usMaxVal || m_usT2 < m_usT1  ) m_usT2 = m_usT1;
    m_usT3 = factor * (21 - 4) + 4 + 7 * near;
    if (m_usT3 > m_usMaxVal || m_usT3 < m_usT2  ) m_usT3 = m_usT2;
  } else {
    UWORD factor = 256 / (m_usMaxVal + 1);
    m_usT1 = 3 / factor + 3 * near;
    if (m_usT1 < 2) m_usT1 = 2;
    if (m_usT1 > m_usMaxVal || m_usT1 < near + 1) m_usT1 = near + 1;
    m_usT2 = 7 / factor + 5 * near;
    if (m_usT2 < 3) m_usT2 = 3;
    if (m_usT2 > m_usMaxVal || m_usT2 < m_usT1  ) m_usT2 = m_usT1;
    m_usT3 = 21 / factor + 7 * near;
    if (m_usT3 < 4) m_usT3 = 4;
    if (m_usT3 > m_usMaxVal || m_usT3 < m_usT2  ) m_usT3 = m_usT2;
  }
  m_usReset  = 64;
}
///
