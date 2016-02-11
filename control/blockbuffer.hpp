/*
**
** This class pulls blocks from the frame and reconstructs from those
** quantized block lines or encodes from them.
**
** $Id: blockbuffer.hpp,v 1.13 2015/05/10 15:21:43 thor Exp $
**
*/

#ifndef CONTROL_BLOCKBUFFER_HPP
#define CONTROL_BLOCKBUFFER_HPP

/// Includes
#include "tools/environment.hpp"
#include "control/blockctrl.hpp"
///

/// Forwards
class DCT;
class UpsamplerBase;
class DownsamplerBase;
class ColorTrafo;
class QuantizedRow;
class ResidualBlockHelper;
///

/// class BlockBuffer
// This class pulls blocks from the frame and reconstructs from those
// quantized block lines or encodes from them.
class BlockBuffer : public BlockCtrl {
  //
  class Frame               *m_pFrame;
  //
protected:
  //
  // Dimensions of the frame.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Number of components.
  UBYTE                      m_ucCount;
  //
  // Next line to be processed.
  ULONG                     *m_pulY;
  //
  // Number of the topmost line currently represented by the 
  // quantizer buffer line.
  ULONG                     *m_pulCurrentY;
  //
  // The DCT for encoding or decoding, together with the quantizer.
  class DCT                **m_ppDCT; 
  //
  // Quantized rows: 
  // Top row (start of the image buffer),
  // current encoding or decoding position for the codestream
  // parsers, current encoding or decoding position for the user
  // color transformer.
  //
  // First quantized image data row
  class QuantizedRow       **m_ppQTop;
  //
  // First residual data row.
  class QuantizedRow       **m_ppRTop;
  //
  // Current position in stream parsing or writing.
  class QuantizedRow      ***m_pppQStream;
  //
  // Current position in stream parsing for the residual.
  class QuantizedRow      ***m_pppRStream;
  //
  // Build common structures for encoding and decoding
  void BuildCommon(void);
  //
  //
public:
  //
  BlockBuffer(class Frame *frame);
  //
  virtual ~BlockBuffer(void); 
  //
  // Return a pointer to an array of DCT transformers.
  class DCT *const *DCTsOf(void) const
  {
    return m_ppDCT;
  }
  //
  // Return the current top MCU quantized line.
  virtual class QuantizedRow *CurrentQuantizedRow(UBYTE comp)
  {
    assert(comp < m_ucCount);
    return *m_pppQStream[comp];
  }
  //
  // Return the current top row of the residuals.
  class QuantizedRow *CurrentResidualRow(UBYTE comp)
  {
    assert(comp < m_ucCount);
    if (m_pppRStream[comp])
      return *m_pppRStream[comp];
    return NULL;
  }
  //
  // Start a MCU scan by initializing the quantized rows for this row
  // in this scan.
  virtual bool StartMCUQuantizerRow(class Scan *scan);
  //
  // Scan-dependent residual start
  virtual bool StartMCUResidualRow(class Scan *scan);
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
    return false;
  } 
  //
  // Post the height of the frame in lines. This happens
  // when the DNL marker is processed.
  virtual void PostImageHeight(ULONG lines)
  {
    m_ulPixelHeight = lines;
  }
};
///

///
#endif
