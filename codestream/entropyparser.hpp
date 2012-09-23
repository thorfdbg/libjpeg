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
**
** This class represents the interface for parsing the
** entropy coded data in JPEG as part of a single scan.
**
** $Id: entropyparser.hpp,v 1.12 2012-09-22 20:51:40 thor Exp $
**
*/


#ifndef CODESTREAM_ENTROPYPARSER_HPP
#define CODESTREAM_ENTROPYPARSER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "std/assert.hpp"
///

/// Forwards
class ByteStream;
class Scan;
class Frame;
class BitmapCtrl;
class LineAdapter;
class BufferCtrl;
///

/// class EntropyParser
// This class represents the interface for parsing the
// entropy coded data in JPEG as part of a single scan.
class EntropyParser : public JKeeper {
  // 
  // The restart interval in MCUs
  UWORD                 m_usRestartInterval;
  //
  // The next restart marker expected or to be written.
  UWORD                 m_usNextRestartMarker;
  //
  // Number of MCUs to be handled before the next MCU
  // is to be written. If this becomes zero before a MCU
  // the restart marker will be written.
  UWORD                 m_usMCUsToGo;
  //
  // Boolean indicator whether the next entropy coded segment
  // up to the next restart marker or SOF/SOS is valid. If true,
  // continue parsing. Otherwise, replace just be zero/grey.
  bool                  m_bSegmentIsValid;
  //
  // Must scan for the DNL marker because the frame does not 
  // know our size.
  bool                  m_bScanForDNL;
  //
  // Set if parsing has come to an halt because DNL has been hit.
  bool                  m_bDNLFound;
  //
  // Flush the entropy coder, write the restart marker and
  // restart the MCU counter.
  void WriteRestartMarker(class ByteStream *io);
  //
  // Parse the restart marker or resync at the restart marker.
  void ParseRestartMarker(class ByteStream *io);
  //
  // Parse the DNL marker, update the frame height. If the
  // return is true, the DNL marker has been found.
  bool ParseDNLMarker(class ByteStream *io);
  //
protected:
  //
  // The scan this is part of.
  class Scan           *m_pScan;
  //
  // The frame we are part of.
  class Frame          *m_pFrame;
  //
  // The components in the scan - at most four.
  class Component      *m_pComponent[4];
   //
  // The number of components we have here.
  UBYTE                 m_ucCount;
  //
  // Create a new parser.
  EntropyParser(class Frame *frame,class Scan *scan);
  //
  // Return the number of fractional bits due to color
  // transformation.
  UBYTE FractionalColorBitsOf(void) const;
  //
  // Start writing a new MCU. Potentially, a restart marker
  // is emitted.
  void BeginWriteMCU(class ByteStream *io)
  {
    if (m_usRestartInterval) {
      if (m_usMCUsToGo == 0) {
	WriteRestartMarker(io);
      }
      m_usMCUsToGo--;
    }
  }
  //
  // Start parsing/reading a MCU. Might expect a restart marker.
  // If so, Restart() (below) is called. Returns true if the next MCU
  // is valid, or false if the parser should replace the next MCU with
  // grey.
  bool BeginReadMCU(class ByteStream *io)
  {   
    if (m_bScanForDNL) {
      if (ParseDNLMarker(io))
	return false;
    }
    if (m_usRestartInterval) {
      if (m_usMCUsToGo == 0) {
	ParseRestartMarker(io);
      }
      m_usMCUsToGo--;
    } 
    return m_bSegmentIsValid;
  }
  //
public:
  //
  virtual ~EntropyParser(void);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final) = 0;
  //
  // Restart the statistics/prediction at the next restart marker on reading.
  virtual void Restart(void) = 0;
  //
  // Return the i'th component of the scan.
  class Component *ComponentOf(UBYTE i)
  {
    assert(i < 4);
    return m_pComponent[i];
  }
  //
  // Return the number of the components in the scan.
  UBYTE ComponentsInScan(void) const
  {
    return m_ucCount;
  }
  //
  // Parse the marker contents.
  virtual void StartParseScan(class ByteStream *io,class BufferCtrl *ctrl) = 0;
  //
  // Write the marker to the stream.
  virtual void StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl) = 0;
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io) = 0;
  //
  // Start making a measurement run to optimize the
  // huffman tables.
  virtual void StartMeasureScan(class BufferCtrl *) = 0;
  //
  // Start a MCU scan.
  virtual bool StartMCURow(void) = 0;  
  //
  // Parse a single MCU in this scan.
  virtual bool ParseMCU(void) = 0;
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void) = 0;
};
///

///
#endif
