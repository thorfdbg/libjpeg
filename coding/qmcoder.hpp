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
** This class implements the binary arithmetic coder used in
** Recommendation ITU-T T.81 | ISO/IEC 10918-1.
**
** $Id: qmcoder.hpp,v 1.20 2018/07/27 06:56:43 thor Exp $
**
*/

#ifndef CODING_QMCODER_HPP
#define CODING_QMCODER_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "io/bytestream.hpp"
#include "std/string.hpp"
///

/// Defines
#undef  DEBUG_QMCODER
#define FAST_QMCODER
///

/// Forwards
class QMCoder;
class Checksum;
///

/// QMContext
// A context bin of the QM coder
#if ACCUSOFT_CODE
class QMContext : public JObject {
  friend class QMCoder;
  UBYTE m_ucIndex; // status in the index table
  bool  m_bMPS;    // most probable symbol
  //
#ifdef DEBUG_QMCODER
  // The ID of the QM Coder as four characters
public:
  UBYTE m_ucID[4];
#endif
  //
public:
  void Init(void)
  {
    m_ucIndex = 0;
    m_bMPS    = false;
  }
  //
  void Init(UBYTE state)
  {
    m_ucIndex = state;
    m_bMPS    = false;
  }
  //
  void Init(UBYTE state,bool mps)
  {
    m_ucIndex = state;
    m_bMPS    = mps;
  }
  //
#ifdef DEBUG_QMCODER
  void Init(const char *name)
  {
    Init();
    memcpy(m_ucID,name,4);
  }
  //
  void Init(UBYTE state,const char *name)
  {
    Init(state);
    memcpy(m_ucID,name,4);
  }
  //
  void Print(void)
  {
    if ((m_ucIndex >= 9  && m_ucIndex <= 13) ||
        (m_ucIndex >= 72 && m_ucIndex <= 77) ||
        (m_ucIndex >= 99 && m_ucIndex <= 11)) {
      printf("%c%c%c%c : %d(%d)\n",m_ucID[0],m_ucID[1],m_ucID[2],m_ucID[3],m_ucIndex,m_bMPS);
    }
  }
#endif
};
#endif
///

/// QMCoder
// The coder itself.
#if ACCUSOFT_CODE
class QMCoder : public JObject {
  //
  // The coding interval size
  ULONG             m_ulA;
  //
  // For decoding: The short version of it.
  UWORD             m_usA;
  //
  // The computation register
  ULONG             m_ulC;
  //
  // The MSBs of C for decoding only.
  UWORD             m_usC;
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
  // Count delayed 0xff.
  UWORD             m_usST;
  //
  // Count delayed 0x00 - this is not stricly required,
  // but simplifies the flushing process where trailing
  // 0x00's shall be discarded.
  UWORD             m_usSZ;
  //
  // The bytestream we code from or code into.
  class ByteStream *m_pIO;
  //
  // The checksum we keep updating.
  class Checksum   *m_pChk;
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
  // Flush the upper bits of the computation register.
  void ByteOut(void);
  //
  // Fill the byte input buffer
  void ByteIn(void);
  //
#ifdef FAST_QMCODER
  //
  // Read a single bit from the MQ coder in the given context.
  bool GetSlow(class QMContext &ctxt);
  //
  // Write a single bit.
  void PutSlow(class QMContext &ctxt,bool bit);
  //
#endif
  //
public:
  //
  QMCoder(void)
  {
    // Nothing really here.
  }
  //
  ~QMCoder(void)
  {
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
  // Open for writing.
  void OpenForWrite(class ByteStream *io,class Checksum *chk);
  //
  // Open for reading.
  void OpenForRead(class ByteStream *io,class Checksum *chk);
  //
#ifndef FAST_QMCODER
  //
  // Read a single bit from the MQ coder in the given context.
  bool Get(class QMContext &ctxt);
  //
  // Write a single bit.
  void Put(class QMContext &ctxt,bool bit);
#else
  //
  bool Get(class QMContext &ctxt)
  {  
    UWORD q = Qe_Value[ctxt.m_ucIndex];

    m_usA -= q;
    if (((WORD)m_usA < 0) && m_usC < m_usA) {
      // Short MPS case
      return ctxt.m_bMPS;
    } 
    return GetSlow(ctxt);
  }
  //
  //
  void Put(class QMContext &ctxt,bool bit)
  { 
    ULONG q = Qe_Value[ctxt.m_ucIndex];

    m_ulA  -= q;
    // Check for MPS and LPS coding
    if ((m_ulA & 0x8000) && bit == ctxt.m_bMPS) {
      // Short MPS case
      return;
    } else {
      PutSlow(ctxt,bit);
    }
  }
#endif
  //
  // Flush out the remaining bits. This must be called before completing
  // the MQCoder write.
  void Flush();
  //
  // The uniform state
  enum {
    Uniform_State = 113
  };
};
#endif
///

///
#endif
