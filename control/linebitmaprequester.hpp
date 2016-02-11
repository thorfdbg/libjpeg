/*
**
** This class pulls single lines from the frame and reconstructs
** them from the codestream. Only the lossless scheme uses this buffer
** organization.
**
** $Id: linebitmaprequester.hpp,v 1.22 2015/03/11 16:02:42 thor Exp $
**
*/

#ifndef CONTROL_LINEARBITMAPREQUESTER_HPP
#define CONTROL_LINEARBITMAPREQUESTER_HPP

/// Includes
#include "control/bitmapctrl.hpp"
#include "control/linebuffer.hpp"
///

/// Forwards
class DCT;
class UpsamplerBase;
class DownsamplerBase;
class ColorTrafo;
struct Line;
///

/// class LineBitmapRequester
// This class pulls single lines from the frame and reconstructs
// them from the codestream. Only the lossless scheme uses this buffer
// organization.
class LineBitmapRequester : public LineBuffer, public BitmapCtrl {
  //
  class Environ             *m_pEnviron;
  class Frame               *m_pFrame;
  //
  //
  // Dimensions of the frame.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Number of components in the frame.
  UBYTE                      m_ucCount;
  //
  // Number of lines already in the input buffer on encoding.
  ULONG                     *m_pulReadyLines;
  // 
  // Downsampling operator.
  class DownsamplerBase    **m_ppDownsampler;
  //
  // And the inverse, if required.
  class UpsamplerBase      **m_ppUpsampler;
  //
  // Temporary bitmaps
  struct ImageBitMap       **m_ppTempIBM;
  //
  // Current position in reconstruction or encoding,
  // going through the color transformation.
  // On decoding, the line in here has the Y-coordinate 
  // in m_ulReadyLines.
  struct Line             ***m_pppImage;
  //
  // Temporary for decoding how many MCUs are ready on the next
  // iteration.can be pulled next.
  ULONG                      m_ulMaxMCU;
  //
  // True if subsampling is required.
  bool                       m_bSubsampling;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
  // Advance the image line pointer by the next eight lines
  // which is here a "pseudo"-MCU block.
  void Next8Lines(UBYTE c);
  //
  // Get the next block of eight lines of the image
  struct Line *Start8Lines(UBYTE c);
  //
public:
  //
  LineBitmapRequester(class Frame *frame);
  //
  virtual ~LineBitmapRequester(void); 
  //
  // First time usage: Collect all the information for encoding.
  // May throw on out of memory situations
  virtual void PrepareForEncoding(void);
  //
  // First time usage: Collect all the information for decoding.
  // May throw on out of memory situations.
  virtual void PrepareForDecoding(void);
  //
  // Return the color transformer responsible for this scan.
  class ColorTrafo *ColorTrafoOf(bool encoding);
  //
  // First step of a region encoder: Find the region that can be pulled in the next step,
  // from a rectangle request. This potentially shrinks the rectangle, which should be
  // initialized to the full image.
  virtual void CropEncodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *rr);
  //
  // Request user data for encoding for the given region, potentially clip the region to the
  // data available from the user.
  virtual void RequestUserDataForEncoding(class BitMapHook *bmh,RectAngle<LONG> &region,bool alpha);
  //
  // Pull data buffers from the user data bitmap hook
  virtual void RequestUserDataForDecoding(class BitMapHook *bmh,RectAngle<LONG> &region,
					  const struct RectangleRequest *rr,bool alpha);
  //
  // Encode a region, push it into the internal buffers and
  // prepare everything for coding.
  virtual void EncodeRegion(const RectAngle<LONG> &region);
  //
  // Reconstruct a block, or part of a block
  virtual void ReconstructRegion(const RectAngle<LONG> &region,const struct RectangleRequest *rr);
  //
  // Return true if the next MCU line is buffered and can be pushed
  // to the encoder.
  virtual bool isNextMCULineReady(void) const;
  //
  // Reset all components on the image side of the control to the
  // start of the image. Required when re-requesting the image
  // for encoding or decoding.
  virtual void ResetToStartOfImage(void); 
  //
  // Return an indicator whether all of the image has been loaded into
  // the image buffer.
  virtual bool isImageComplete(void) const; 
  //
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const
  {
    return true;
  }  
  //
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(const struct RectangleRequest *rr) const
  {
    return LineBuffer::BufferedLines(rr);
  }  
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    LineBuffer::PostImageHeight(lines);
    BitmapCtrl::PostImageHeight(lines);
    
    m_ulPixelHeight = lines;
  }
};
///

///
#endif
