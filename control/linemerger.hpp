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
** This class merges the two sources of a differential frame together,
** expanding its non-differential source.
**
** $Id: linemerger.hpp,v 1.23 2015/03/25 08:45:43 thor Exp $
**
*/

#ifndef CONTROL_LINEMERGER_HPP
#define CONTROL_LINEMERGER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/line.hpp"
#include "control/lineadapter.hpp"
///

/// Class LineMerger
// This class merges the two sources of a differential frame together,
// expanding its non-differential source.
class LineMerger : public LineAdapter {
  //
#if ACCUSOFT_CODE
  // Frame this is part of - where it takes the dimensions from.
  // This is always the larger (highpass) frame.
  class Frame       *m_pFrame;
  //
  // The non-differential source that will be expanded on decoding.
  class LineAdapter *m_pLowPass;
  //
  // The differential source whose contents will be added to the above.
  class LineAdapter *m_pHighPass;
  // 
  // Temporary buffer for odd lines if vertical expansion is necessary.
  struct Line      **m_ppVBuffer;
  struct Line      **m_ppHBuffer;
  struct Line      **m_ppIBuffer; // Interpolated line.
  //
  // The image buffer. Keeps the data until the high-pass can be written.
  struct Line      **m_ppFirstLine;
  //
  // Pointer to the currently last line in an image
  struct Line     ***m_pppImage;
  //
  // Line pointers for the filtering, pointers to the
  // top,central and bottom line of the three-top filter
  struct Line      **m_ppTop;
  struct Line      **m_ppCenter;
  struct Line      **m_ppBottom;
  //
  // Dimensions of the subimages.
  ULONG             *m_pulPixelWidth;
  ULONG             *m_pulPixelHeight;
  //
  // Y-positions. Actually, only the even-odd part is important.
  ULONG             *m_pulY;
  //
  // Expansion flags in horizontal and vertical direction.
  bool               m_bExpandH;
  bool               m_bExpandV;
  //
  // Fetch a line from the low-pass filter and expand it in horizontal
  // or vertical direction. Do not do anything else.
  struct Line *GetNextExpandedLowPassLine(UBYTE comp);
  //
  // Fetch the next line from the low-pass and expand it horizontally if required.
  struct Line *GetNextLowpassLine(UBYTE comp);
#endif
  //
public:
  // The frame to create the line merger from is the highpass frame as
  // its line dimensions are identical to that of the required output.
  LineMerger(class Frame *frame,class LineAdapter *low,class LineAdapter *high,
             bool expandh,bool expandv);
  //
  virtual ~LineMerger(void);
  //
  // Second-stage constructor, construct the internal details.
  void BuildCommon(void);
  // 
  // First time usage: Collect all the information for encoding.
  // May throw on out of memory situations
  virtual void PrepareForEncoding(void)
  {
#if ACCUSOFT_CODE
    BuildCommon();
    m_pHighPass->PrepareForEncoding();
    m_pLowPass->PrepareForEncoding();
#endif
  }
  //
  // First time usage: Collect all the information for decoding.
  // May throw on out of memory situations.
  virtual void PrepareForDecoding(void)
  {
#if ACCUSOFT_CODE
    BuildCommon();
    m_pHighPass->PrepareForDecoding();
    m_pLowPass->PrepareForDecoding();
#endif
  }
  //
  // Return the next smaller scale adapter if there is any, or
  // NULL otherwise.
  virtual class LineAdapter *LowPassOf(void) const
  {
#if ACCUSOFT_CODE
    return m_pLowPass;
#else
    return NULL;
#endif
  }
  //
  // The high-pass end if there is one, or NULL.
  virtual class LineAdapter *HighPassOf(void) const
  {
#if ACCUSOFT_CODE
    return m_pHighPass;
#else
    return NULL;
#endif
  }
  //
  // Check whether a horizontal expansion is performed here.
  bool isHorizontallyExpanding(void) const
  {
#if ACCUSOFT_CODE
    return m_bExpandH;
#else
    return false;
#endif
  }
  //
  // Check whether a vertical expansion is performed here.
  bool isVerticallyExpanding(void) const
  {
#if ACCUSOFT_CODE
    return m_bExpandV;
#else
    return false;
#endif
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
  virtual void ReleaseLine(struct Line *line,UBYTE comp);
  //
  // Allocate the next line for encoding. This line must
  // later on then be pushed back into this buffer by
  // PushLine below.
  virtual struct Line *AllocateLine(UBYTE comp);
  //
  // Push the next line into the output buffer. If eight lines
  // are accumulated (or enough lines up to the end of the image)
  // these lines are automatically transfered to the input
  // buffer of the block based coding back-end.
  virtual void PushLine(struct Line *line,UBYTE comp);
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
  // Reset all components on the image side of the control to the
  // start of the image. Required when re-requesting the image
  // for encoding or decoding.
  virtual void ResetToStartOfImage(void); 
  //
  // Generate a differential image by pulling the reconstructed
  // image from the low-pass and pushing the differential signal
  // into the high-pass.
  void GenerateDifferentialImage(void); 
  //
  // Return an indicator whether all of the image has been loaded into
  // the image buffer.
  virtual bool isImageComplete(void) const
  {
#if ACCUSOFT_CODE
    // If and only if the lowpass is complete. The highpass is then generated
    // when done.
    return m_pLowPass->isImageComplete();
#else
    return true;
#endif
  } 
  //
  // Return true if the next MCU line is buffered and can be pushed
  // to the encoder.
  virtual bool isNextMCULineReady(void) const
  {
#if ACCUSOFT_CODE
    // Only if the lowpass is ready. The high-pass is then written
    // when done, but the Low-pass must be written first.
    return m_pLowPass->isNextMCULineReady();
#else
    return false;
#endif
  } 
  //
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(UBYTE comp) const
  {
#if ACCUSOFT_CODE
    // Since the high-pass is loaded last, it must be asked...
    return m_pHighPass->BufferedLines(comp);
#else
    NOREF(comp);
    return 0;
#endif
  }
  //
  // This does not really make any difference as this object is not used
  // directly by the back-end.
  virtual bool isLineBased(void) const
  {
    return true;
  }
  //  
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines);
  //
  // In case the high-pass has a DC offset in its data, deliver it here.
  virtual LONG DCOffsetOf(void) const
  {
    return 0; // none.
  }
  //
  // In case the high-pass is supposed to be a lossless process such that
  // we require exact differentials, return true.
  virtual bool isLossless(void) const
  {
#if ACCUSOFT_CODE
    return m_pHighPass->isLossless();
#else
    return false;
#endif
  }
};
///

///
#endif
