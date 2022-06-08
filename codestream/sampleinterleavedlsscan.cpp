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
** A JPEG LS scan interleaving samples of several components,
** sample by sample.
**
** $Id: sampleinterleavedlsscan.cpp,v 1.16 2022/06/08 10:54:55 thor Exp $
**
*/

/// Includes
#include "codestream/jpeglsscan.hpp"
#include "codestream/sampleinterleavedlsscan.hpp"
#include "marker/frame.hpp"
///

/// SampleInterleavedLSScan::SampleInterleavedLSScan
// Create a new scan. This is only the base type.
SampleInterleavedLSScan::SampleInterleavedLSScan(class Frame *frame,class Scan *scan,
                                                 UBYTE near,const UBYTE *mapping,UBYTE point)
  : JPEGLSScan(frame,scan,near,mapping,point)
{
}
///

/// SampleInterleavedLSScan::~SampleInterleavedLSScan
// Dispose a scan.
SampleInterleavedLSScan::~SampleInterleavedLSScan(void)
{
}
///


/// SampleInterleavedLSScan::FindComponentDimensions
// Collect component information and install the component dimensions.
void SampleInterleavedLSScan::FindComponentDimensions(void)
{ 
#if ACCUSOFT_CODE
  UBYTE cx;
  
  JPEGLSScan::FindComponentDimensions();

  //
  // Check that all MCU dimensions are 1.
  for(cx = 0;cx < m_ucCount;cx++) {
    class Component *comp = ComponentOf(cx);
    if (comp->MCUHeightOf() != 1 || comp->MCUWidthOf() != 1)
      JPG_THROW(INVALID_PARAMETER,"SampleInterleavedLSScan::FindComponentDimensions",
                "sample interleaved JPEG LS does not support subsampling");
  }
#endif
}
///

/// SampleInterleavedLSScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more
// MCUs in this row.
bool SampleInterleavedLSScan::ParseMCU(void)
{
#if ACCUSOFT_CODE
  int lines             = m_ulRemaining[0]; // total number of MCU lines processed.
  UBYTE preshift        = m_ucLowBit + FractionalColorBitsOf();
  struct Line *line[4];
  UBYTE cx;

  //
  // If a DNL marker is present, the number of remaining lines is zero. Fix it.
  if (m_pFrame->HeightOf() == 0) {
    assert(lines == 0);
    lines = 8;
  }
  //
  // A "MCU" in respect to the code organization is eight lines.
  if (lines > 8) {
    lines = 8;
  }

  if (lines == 0)
    return false;
  
  if (m_pFrame->HeightOf() > 0)
    m_ulRemaining[0] -= lines;
  assert(m_ucCount < 4);

  //
  // Fill the line pointers.
  for(cx = 0;cx < m_ucCount;cx++) {
    line[cx] = CurrentLine(cx);
  }

  // Loop over lines and columns
  do {
    LONG length = m_ulWidth[0];
    LONG *lp[4];

    // Get the line pointers and initialize the internal backup lines.
    for(cx = 0;cx < m_ucCount;cx++) {
      lp[cx] = line[cx]->m_pData;
      StartLine(cx);
    }

    if (BeginReadMCU(m_Stream.ByteStreamOf())) { 
      // No error handling strategy. No RST in scans. Bummer!
      do {
        LONG a[4],b[4],c[4],d[4]; // neighbouring values.
        LONG d1[4],d2[4],d3[4];   // local gradients.
        bool isrun = true;
      
        for(cx = 0;cx < m_ucCount;cx++) {
          GetContext(cx,a[cx],b[cx],c[cx],d[cx]);

          d1[cx]  = d[cx] - b[cx];    // compute local gradients
          d2[cx]  = b[cx] - c[cx];
          d3[cx]  = c[cx] - a[cx];

          //
          // Run mode only if the run condition is met for all components
          if (isrun && !isRunMode(d1[cx],d2[cx],d3[cx]))
            isrun = false;
        }
        
        if (isrun) {
          LONG run = DecodeRun(length,m_lRunIndex[0]);
          //
          // Now fill the data.
          while(run) {
            // Update so that the next process gets the correct value.
            // There is one sample per component.
            for(cx = 0;cx < m_ucCount;cx++) {
              UpdateContext(cx,a[cx]);
              // And insert the value into the target line as well.
              *lp[cx]++ = a[cx] << preshift;
            }
            run--,length--;
            // As long as there are pixels on the line.
          }
          //
          // More data on the line? I.e. the run did not cover the full m_lJ samples?
          // Now decode the run interruption sample. The rtype is here always zero.
          if (length) {
            bool negative; // the sign variable
            LONG errval;   // the prediction error
            LONG merr;     // the mapped error (symbol)
            LONG rx;       // the reconstructed value
            UBYTE k;       // golomb parameter
            //
            // Decode the interrupting pixels.
            for(cx = 0;cx < m_ucCount;cx++) {
              // Get the neighbourhood.
              GetContext(cx,a[cx],b[cx],c[cx],d[cx]);
              // The prediction mode is always false, but the sign information
              // is required.
              negative = a[cx] > b[cx];
              // Get the golomb parameter for run interruption coding.
              k       = GolombParameter(false);
              // Golomb-decode the error symbol. It is always using the common
              // run index.
              merr    = GolombDecode(k,m_lLimit - m_lJ[m_lRunIndex[0]] - 1);
              // Inverse the error mapping procedure.
              errval  = InverseErrorMapping(merr,ErrorMappingOffset(false,merr != 0,k));
              // Compute the reconstructed value.
              rx      = Reconstruct(negative,b[cx],errval);
              // Update so that the next process gets the correct value.
              UpdateContext(cx,rx);
              // Fill in the value into the line
              *lp[cx]++ = rx << preshift;
              // Update the variables of the run mode.
              UpdateState(false,errval);
            }
            // Update the run index now. This is not part of
            // EncodeRun because the non-reduced run-index is
            // required for the golomb coder length limit. 
            if (m_lRunIndex[0] > 0)
              m_lRunIndex[0]--;
          } else break; // end of line.
        } else {
          UWORD ctxt;
          bool  negative; // the sign variable.
          LONG  px;       // the predicted variable.
          LONG  rx;       // the reconstructed value.
          LONG  errval;   // the error value.
          LONG  merr;     // the mapped error value.
          UBYTE k;        // the Golomb parameter.
          //
          for(cx = 0;cx < m_ucCount;cx++) {
            // Quantize the gradients.
            d1[cx]  = QuantizedGradient(d1[cx]);
            d2[cx]  = QuantizedGradient(d2[cx]);
            d3[cx]  = QuantizedGradient(d3[cx]);
            // Compute the context.
            ctxt    = Context(negative,d1[cx],d2[cx],d3[cx]); 
            // Compute the predicted value.
            px      = Predict(a[cx],b[cx],c[cx]);
            // Correct the prediction.
            px      = CorrectPrediction(ctxt,negative,px);
            // Compute the golomb parameter k from the context.
            k       = GolombParameter(ctxt);
            // Decode the error symbol.
            merr    = GolombDecode(k,m_lLimit);
            // Inverse the error symbol into an error value.
            errval  = InverseErrorMapping(merr,ErrorMappingOffset(ctxt,k));
            // Update the variables.
            UpdateState(ctxt,errval);
            // Compute the reconstructed value.
            rx      = Reconstruct(negative,px,errval);
            // Update so that the next process gets the correct value.
            UpdateContext(cx,rx);
            // And insert the value into the target line as well.
            *lp[cx]++ = rx << preshift;
          }
        }
      } while(--length);
    } // No error handling here.
    //
    // Advance the line pointers.
    for(cx = 0;cx < m_ucCount;cx++) {
      EndLine(cx);
      line[cx] = line[cx]->m_pNext;
    }
    //
  } while(--lines);
  //
  // If this is the last line, gobble up all the
  // bits from bitstuffing the last byte may have left.
  // As SkipStuffing is idempotent, we can also do that
  // all the time.
  m_Stream.SkipStuffing();
#endif  
  return false;
}
///

/// SampleInterleavedLSScan::WriteMCU
// Write a single MCU in this scan.
bool SampleInterleavedLSScan::WriteMCU(void)
{
#if ACCUSOFT_CODE
  int lines             = m_ulRemaining[0]; // total number of MCU lines processed.
  UBYTE preshift        = m_ucLowBit + FractionalColorBitsOf();
  struct Line *line[4];
  UBYTE cx;
  
  //
  // A "MCU" in respect to the code organization is eight lines.
  if (lines > 8) {
    lines = 8;
  }
  m_ulRemaining[0] -= lines;
  assert(lines > 0);
  assert(m_ucCount < 4);

  //
  // Fill the line pointers.
  for(cx = 0;cx < m_ucCount;cx++) {
    line[cx] = CurrentLine(cx);
  }

  // Loop over lines and columns
  do {
    LONG length = m_ulWidth[0];
    LONG *lp[4];

    // Get the line pointers and initialize the internal backup lines.
    for(cx = 0;cx < m_ucCount;cx++) {
      lp[cx] = line[cx]->m_pData;
      StartLine(cx);
    }
    //
    BeginWriteMCU(m_Stream.ByteStreamOf()); 
    do {
        LONG a[4],b[4],c[4],d[4]; // neighbouring values.
        LONG d1[4],d2[4],d3[4];   // local gradients.
        bool isrun = true;
      
        for(cx = 0;cx < m_ucCount;cx++) {
          GetContext(cx,a[cx],b[cx],c[cx],d[cx]);

          d1[cx]  = d[cx] - b[cx];    // compute local gradients
          d2[cx]  = b[cx] - c[cx];
          d3[cx]  = c[cx] - a[cx];

          //
          // Run mode only if the run condition is met for all components
          if (isrun && !isRunMode(d1[cx],d2[cx],d3[cx]))
            isrun = false;
        }
        
        if (isrun) {
          LONG runcnt = 0;
          do {
            //
            // Check whether the pixel is close enough to continue the run.
            for(cx = 0;cx < m_ucCount;cx++) {
              LONG x  = *lp[cx] >> preshift;
              if (x - a[cx] < -m_lNear || x - a[cx] > m_lNear)
                break;
            }
            if (cx < m_ucCount)
              break; // run ends.
            //
            // Update so that the next process gets the correct value.
            // Also updates the line pointers.
            for(cx = 0;cx < m_ucCount;cx++) {
              UpdateContext(cx,a[cx]);
              lp[cx]++;
            }
          } while(runcnt++,--length);
          //
          // Encode the run. Note that only a single run index is used here.
          EncodeRun(runcnt,length == 0,m_lRunIndex[0]);
          // Continue the encoding of the end of the run if there are more
          // samples to encode.
          if (length) {       
            bool negative; // the sign variable
            LONG errval;   // the prediction error
            LONG merr;     // the mapped error (symbol)
            LONG rx;       // the reconstructed value
            UBYTE k;       // golomb parameter
            //
            // The complete pixel in all components is now to be encoded.
            for(cx = 0;cx < m_ucCount;cx++) {
              // Get the neighbourhood.
              GetContext(cx,a[cx],b[cx],c[cx],d[cx]);
              // The prediction mode is always fixed, but the sign
              // has to be found.
              negative = a[cx] > b[cx];
              // Compute the error value.
              errval   = (*lp[cx]++ >> preshift) - b[cx];
              if (negative)
                errval = -errval;
              // Quantize the error.
              errval = QuantizePredictionError(errval);
              // Compute the reconstructed value.
              rx     = Reconstruct(negative,b[cx],errval);
              // Update so that the next process gets the correct value.
              UpdateContext(cx,rx);
              // Get the golomb parameter for run interruption coding.
              k      = GolombParameter(false);
              // Map the error into a symbol.
              merr   = ErrorMapping(errval,ErrorMappingOffset(false,errval != 0,k));
              // Golomb-coding of the error.
              GolombCode(k,merr,m_lLimit - m_lJ[m_lRunIndex[0]] - 1);
              // Update the variables of the run mode.
              UpdateState(false,errval);
            }
            // Update the run index now. This is not part of
            // EncodeRun because the non-reduced run-index is
            // required for the golomb coder length limit.
            if (m_lRunIndex[0] > 0)
                m_lRunIndex[0]--;
          } else break; // Line ended, abort the loop over the line.  
        } else { 
          UWORD ctxt;
          bool  negative; // the sign variable.
          LONG  px;       // the predicted variable.
          LONG  rx;       // the reconstructed value.
          LONG  errval;   // the error value.
          LONG  merr;     // the mapped error value.
          UBYTE k;        // the Golomb parameter.
          //
          for(cx = 0;cx < m_ucCount;cx++) {
            // Quantize the gradients.
            d1[cx]     = QuantizedGradient(d1[cx]);
            d2[cx]     = QuantizedGradient(d2[cx]);
            d3[cx]     = QuantizedGradient(d3[cx]);
            // Compute the context.
            ctxt   = Context(negative,d1[cx],d2[cx],d3[cx]); 
            // Compute the predicted value.
            px     = Predict(a[cx],b[cx],c[cx]);
            // Correct the prediction.
            px     = CorrectPrediction(ctxt,negative,px);
            // Compute the error value.
            errval = (*lp[cx]++ >> preshift) - px;
            if (negative)
              errval = -errval;
            // Quantize the prediction error if NEAR > 0
            errval = QuantizePredictionError(errval);
            // Compute the reconstructed value.
            rx     = Reconstruct(negative,px,errval);
            // Update so that the next process gets the correct value.
            UpdateContext(cx,rx);
            // Compute the golomb parameter k from the context.
            k      = GolombParameter(ctxt);
            // Map the error into a symbol
            merr   = ErrorMapping(errval,ErrorMappingOffset(ctxt,k));
            // Golomb-coding of the error.
            GolombCode(k,merr,m_lLimit);
            // Update the variables.
            UpdateState(ctxt,errval);
          }
        }
    } while(--length);
    //
    // Advance the line pointers.
    for(cx = 0;cx < m_ucCount;cx++) {
      EndLine(cx);
      line[cx] = line[cx]->m_pNext;
    }
    //
  } while(--lines);
#endif  
  return false;
}
///
