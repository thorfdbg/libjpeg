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
**
** This class represents the interface for parsing the
** entropy coded data in JPEG as part of a single scan.
**
** $Id: entropyparser.cpp,v 1.26 2024/11/05 06:39:25 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "marker/frame.hpp"
#include "codestream/tables.hpp"
#include "codestream/entropyparser.hpp"
#include "io/bytestream.hpp"
///

/// EntropyParser::EntropyParser
EntropyParser::EntropyParser(class Frame *frame,class Scan *scan)
  : JKeeper(scan->EnvironOf()), m_pScan(scan), m_pFrame(frame)
{
  m_ucCount = scan->ComponentsInScan();

  // The residual scan uses all components here, not just four, but
  // it does not require the component count either.
  for(volatile UBYTE i = 0;i < m_ucCount && i < 4;i++) {
    JPG_TRY {
      m_pComponent[i] = scan->ComponentOf(i);
    } JPG_CATCH {
      m_pComponent[i] = NULL;
    } JPG_ENDTRY;
  }

  m_ulRestartInterval   = m_pFrame->TablesOf()->RestartIntervalOf();
  m_usNextRestartMarker = 0xffd0;
  m_ulMCUsToGo          = m_ulRestartInterval;
  m_bSegmentIsValid     = true;
  m_bScanForDNL         = (m_pFrame->HeightOf() == 0)?true:false;
  m_bDNLFound           = false;
}
///

/// EntropyParser::StartWriteScan
// Write the marker to the stream.
void EntropyParser::StartWriteScan(class ByteStream *,class Checksum *,class BufferCtrl *)
{
  // Reset the restart marker count.
  m_ulRestartInterval   = m_pFrame->TablesOf()->RestartIntervalOf();
  m_usNextRestartMarker = 0xffd0;
  m_ulMCUsToGo          = m_ulRestartInterval;
}
///

/// EntropyParser::~EntropyParser
EntropyParser::~EntropyParser(void)
{
}
///

/// EntropyParser::WriteRestartMarker
// Flush the scan statistics, write the restart marker,
// reset the MCU counter.
void EntropyParser::WriteRestartMarker(class ByteStream *io)
{
  Flush(false);
  if (io) {
    io->PutWord(m_usNextRestartMarker);
    m_usNextRestartMarker = (m_usNextRestartMarker + 1) & 0xfff7;
  }
  m_ulMCUsToGo          = m_ulRestartInterval;
}
///

/// EntropyParser::ParseRestartMarker
// Parse the restart marker or resync at the restart marker.
void EntropyParser::ParseRestartMarker(class ByteStream *io)
{
  LONG dt = io->PeekWord();
  
  while(dt == 0xffff) {
    // Found a filler byte. Skip over and try again.
    io->Get();
    dt = io->PeekWord();
  }
  
  if (dt == 0xffdc && m_bScanForDNL) {
    ParseDNLMarker(io);
  } else if (dt == m_usNextRestartMarker) {
    // Everything worked fine! Continue going after removing the marker.
    io->GetWord();
    Restart();
    m_usNextRestartMarker = (m_usNextRestartMarker + 1) & 0xfff7;
    m_ulMCUsToGo          = m_ulRestartInterval;
    m_bSegmentIsValid     = true;
  } else {
    JPG_WARN(MALFORMED_STREAM,"EntropyParser::ParseRestartMarker",
             "entropy coder is out of sync, trying to advance to the next marker");
    // As said...
    //
    do {
      dt = io->Get();
      if (dt == ByteStream::EOF) {
        // Outch, run completely out of data.
        JPG_THROW(UNEXPECTED_EOF,"EntropyParser::ParseRestartMarker",
                  "run into end of file while trying to resync the entropy parser");
        //
        // Code never goes here...
        return;
      } else if (dt == 0xff) {
        // Could be a marker.
        io->LastUnDo();
        dt = io->PeekWord();
        // Depends now on the marker.
        if (dt >= 0xffd0 && dt < 0xffd8) {
          // Is a restart marker. If this is the correct one, just leave,
          // the entropy coder was behind and we are then again up at the
          // correct index.
          if (dt == m_usNextRestartMarker) {
            io->GetWord();
            Restart();
            m_usNextRestartMarker = (m_usNextRestartMarker + 1) & 0xfff7;
            m_ulMCUsToGo          = m_ulRestartInterval;
            m_bSegmentIsValid     = true;
            return;
          } else if (((dt - m_usNextRestartMarker) & 0x07) >= 4) {
            // Here dt is *likely* behind, i.e. we need to skip more
            // data to advance to the correct restart marker.
            io->GetWord();
            // Remove the marker and keep going.
          } else {
            // Here dt is likely ahead, that is, the entropy decoder
            // should better skip the next entropy coded segment
            // completely and then should re-enter to re-examine whether
            // the marker fits. Keep the marker in the stream, then, but
            // do not continue to decode.
            m_bSegmentIsValid     = false;
            m_usNextRestartMarker = (m_usNextRestartMarker + 1) & 0xfff7;
            m_ulMCUsToGo          = m_ulRestartInterval;
            // Do not run into a restart as this may pull bytes.
            return;
          }
        } else if (dt >= 0xffc0 && dt < 0xfff0) {
          // Is apparently some other marker, i.e. we are at the end of
          // the segment. Continue skipping until the end is reached and
          // the parser run out of fun...
          m_bSegmentIsValid     = false;
          m_usNextRestartMarker = (m_usNextRestartMarker + 1) & 0xfff7;
          m_ulMCUsToGo          = m_ulRestartInterval;
          // Do not run into a restart as this may pull bytes.
          return;
        } else {
          // Some garbadge data, or a 0xff00. Just eat it up, and continue
          // scanning. Note that a single Get is used here to eventually
          // skip over a "fill byte".
          io->Get();
        }
      }
    } while(true);
  }
}
///

/// EntropyParser::ParseDNLMarker
// Parse the DNL marker, update the frame height. If
// the result is true, the marker has been found.
bool EntropyParser::ParseDNLMarker(class ByteStream *io)
{
  LONG dt;

  if (m_bDNLFound)
    return true;
  
  dt = io->PeekWord();

  while(dt == 0xffff) {
    // A filler byte followed by the marker (hopefully). Skip the
    // filler and try again.
    io->Get();
    dt = io->PeekWord();
  }

  if (dt == 0xffdc) {
    dt = io->GetWord();
    dt = io->GetWord();
    if (dt != 4)
      JPG_THROW(MALFORMED_STREAM,"EntropyParser::ParseDNLMarker",
                "DNL marker size is out of range, must be exactly four bytes long");
    
    dt = io->GetWord();
    if (dt == ByteStream::EOF)
      JPG_THROW(UNEXPECTED_EOF,"EntropyParser::ParseDNLMarker",
                "stream is truncated, could not read the DNL marker");
    if (dt == 0)
      JPG_THROW(MALFORMED_STREAM,"EntropyParser::ParseDNLMarker",
                "frame height as indicated by the DNL marker is corrupt, must be > 0");

    //
    // Let the parser itself know what the height is now.
    PostImageHeight(dt);
    // Forward to the frame.
    m_pFrame->PostImageHeight(dt);

    m_bDNLFound = true;
    return true;
  } else {
    return false;
  }
}
///

/// EntropyParser::FractionalColorBitsOf
// Return the number of fractional bits due to the color
// transformation.
UBYTE EntropyParser::FractionalColorBitsOf(void) const
{
  return m_pFrame->TablesOf()->FractionalColorBitsOf(m_pFrame->DepthOf(),m_pFrame->isDCTBased());
}
///
