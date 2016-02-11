/*
**
** This class pulls single lines from the frame and reconstructs
** them from the codestream. Only the lossless scheme uses this buffer
** organization.
**
** $Id: linebuffer.hpp,v 1.11 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CONTROL_LINEBUFFER_HPP
#define CONTROL_LINEBUFFER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class DCT;
class UpsamplerBase;
class DownsamplerBase;
class ColorTrafo;
struct Line;
///

/// class LineBuffer
// This class pulls single lines from the frame and reconstructs
// them from the codestream. Only the lossless scheme uses this buffer
// organization.
class LineBuffer : public JKeeper {
  //
  // The frame this is part of.
  class Frame               *m_pFrame;
  //
protected:
  //
  // Dimensions of the frame.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Number of components in the frame.
  UBYTE                      m_ucCount;
  //
  // Next line to be processed.
  ULONG                     *m_pulY;
  //
  // Number of the topmost line.
  ULONG                     *m_pulCurrentY;
  //
  // Number of pixels allocated per component.
  ULONG                     *m_pulWidth;
  //
  // Length of a line in valid pixels.
  ULONG                     *m_pulEnd;
  //
  // Lines: 
  // Top row (start of the image buffer),
  // current encoding or decoding position for the codestream
  // parsers, current encoding or decoding position for the user
  // color transformer.
  //
  // First quantized image data row
  struct Line              **m_ppTop;
  //
  // Current position in stream parsing or writing.
  struct Line             ***m_pppCurrent;
  //
  // Previous line, required for the predictor.
  struct Line              **m_ppPrev;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
public:
  //
  LineBuffer(class Frame *frame);
  //
  virtual ~LineBuffer(void); 
  //
  // Return the current top MCU quantized line.
  struct Line *CurrentLineOf(UBYTE comp)
  {
    assert(comp < m_ucCount);
    return *m_pppCurrent[comp];
  }
  //
  // Return the previous line on top of the current.
  struct Line *PreviousLineOf(UBYTE comp)
  {
    assert(comp < m_ucCount);
    return m_ppPrev[comp];
  }
  //
  // Return the current Y coordinate.
  ULONG CurrentYOf(UBYTE comp) const
  {
    assert(comp < m_ucCount);
    return m_pulCurrentY[comp];
  }

  //
  // Start a MCU scan by initializing the quantized rows for this row
  // in this scan.
  virtual bool StartMCUQuantizerRow(class Scan *scan);
  //
  // The same for a row of residuals.
  virtual bool StartMCUResidualRow(void);
  //
  // Return the number of lines available for reconstruction from this scan.
  virtual ULONG BufferedLines(const struct RectangleRequest *rr) const;
  //
  // Make sure to reset the block control to the
  // start of the scan for the indicated components in the scan, 
  // required after collecting the statistics for this scan.
  virtual void ResetToStartOfScan(class Scan *scan);
  //
  // Return true in case this buffer is organized in lines rather
  // than blocks.
  virtual bool isLineBased(void) const
  {
    return true;
  }
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    m_ulPixelHeight = lines;
  }
  //
  // Define a single 8x8 block starting at the x offset and the given
  // line, taking the input 8x8 buffer.
  void DefineRegion(LONG x,struct Line *line,const LONG *buffer,UBYTE comp);
  //
  // Define a single 8x8 block starting at the x offset and the given
  // line, taking the input 8x8 buffer.
  void FetchRegion(LONG x,const struct Line *line,LONG *buffer); 
};
///

///
#endif
