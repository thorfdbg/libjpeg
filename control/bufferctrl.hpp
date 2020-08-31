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
