/*
**
** This is a common subclass of both the class controlling the
** hierarchical process, the LineAdapter, as well as the class controlling
** direct communication with the user buffer, the BitmapCtrl.
** It only exists for casting between the two and to the target class,
** the line or block buffer.
**
** $Id: bufferctrl.hpp,v 1.9 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CONTROL_BUFFERCTRL_HPP
#define CONTROL_BUFFERCTRL_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Class BufferCtrl
// This is a common subclass of both the class controlling the
// hierarchical process, the LineAdapter, as well as the class controlling
// direct communication with the user buffer, the BitmapCtrl.
// It only exists for casting between the two and to the target class,
// the line or block buffer.
class BufferCtrl : public JKeeper {
  //
  // Nothing in it.
  //
public:
  BufferCtrl(class Environ *env)
    : JKeeper(env)
  { }
  //
  virtual ~BufferCtrl(void)
  { }
  //  
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const = 0;
  //
  // First time usage: Collect all the information for encoding.
  // May throw on out of memory situations
  virtual void PrepareForEncoding(void) = 0;
  //
  // First time usage: Collect all the information for decoding.
  // May throw on out of memory situations.
  virtual void PrepareForDecoding(void) = 0;
  //
  // Indicate the frame height after the frame has already been
  // started. This call makes the necessary adjustments to handle
  // the DNL marker which appears only after the scan.
  virtual void PostImageHeight(ULONG height) = 0;
};
///

///
#endif
