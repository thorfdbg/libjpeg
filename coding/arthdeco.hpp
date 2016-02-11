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
