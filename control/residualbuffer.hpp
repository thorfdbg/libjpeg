/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** This class provides an access path to the residual image in the
** form consistent to the block buffer / buffer control class, except
** that all the regular accesses go to the residual part. It does not
** manage buffers itself, but requires a block bitmap requester as
** base to forward the requests to.
**
** $Id: residualbuffer.hpp,v 1.5 2016/10/28 13:58:53 thor Exp $
**
*/

#ifndef CONTROL_RESIDUALBUFFER_HPP
#define CONTROL_RESIDUALBUFFER_HPP

/// Includes
#include "control/bitmapctrl.hpp"
#include "control/bufferctrl.hpp"
#include "control/blockctrl.hpp"
#include "control/blockbitmaprequester.hpp"
///

/// class ResidualBuffer
// This class provides an access path to the residual image in the
// form consistent to the block buffer / buffer control class, except
// that all the regular accesses go to the residual part. It does not
// manage buffers itself, but requires a block bitmap requester as
// base to forward the requests to.
class ResidualBuffer : public BlockCtrl, public BufferCtrl {
  //
  // Pointer to the actual worker class that has the image.
  class BlockBitmapRequester *m_pParent;
  //
public:
  //
  // Construct the residual image. Requires the parent that keeps the actual buffer.
  ResidualBuffer(class BlockBitmapRequester *parent)
    : BlockCtrl(parent->EnvironOf()), BufferCtrl(parent->EnvironOf()), m_pParent(parent)
  { }
  //
  // Destructor: Nothing to destroy here.
  virtual ~ResidualBuffer(void)
  { }
  //
  //
  // Block buffer interface:
  //
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const
  {
    return false;
  }
  //
  // First time usage: Collect all the information for encoding.
  // May throw on out of memory situations
  virtual void PrepareForEncoding(void)
  {
    m_pParent->PrepareForEncoding();
  }
  //
  // First time usage: Collect all the information for decoding.
  // May throw on out of memory situations.
  virtual void PrepareForDecoding(void)
  {
    m_pParent->PrepareForDecoding();
  }
  //
  // Indicate the frame height after the frame has already been
  // started. This call makes the necessary adjustments to handle
  // the DNL marker which appears only after the scan.
  virtual void PostImageHeight(ULONG height)
  {
    m_pParent->PostImageHeight(height);
  }
  //
  //
  // Return the current top MCU quantized line.
  virtual class QuantizedRow *CurrentQuantizedRow(UBYTE comp)
  {
    // On purpose, forward to the residual row.
    return m_pParent->CurrentResidualRow(comp);
  }  
  //
  // Start a MCU scan by initializing the quantized rows for this row
  // in this scan.
  virtual bool StartMCUQuantizerRow(class Scan *scan)
  {
    // On purpose, forward to the start of the residuals, not
    // the legacy data.
    return m_pParent->StartMCUResidualRow(scan);
  }
  //
  // Make sure to reset the block control to the
  // start of the scan for the indicated components in the scan, 
  // required after collecting the statistics for this scan.
  virtual void ResetToStartOfScan(class Scan *scan)
  {
    m_pParent->ResetToStartOfScan(scan);
  }
};
///

///
#endif
