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
** A JPEG LS scan. This is the base for all JPEG LS scan types, namely
** separate, line interleaved and sample interleaved.
**
** $Id: jpeglsscan.hpp,v 1.32 2021/04/12 10:01:22 thor Exp $
**
*/

#ifndef CODESTREAM_JPEGLSSCAN_HPP
#define CODESTREAM_JPEGLSSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "io/bitstream.hpp"
#include "codestream/entropyparser.hpp"
#include "tools/line.hpp"
#include "marker/component.hpp"
#include "control/linebuffer.hpp"
///

/// Forwards
class Frame;
class LineCtrl;
class ByteStream;
class HuffmanCoder;
class HuffmanDecoder;
class HuffmanStatistics;
class LineBitmapRequester;
class LineBuffer;
class LineAdapter;
class Scan;
class Thresholds;
///

/// class JPEGLSScan
// A JPEG LS scan, the base class for all LS scan types
class JPEGLSScan : public EntropyParser {
  //
#if ACCUSOFT_CODE
  // The class used for pulling and pushing data.
  class LineBuffer          *m_pLineCtrl;
  //
  // In case no LSE threshold marker is here, this is a dummy constructed
  // here to avoid the computation of the threshold bounds once again.
  class Thresholds          *m_pDefaultThresholds;
  //
  // Dimension of the frame in full pixels.
  ULONG                      m_ulPixelWidth;
  ULONG                      m_ulPixelHeight;
  //
  // Mapping table index.
  UBYTE                      m_ucMapIdx[4];
  // 
  // The previous line, required to compute the contexts and the prediction.
  struct Line                m_Top[4];
  //
  // The line above the previous line. This and m_pusTop are swapped every
  // line to have a continuous line buffer.
  struct Line                m_AboveTop[4];
  //
protected:
  //
  // Dimensions of the components.
  ULONG                      m_ulWidth[4];
  ULONG                      m_ulHeight[4];
  ULONG                      m_ulRemaining[4]; // Number of remaining lines
  //
  // The bitstream for bit-IO. This is bitstuffed, not bytestuffed.
  BitStream<true>            m_Stream;
  //
  // Pointer into this and the previous line.
  LONG                      *m_pplCurrent[4];
  LONG                      *m_pplPrevious[4];
  //
  // The thresholds, near parameters and reset parameter.
  //
  // First the approximation value, zero for lossless.
  LONG                       m_lNear;
  // The quantization bucket size, 2 * m_lNear + 1
  LONG                       m_lDelta;
  //
  // The maximum sample value. Need not to be the bitdepths
  LONG                       m_lMaxVal;
  //
  // The range value
  LONG                       m_lRange;
  //
  // Minimum and maximum error value before range-reduction.
  LONG                       m_lMinErr;
  LONG                       m_lMaxErr;
  //
  // Minimum and maximum reconstructed value before clipping.
  LONG                       m_lMinReconstruct;
  LONG                       m_lMaxReconstruct;
  //
  // The Qbbp value, also from the standard.
  LONG                       m_lQbpp;
  //
  // The Bpp value
  LONG                       m_lBpp;
  //
  // The limit value from the specs.
  LONG                       m_lLimit;
  //
  // The first threshold for context definition.
  LONG                       m_lT1;
  //
  // The second threshold for context definition.
  LONG                       m_lT2;
  //
  // The third threshold for context definition.
  LONG                       m_lT3;
  //
  // The reset interval.
  LONG                       m_lReset;
  //
  // The run index, one per component.
  LONG                       m_lRunIndex[4];
  //
  // The J array for the run mode.
  static const LONG          m_lJ[32];
  //
  // The low bit for the point transform.
  UBYTE                      m_ucLowBit;
  //
  // Quick golomb decoder. This array returns the number of leading
  // zero bits of its input.
  UBYTE                      m_ucLeadingZeros[256];
  //
  // Context state variables. The first two are
  // reserved for the run mode.
  LONG                       m_lN[405 + 2];
  LONG                       m_lA[405 + 2];
  LONG                       m_lB[405 + 2];
  LONG                       m_lC[405 + 2];
  //
  // 
  // Return the current line of the given component index.
  struct Line *CurrentLine(UBYTE c)
  {
    class Component *comp = ComponentOf(c);
    struct Line *line     = m_pLineCtrl->CurrentLineOf(comp->IndexOf());

    return line;
  }
  //
  // Return the Y position of the current topmost line to process,
  // given the index of the component in the scan.
  ULONG CurrentYOf(UBYTE c)
  {
    return m_pLineCtrl->CurrentYOf(ComponentOf(c)->IndexOf());
  }
  //
  // Reset to the start of a line for component i.
  void StartLine(UBYTE comp)
  {
    m_pplCurrent[comp]     = m_AboveTop[comp].m_pData + 1;
    m_pplPrevious[comp]    = m_Top[comp].m_pData + 1;
    // Copy at the start of the line the sample at position b to
    // the sample at position a
    m_pplCurrent[comp][-1] = m_pplPrevious[comp][0];
  }
  //
  // End a line.
  void EndLine(UBYTE comp)
  {
    LONG *data  = m_Top[comp].m_pData;
    //
    // Interchange the lines. This automatically copies the
    // samples from the current line (which m_pCurrent = AboveTop)
    // points to to the previous line. It also copies the sample
    // at position a to the sample at position c.
    m_Top[comp].m_pData      = m_AboveTop[comp].m_pData;
    m_AboveTop[comp].m_pData = data;
  }
  //
  // Update the context from the sample at position x so the next line
  // reads the correct context for a and b. Also advances the pointer
  // positions to move to the next sample in the component.
  void UpdateContext(UBYTE comp,LONG x)
  {
    m_pplCurrent[comp][0] = x;
    m_pplCurrent[comp][1] = x; // This defines the proper value for d at the edge.

    m_pplCurrent[comp]++;
    m_pplPrevious[comp]++;
  }
  //
  // Extract the samples at positions A,B,C,D, i.e. the context.
  void GetContext(UBYTE comp,LONG &a,LONG &b,LONG &c,LONG &d) const
  {
    b = m_pplPrevious[comp][0]; // Always above. This is zero-initialized.
    d = m_pplPrevious[comp][1]; // Last sample is copied over
    c = m_pplPrevious[comp][-1];
    a = m_pplCurrent[comp][-1];
  }
  //
  // Check whether the runlength mode should be enabled. Input are the
  // deltas.
  bool isRunMode(LONG d1,LONG d2,LONG d3) const
  {
    if ((d1 > m_lNear || d1 < -m_lNear) ||
        (d2 > m_lNear || d2 < -m_lNear) ||
        (d3 > m_lNear || d3 < -m_lNear))
         return false;
    return true;
  }
  //
  // Predict the pixel value from the context values a,b,c
  static LONG Predict(LONG a,LONG b,LONG c)
  {
    LONG maxab = (a > b)?(a):(b);
    LONG minab = (a < b)?(a):(b);

    if (c >= maxab) {
      return minab;
    } else if (c <= minab) {
      return maxab;
    } else {
      return a + b - c;
    }
  }
  //
  // Quantize the gradient using T1,T2,T3
  LONG QuantizedGradient(LONG d) const
  {
    if (d <= -m_lT3) {
      return -4;
    } else if (d <= -m_lT2) {
      return -3;
    } else if (d <= -m_lT1) {
      return -2;
    } else if (d <  -m_lNear) {
      return -1;
    } else if (d <=  m_lNear) {
      return 0;
    } else if (d <   m_lT1) {
      return 1;
    } else if (d <   m_lT2) {
      return 2;
    } else if (d <   m_lT3) {
      return 3;
    } else {
      return 4;
    }
  }
  //
  // Correct the prediction using the context and the sign value.
  LONG CorrectPrediction(UWORD ctxt,bool negative,LONG px) const
  {
    if (negative) {
      px -= m_lC[ctxt];
    } else {
      px += m_lC[ctxt];
    }
    if (unlikely(px > m_lMaxVal))
      return m_lMaxVal;
    if (unlikely(px < 0))
      return 0;
    return px;
  }
  //
  // Compute the reconstructed value from the predicted value, the sign and the error.
  LONG Reconstruct(bool &negative,LONG px,LONG errval) const
  {
    LONG rx;
    
    if (negative)
      rx = px - errval * m_lDelta;
    else
      rx = px + errval * m_lDelta;
    
    // First wraparound into the extended reconstruct range.
    if (unlikely(rx < m_lMinReconstruct))
      rx += m_lRange * m_lDelta;
    if (unlikely(rx > m_lMaxReconstruct))
      rx -= m_lRange * m_lDelta;

    // Clip into the range.
    if (unlikely(rx > m_lMaxVal))
      rx = m_lMaxVal;
    if (unlikely(rx < 0))
      rx = 0;
    
    return rx;      
  }
  //
  // Compute the context index from the quantization parameters, also compute a sign
  // value.
  static UWORD Context(bool &negative,LONG q1,LONG q2,LONG q3)
  {
    if (q1 < 0 || (q1 == 0 && q2 < 0) || (q1 == 0 && q2 == 0 && q3 < 0)) {
      q1       = -q1;
      q2       = -q2;
      q3       = -q3;
      negative = true;
    } else {
      negative = false;
    }

    // The two extra states are for runlength coding.
    return q1 * 9 * 9 + (q2 + 4) * 9 + (q3 + 4) + 2;
  }
  //
  // Quantize the prediction error, reduce to the coding range.
  LONG QuantizePredictionError(LONG errval) const
  {
    // Quantization of the error signal.
    if (unlikely(m_lNear > 0)) {
      if (errval > 0) {
        errval =  (m_lNear + errval) / m_lDelta;
      } else {
        errval = -(m_lNear - errval) / m_lDelta;
      }
    }

    //
    // A.9 is buggy since it does not allow negative errors.
    // Instead, map into the range of 
    // (range + 1) / 2 - range .. (range + 1) / 2 - 1
    if (unlikely(errval < m_lMinErr))
      errval += m_lRange;
    if (unlikely(errval >= m_lMaxErr))
      errval -= m_lRange;
      
    return errval;
  }
  //
  // Compute the Golomb parameter from the context.
  UBYTE GolombParameter(UWORD context) const
  {
    UBYTE k;

    for(k = 0;(m_lN[context] << k) < m_lA[context] && k < 24;k++) {
    }

    if (unlikely(k == 24)) {
      JPG_WARN(MALFORMED_STREAM,"JPEGLSScan::GolombParameter",
               "Golomb coding parameter of JPEG LS stream run out of bounds, "
               "synchronization lost");
      return 0;
    }
    
    return k;
  }
  //
  // Check whether the regular mode uses the inverse error mapping.
  // Requires the context index and the Golomb parameter.
  LONG ErrorMappingOffset(UWORD context,UBYTE k) const
  {
    return m_lNear == 0 && k == 0 && (m_lB[context] << 1) <= -m_lN[context];
  }
  //
  // Check whether the error mapping is inverted for runlength interruption
  // coding. Error mapping is done differently here by inverting the signal
  // instead of changing the order. The second argument identifies
  // whether the mapped error is zero or not.
  LONG ErrorMappingOffset(UWORD context,bool nonzero,UBYTE k) const
  {
    return -(nonzero && k == 0 && (m_lB[context] << 1) < m_lN[context]);
  }
  //
  // Map the error to a positive symbol using the golomb parameter and the
  // context information.
  // By default, the output will be ordered as 0,-1,1,-2,2,-3,3. If
  // offset == +1, the order will be -1,0,-2,1,-3,...
  // If offset == -1, the order will be 0,1,-1,2,-2,...
  static LONG ErrorMapping(LONG errval,LONG offset)
  {
    if (errval < 0) {
      return ((-errval) << 1) - 1 - offset;
    } else {
      return (errval << 1) + offset;
    }
  }
  //
  // Inverse error mapping, from the absolute error symbol to the signed error.
  static LONG InverseErrorMapping(LONG merr,LONG offset)
  {
    LONG errval;

    if (merr & 1) {
      errval = - (merr + 1) >> 1;
    } else {
      errval = merr >> 1;
    }

    if (offset > 0) {
      return -(errval + 1);
    } else if (offset < 0) {
      return -errval;
    } else {
      return errval;
    }
  }
  //
  // Encode the mapped error using the golomb code k.
  // limit is the maximum number of unary bits to encode.
  void GolombCode(UBYTE k,LONG errval,LONG limit)
  {
    LONG unary = errval >> k;
    
    if (likely(unary < limit)) {
      // Unary part
      if (likely(unary)) {
        if (unlikely(unary > 32)) {
          m_Stream.Put<32>(0);
          unary -= 32;
        }
        m_Stream.Put(unary,0);
      }
      m_Stream.Put<1>(1);
      // binary part.
      if (k)
        m_Stream.Put(k,errval);
    } else {
      if (unlikely(limit > 32)) {
        m_Stream.Put<32>(0);
        limit -= 32;
      }
      m_Stream.Put(limit,0);
      m_Stream.Put<1>(1);
      m_Stream.Put(m_lQbpp,errval - 1);
    }
  }
  //
  // Decode a mapped error given the golomb parameter and the limit.
  LONG GolombDecode(UBYTE k,LONG limit)
  {
    UBYTE u  = 0;
    UWORD in;
 
    //
    // Find number of leading zeros by reading them in groups of 8 bits
    // if possible.
    do {
      in = m_Stream.PeekWord();
      // Count leading zeros.
      in = m_ucLeadingZeros[in >> 8];
      u += in;
      // Can be at most "limit" zeros, the encoder writes a one after at most "limit" zeros.
      // If not, we're pretty much out of sync.
      if (unlikely(u > limit)) {
        JPG_WARN(MALFORMED_STREAM,"JPEGLSScan::GolombDecode","found invalid Golomb code");
        return 0;
      }
      if (likely(in < 8)) {
        m_Stream.SkipBits(in+1);
        if (unlikely(u == limit)) {
          return m_Stream.Get(m_lQbpp) + 1;
        } else if (k) {
          return m_Stream.Get(k) | (u << k);
        } else {
          return u;
        }
      }
      m_Stream.SkipBits(8);
    } while(true);
  }
  //
  // Update the state information given the context and the unmapped error value.
  void UpdateState(UWORD context,LONG errval)
  {
    m_lB[context] += errval * m_lDelta;
    m_lA[context] += (errval >= 0)?(errval):(-errval);
    
    if (unlikely(m_lN[context] >= m_lReset)) {
      m_lA[context] >>= 1;
      if (m_lB[context] >= 0)
        m_lB[context] >>= 1;
      else
        m_lB[context]   = -((1 - m_lB[context]) >> 1);
      m_lN[context] >>= 1;
    }
    m_lN[context]++;
    
    if (unlikely(m_lB[context] <= -m_lN[context])) {
      m_lB[context] += m_lN[context];
      if (m_lC[context] > -128)
        m_lC[context]--;
      if (m_lB[context] <= -m_lN[context]) 
        m_lB[context] = -m_lN[context] + 1;
    } else if (unlikely(m_lB[context] > 0)) {
      m_lB[context] -= m_lN[context];
      if (m_lC[context] < 127)
        m_lC[context]++;
      if (m_lB[context] > 0)
        m_lB[context] = 0;
    }
  }
  //
  // Encode a runlength of the run mode coder. If "interrupted" is set,
  // the run is interrupted by a pixel where the prediction error is > than near.
  void EncodeRun(LONG runcnt,bool end,LONG &runindex)
  {
    while(runcnt >= (1 << m_lJ[runindex])) {
      m_Stream.Put<1>(1);
      runcnt -= 1 << m_lJ[runindex];
      if (runindex < 31)
        runindex++;
    }
    if (end) {
      if (runcnt > 0) 
        m_Stream.Put<1>(1); // decoder will detect an end of line.
    } else {
      m_Stream.Put<1>(0);
      if (m_lJ[runindex])
        m_Stream.Put(m_lJ[runindex],runcnt);
      // Reduction of the run index happens later.
    }
  }
  // 
  // Decode the runlength. Requires the remaining elements on the
  // line and the run index to update.
  LONG DecodeRun(LONG length,LONG &runindex)
  {
    LONG run = 0;
    
    while (m_Stream.Get<1>()) {
      run += (1 << m_lJ[runindex]);
      // Can the run be completed?
      if (run <= length && runindex < 31) 
        runindex++;
      //
      // If the run reaches the end of the line, do not get more bits.
      if (run >= length) {
        return length;
      }
    } 

    //
    // Read the remainder of the run
    // We should here be in the "interrupted by pixel" case.
    if (m_lJ[runindex])
      run += m_Stream.Get(m_lJ[runindex]);
      
    if (run > length) {
      JPG_WARN(MALFORMED_STREAM,"JPEGLSScan::DecodeRun",
               "found run across the end of the line, trimming it");
      run = length;
    }

    return run;
  }
  //
  // Compute the Interrupted Pixel prediction mode. If true, predict from A,
  // otherwise predict from B, also compute the sign flag.
  bool InterruptedPredictionMode(bool &negative,LONG a,LONG b) const
  { 
    negative = false;

    if ((a >= b && a - b <= m_lNear) || (a <= b && b - a <= m_lNear))
      return true;
    
    if (a > b)
      negative = true;

    return false;
  }
  //
  // Compute the Golomb parameter for the interrupted run coding.
  UBYTE GolombParameter(bool rtype) const
  {
    LONG temp;
    UBYTE k;

    if (rtype) {
      temp = m_lA[1] + (m_lN[1] >> 1);
    } else {
      temp = m_lA[0];
    }
    
    for(k = 0;(m_lN[rtype] << k) < temp && k < 24;k++) {
    } 

    if (k == 24) {
      JPG_WARN(MALFORMED_STREAM,"JPEGLSScan::GolombParameter",
               "Golomb coding parameter of JPEG LS stream run out of bounds, "
               "synchronization lost");
      return 0;
    }

    return k;
  }
  //
  // Update the state information given the context and the unmapped error value
  // for runlength interrupted coding.
  void UpdateState(bool rtype,LONG errval)
  {
    if (errval < 0) {
      m_lB[rtype]++;
      m_lA[rtype] += -errval - rtype;
    } else {
      m_lA[rtype] +=  errval - rtype;
    }

    if (unlikely(m_lN[rtype] >= m_lReset)) {
      m_lA[rtype]  >>= 1;
      m_lB[rtype]  >>= 1;
      m_lN[rtype]  >>= 1;
    }
    m_lN[rtype]++;
  }
  //
#endif
  //
  // Collect component information and install the component dimensions.
  virtual void FindComponentDimensions(void);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final); 
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
  // Scanning for a restart marker is here a bit more tricky due to the
  // presence of bitstuffing - the stuffed zero-bit need to be removed
  // (and thus the byte containing it) before scanning for the restart
  // marker.
  bool BeginReadMCU(class ByteStream *io);
  //
public:
  // Create a new scan. This is only the base type.
  JPEGLSScan(class Frame *frame,class Scan *scan,UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~JPEGLSScan(void);
  // 
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding
  virtual void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl);
  //
  // Start the measurement run for the optimized
  // huffman encoder.
  virtual void StartMeasureScan(class BufferCtrl *ctrl);
  // 
  // Start making an optimization run to adjust the coefficients.
  virtual void StartOptimizeScan(class BufferCtrl *ctrl);
  //
  // Start a MCU scan. Returns true if there are more rows. False otherwise.
  // Note that we emulate here that MCUs are multiples of eight lines high
  // even though from a JPEG perspective a MCU is a single pixel in the
  // lossless coding case.
  virtual bool StartMCURow(void);
  //
  // Initialize MCU for the next restart interval
  virtual void InitMCU(void);
  //
  // Parse a single MCU in this scan. Return true if there are more
  // MCUs in this row.
  virtual bool ParseMCU(void) = 0;
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void) = 0; 
  //
  // Make an R/D optimization for the given scan by potentially pushing
  // coefficients into other bins. 
  virtual void OptimizeBlock(LONG bx,LONG by,UBYTE component,double critical,
                             class DCT *dct,LONG quantized[64]); 
  //
  // Make an R/D optimization of the DC scan. This includes all DC blocks in
  // total, not just a single block. This is because the coefficients are not
  // coded independently.
  virtual void OptimizeDC(void);
};
///


///
#endif
