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
** This class adapts to a block buffer in a way that allows the user
** to pull out (or push in) individual lines instead of blocks. The
** purpose of this class is to implement a line-based upsampling or
** downsampling filter for the hierarchical mode. This class does not
** implement a color transformer or a upsampling filter (in the usual sense)
**
** $Id: blocklineadapter.hpp,v 1.22 2016/01/06 14:43:23 thor Exp $
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
  // The current (worked on) line of blocks.
  class QuantizedRow ***m_pppQImage;
  //
  // Current row to be delivered on decoding, one per component.
  // If this runs out of lines, more must be pulled from the
  // scan.
  struct Line        ***m_pppImage;
  //
  // The number of lines already pushed into the image.
  ULONG                *m_pulReadyLines;
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
  //
  // In case the high-pass is supposed to be a lossless process such that
  // we require exact differentials, return true.
  virtual bool isLossless(void) const
  {
    return false;
  }
};
///

///
#endif
