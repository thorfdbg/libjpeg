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
** This class adapts to a block buffer in a way that allows the user
** to pull out (or push in) individual lines instead of blocks. The
** purpose of this class is to implement a line-based upsampling or
** downsampling filter for the hierarchical mode. This class does not
** implement a color transformer or a upsampling filter (in the usual sense)
**
** $Id: blocklineadapter.hpp,v 1.15 2012-09-10 12:46:33 thor Exp $
**
*/

#ifndef CONTROL_BLOCKLINEADAPTER_HPP
#define CONTROL_BLOCKLINEADAPTER_HPP

/// Includes
#include "control/bitmapctrl.hpp"
#include "control/blockbuffer.hpp"
#include "control/lineadapter.hpp"
///

/// Forwards
struct Line;
class ByteStream;
///

/// class BlockLineAdapter
// This class adapts to a block buffer in a way that allows the user
// to pull out (or push in) individual lines instead of blocks. The
// purpose of this class is to implement a line-based upsampling or
// downsampling filter for the hierarchical mode. This class does not
// implement a color transformer or a upsampling filter (in the usual sense)
class BlockLineAdapter : public BlockBuffer, public LineAdapter {
  //
  class Environ        *m_pEnviron;
  //
  class Frame          *m_pFrame;
  //
  // Lines buffered here.
  struct Line         **m_ppTop;
  // 
  // Lines no longer required and ready for recycling.
  struct Line         **m_ppFree;
  //
  // The current (worked on) line of blocks.
  class QuantizedRow ***m_pppQImage;
  //
  // Current row to be delivered on decoding, one per component.
  // If this runs out of lines, more must be pulled from the
  // scan.
  struct Line        ***m_pppImage;
  //
  // The Y position of the line pointed to by the above.
  ULONG                *m_pulImageLine;
  //
  // The number of lines already pushed into the image.
  ULONG                *m_pulReadyLines;
  //
  // The number of pixels allocated per component. Required for
  // memory management purposes.
  ULONG                *m_pulPixelsPerLine;
  //
  // The nominal number of pixels per component. May be smaller
  // than the above, but counts the official number of samples
  // present as specified by the standard.
  ULONG                *m_pulPixelsPerComponent;
  //
  // The number of lines allocated per component.
  ULONG                *m_pulLinesPerComponent;
  //
  // Number of components adminstrated here. This is always the
  // full number of components in a frame as the hierachical process
  // is not limited to a single scan.
  UBYTE                 m_ucCount; 
  //
  // The block buffer "Buffered lines" does not return a useful value
  // as it expands subsampling.
  virtual ULONG BufferedLines(const RectangleRequest*) const
  {
    JPG_THROW(NOT_IMPLEMENTED,"BlockLineAdapter::BufferedLines",NULL);
    return 0;
  }
  //
  // Allocate all the buffers.
  void BuildCommon(void);
  //
public:
  //
  BlockLineAdapter(class Frame *frame);
  //
  virtual ~BlockLineAdapter(void); 
  // 
  // First time usage: Collect all the information for encoding.
  // May throw on out of memory situations
  virtual void PrepareForEncoding(void)
  {
    BuildCommon();
    BlockBuffer::ResetToStartOfScan(NULL);
  }
  //
  // First time usage: Collect all the information for decoding.
  // May throw on out of memory situations.
  virtual void PrepareForDecoding(void)
  {
    BuildCommon();
  }
  //
  // Get the next available line from the output
  // buffer on reconstruction. The caller must make
  // sure that the buffer is really loaded up to the
  // point or the line will be neutral grey.
  virtual struct Line *GetNextLine(UBYTE comp);
  //
  // Release the line as soon as it is no longer required - this
  // step goes after GetNextLine on the client.
  virtual void ReleaseLine(struct Line *line,UBYTE comp)
  {
    FreeLine(line,comp);
  }
  //
  // Allocate the next line for encoding. This line must
  // later on then be pushed back into this buffer by
  // PushLine below.
  virtual struct Line *AllocateLine(UBYTE comp);
  //
  // In case an allocated line shall be destroyed, call
  // this instead of ReleaseLine. The allocation strategy on
  // encoding and decoding might be different, and this is
  // the encoding release.
  virtual void DropLine(struct Line *line,UBYTE comp)
  {
    FreeLine(line,comp);
  }
  //
  // Push the next line into the output buffer. If eight lines
  // are accumulated (or enough lines up to the end of the image)
  // these lines are automatically transfered to the input
  // buffer of the block based coding back-end.
  virtual void PushLine(struct Line *line,UBYTE comp);
  //
  // Reset all components on the image side of the control to the
  // start of the image. Required when re-requesting the image
  // for encoding or decoding.
  virtual void ResetToStartOfImage(void); 
  //
  // Return true if the next MCU line is buffered and can be pushed
  // to the encoder. Note that the data here is *not* subsampled.
  virtual bool isNextMCULineReady(void) const;
  //
  // Return an indicator whether all of the image has been loaded into
  // the image buffer.
  virtual bool isImageComplete(void) const; 
  //
  // Returns the number of lines buffered for the given component.
  // Note that subsampling expansion has not yet taken place here,
  // this is to be done top-level.
  ULONG BufferedLines(UBYTE comp) const;
  //
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const
  {
    return false;
  }
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines);
  //
  // In case the high-pass has a DC offset in its data, deliver it here.
  virtual LONG DCOffsetOf(void) const;
};
///

///
#endif
