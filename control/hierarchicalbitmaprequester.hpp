/*
**
** This is the top-level bitmap requester that distributes data to
** image scales on encoding, and collects data from image scales on
** decoding. It also keeps the top-level color transformer and the
** toplevel subsampling expander.
**
** $Id: hierarchicalbitmaprequester.hpp,v 1.14 2015/03/11 16:02:42 thor Exp $
**
*/

#ifndef CONTROL_HIERARCHICALBITMAPREQUESTER_HPP
#define CONTROL_HIERARCHICALBITMAPREQUESTER_HPP

/// Includes
#include "tools/environment.hpp"
#include "control/bitmapctrl.hpp"
///

/// Forwards
class DownsamplerBase;
class UpsamplerBase;
struct ImageBitMap;
class Frame;
class LineAdapter;
///

/// Class HierarchicalBitmapRequester
// This is the top-level bitmap requester that distributes data to
// image scales on encoding, and collects data from image scales on
// decoding. It also keeps the top-level color transformer and the
// toplevel subsampling expander.
class HierarchicalBitmapRequester : public BitmapCtrl {
  //
#if ACCUSOFT_CODE
  class DownsamplerBase    **m_ppDownsampler;
  //
  // And the inverse, if required.
  class UpsamplerBase      **m_ppUpsampler;
  //
  // Temporary bitmaps
  struct ImageBitMap       **m_ppTempIBM;
  //
  // The tree of line adapters. This here points to the smallest scale
  // containing the low-pass.
  class LineAdapter         *m_pSmallestScale;
  //
  // The largest scale of the tree, i.e. the end where more scales are
  // added by means of exp.
  class LineAdapter         *m_pLargestScale;
  //
  // Keeps dangling pointers on construction until the merger is build
  // which then controls the life-time.
  class LineAdapter         *m_pTempAdapter;
  //
  // Line counters how many lines have been already reconstructed.
  ULONG                     *m_pulReadyLines;
  //
  // Y counters, but in subsampled lines.
  ULONG                     *m_pulY;
  //
  // Height of each component, in subsampled components.
  ULONG                     *m_pulHeight;
  //
  // The current MCU block of lines allocated from the largest scale.
  struct Line              **m_ppEncodingMCU;
  //
  // The current MCU block being retrieved from the decoder.
  struct Line              **m_ppDecodingMCU;
  //
  // Internal status for requesting, keeps the number of MCUs ready.
  ULONG                      m_ulMaxMCU;
  //
  // True if subsampling is required.
  bool                       m_bSubsampling;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
  // Define a single 8x8 block starting at the x offset and the given
  // line, taking the input 8x8 buffer.
  void DefineRegion(LONG x,const struct Line *const *line,const LONG *buffer,UBYTE comp);
  //
  // Define a single 8x8 block starting at the x offset and the given
  // line, taking the input 8x8 buffer.
  void FetchRegion(LONG x,const struct Line *const *line,LONG *buffer);
  //
  // Get the next block of eight lines of the image, this is the encoding
  // side of the story.
  void Allocate8Lines(UBYTE c);
  //
  // Pull 8 lines from the top-level and place them into
  // the decoder MCU.
  void Pull8Lines(UBYTE c);
  //
  // Release the currently buffered decoder MCU for the given component.
  void Release8Lines(UBYTE c);
  //
  // Advance the image line pointer by the next eight lines
  // which is here a "pseudo"-MCU block. This is also an encoding
  // call.
  void Push8Lines(UBYTE c);
#endif
  //
public:
  // Construct from a frame - the frame is just a "dummy frame"
  // that contains the dimensions, actually a DHP marker segment
  // without any data in it.
  HierarchicalBitmapRequester(class Frame *dimensions);
  //
  ~HierarchicalBitmapRequester(void);
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
  // than blocks. Actually, this matters little as the entropy coding
  // backend talks to the adapters instead.
  virtual bool isLineBased(void) const
  {
    return true;
  }
  // 
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(const struct RectangleRequest *rr) const;
  //
  // As soon as a frame is parsed off, or created: Add another scale to the image.
  // The boolean arguments identify whether the reference frame, i.e. what is
  // buffered already from previous frames, will be expanded.
  void AddImageScale(class Frame *frame,bool expandh,bool expandv);
  //
  // After having written the previous image, compute the differential from the downscaled
  // and-re-upscaled version and push it into the next frame, collect the
  // residuals, make this frame ready for encoding, and retrieve the downscaling
  // data.
  void GenerateDifferentialImage(class Frame *target,bool &hexp,bool &vexp); 
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines);
};
///

///
#endif
