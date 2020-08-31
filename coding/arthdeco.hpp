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
 * The Art-Deco (MQ) decoder and encoder as specified by the jpeg2000 
 * standard, FDIS, Annex C
 *
 * $Id: arthdeco.hpp,v 1.16 2014/09/30 08:33:16 thor Exp $
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

/// Forwards
class Checksum;
///

/// MQCoder
// The coder itself.
#if ACCUSOFT_CODE
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
  // The checksum we keep updating.
  class Checksum   *m_pChk;
  //
  // A single context information
  struct MQContext {
    UBYTE m_ucIndex; // status in the index table
    bool  m_bMPS;    // most probable symbol
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
  void OpenForWrite(class ByteStream *io,class Checksum *chk);
  //
  // Open for reading.
  void OpenForRead(class ByteStream *io,class Checksum *chk);
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
#endif
///

///
#endif
