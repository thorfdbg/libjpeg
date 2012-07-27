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
 * The Art-Deco (MQ) decoder and encoder as specified by the jpeg2000 
 * standard, FDIS, Annex C
 *
 * $Id: arthdeco.hpp,v 1.8 2012-06-02 10:27:13 thor Exp $
 *
 */

#ifndef ARTHDECO_HPP
#define ARTHDECO_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "std/string.hpp"
///

/// MQCoder
// The coder itself.
class MQCoder : public JObject {
  //
public:
  // Context definitions
  enum {
    Zero       = 0,
    Magnitude  = 1,
    Sign       = 11,
    FullZero   = 16,
    Count      = 17 // the number of contexts, not a context label.
  };
  //
private:
  //
  // The coding interval size
  ULONG             m_ulA;
  //
  // The computation register
  ULONG             m_ulC;
  //
  // The bit counter.
  UBYTE             m_ucCT;
  //
  // The output register
  UBYTE             m_ucB;
  //
  // Coding flags.
  UBYTE             m_bF;
  //
  // The bytestream we code from or code into.
  class ByteStream *m_pIO;
  //
  // A single context information
  struct MQContext {
    UBYTE m_ucIndex; // status in the index table
    UBYTE m_bMPS;    // most probable symbol
  }       m_Contexts[Count];
  //
  // Qe probability estimates.
  static const UWORD Qe_Value[];
  //
  // MSB/LSB switch flag.
  static const bool  Qe_Switch[];
  //
  // Next state for MPS coding.
  static const UBYTE Qe_NextMPS[];
  //
  // Next state for LPS coding.
  static const UBYTE Qe_NextLPS[];
  //
  // Initialize the contexts
  void InitContexts(void);
  //
public:
  //
  MQCoder(void)
  {
    // Nothing really here.
  }
  //
  ~MQCoder(void)
  {
  }
  //
  // Open for writing.
  void OpenForWrite(class ByteStream *io);
  //
  // Open for reading.
  void OpenForRead(class ByteStream *io);
  //
  // Read a single bit from the MQ coder in the given context.
  bool Get(UBYTE ctxt);
  //
  // Write a single bit.
  void Put(UBYTE ctxt,bool bit);
  //
  // Flush out the remaining bits. This must be called before completing
  // the MQCoder write.
  void Flush();
  //
};
///

///
#endif
