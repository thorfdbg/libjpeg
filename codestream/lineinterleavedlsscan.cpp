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
** A JPEG LS scan covering only a single component.
**
** $Id: lineinterleavedlsscan.cpp,v 1.13 2014/11/14 15:41:32 thor Exp $
**
*/

/// Includes
#include "codestream/jpeglsscan.hpp"
#include "codestream/lineinterleavedlsscan.hpp"
///

/// LineInterleavedLSScan::LineInterleavedLSScan
// Create a new scan. This is only the base type.
LineInterleavedLSScan::LineInterleavedLSScan(class Frame *frame,class Scan *scan,
                                             UBYTE near,const UBYTE *mapping,UBYTE point)
  : JPEGLSScan(frame,scan,near,mapping,point)
{
}
///

/// LineInterleavedLSScan::~LineInterleavedLSScan
// Dispose a scan.
LineInterleavedLSScan::~LineInterleavedLSScan(void)
{
}
///

/// LineInterleavedLSScan::ParseMCU
// Parse a single MCU in this scan. Return true if there are more
// MCUs in this row.
bool LineInterleavedLSScan::ParseMCU(void)
{
#if ACCUSOFT_CODE
  UBYTE cx;
  UBYTE preshift        = m_ucLowBit + FractionalColorBitsOf();
  struct Line *line[4]; // Pointer to the lines.
  UWORD mcuheight[4];   // number of lines in a MCU
  ULONG ypos[4];
  int lines = 8;        // lines to process in this block of lines (not really a MCU)
  bool more;

  assert(m_ucCount < 4);

  for(cx = 0;cx < m_ucCount;cx++) {
    class Component *comp = ComponentOf(cx);
    line[cx]      = CurrentLine(cx);
    mcuheight[cx] = comp->MCUHeightOf();
    ypos[cx]      = CurrentYOf(cx);
  }

  // Loop over lines, lines are coded independently, in groups where
  // each group contains MCUheight lines of each.
  do {
    // MCU is a group of lines.
    if (BeginReadMCU(m_Stream.ByteStreamOf())) {
      //
      // Loop over components.
      for(cx = 0;cx < m_ucCount;cx++) {
        UWORD ymcu;
        // Loop over the MCU height of this component.
        for(ymcu = 0;line[cx] && ymcu < mcuheight[cx];ymcu++) {
          LONG length = m_ulWidth[cx];
          LONG *lp    = line[cx]->m_pData;
#ifdef DEBUG_LS
          int xpos    = 0;
          static int linenumber = 0;
          printf("\n%4d : ",++linenumber);
#endif
          //
          StartLine(cx);
          do {
            LONG a,b,c,d;   // neighbouring values.
            LONG d1,d2,d3;  // local gradients.
            
            GetContext(cx,a,b,c,d);
            d1  = d - b;    // compute local gradients
            d2  = b - c;
            d3  = c - a;
            
            if (isRunMode(d1,d2,d3)) {
              LONG run = DecodeRun(length,m_lRunIndex[cx]);
              //
              // Now fill the data.
              while(run) {
                // Update so that the next process gets the correct value.
                UpdateContext(cx,a);
                // And insert the value into the target line as well.
                *lp++ = a << preshift;
#ifdef DEBUG_LS
                printf("%4d:<%2x> ",xpos++,lp[-1]);
#endif
                run--,length--;
                // As long as there are pixels on the line.
              }
              //
              // More data on the line? I.e. the run did not cover the full m_lJ samples?
              // Now decode the run interruption sample.
              if (length) {
                bool negative; // the sign variable
                bool rtype;    // run interruption type
                LONG errval;   // the prediction error
                LONG merr;     // the mapped error (symbol)
                LONG rx;       // the reconstructed value
                UBYTE k;       // golomb parameter
                // Get the neighbourhood.
                GetContext(cx,a,b,c,d);
                // Get the prediction mode.
                rtype  = InterruptedPredictionMode(negative,a,b);
                // Get the golomb parameter for run interruption coding.
                k      = GolombParameter(rtype);
                // Golomb-decode the error symbol.
                merr   = GolombDecode(k,m_lLimit - m_lJ[m_lRunIndex[cx]] - 1);
                // Inverse the error mapping procedure.
                errval = InverseErrorMapping(merr + rtype,ErrorMappingOffset(rtype,rtype || merr,k));
                // Compute the reconstructed value.
                rx     = Reconstruct(negative,rtype?a:b,errval);
                // Update so that the next process gets the correct value.
                UpdateContext(cx,rx );
                // Fill in the value into the line
                *lp    = rx << preshift;
#ifdef DEBUG_LS
                printf("%4d:<%2x> ",xpos++,lp[0]);
#endif
                // Update the variables of the run mode.
                UpdateState(rtype,errval);
                // Update the run index now. This is not part of
                // EncodeRun because the non-reduced run-index is
                // required for the golomb coder length limit. 
                if (m_lRunIndex[cx] > 0)
                  m_lRunIndex[cx]--;
              } else break; // end of line.
            } else {
              UWORD ctxt;
              bool  negative; // the sign variable.
              LONG  px;       // the predicted variable.
              LONG  rx;       // the reconstructed value.
              LONG  errval;   // the error value.
              LONG  merr;     // the mapped error value.
              UBYTE k;        // the Golomb parameter.
              // Quantize the gradients.
              d1     = QuantizedGradient(d1);
              d2     = QuantizedGradient(d2);
              d3     = QuantizedGradient(d3);
              // Compute the context.
              ctxt   = Context(negative,d1,d2,d3); 
              // Compute the predicted value.
              px     = Predict(a,b,c);
              // Correct the prediction.
              px     = CorrectPrediction(ctxt,negative,px);
              // Compute the golomb parameter k from the context.
              k      = GolombParameter(ctxt);
              // Decode the error symbol.
              merr   = GolombDecode(k,m_lLimit);
              // Inverse the error symbol into an error value.
              errval = InverseErrorMapping(merr,ErrorMappingOffset(ctxt,k));
              // Update the variables.
              UpdateState(ctxt,errval);
              // Compute the reconstructed value.
              rx     = Reconstruct(negative,px,errval);
              // Update so that the next process gets the correct value.
              UpdateContext(cx,rx);
              // And insert the value into the target line as well.
              *lp    = rx << preshift;
#ifdef DEBUG_LS
              printf("%4d:<%2x> ",xpos++,lp[0]);
#endif
            }
          } while(++lp,--length);
          EndLine(cx);
          line[cx] = line[cx]->m_pNext;
        } // Of loop over MCU height
      } // of loop over components.
    } // Of MCU is correct
    // Advance the Y pos over the MCU
    for(more = true,cx = 0;cx < m_ucCount;cx++) {
      ypos[cx] += mcuheight[cx];
      if (m_ulHeight[cx] && ypos[cx] >= m_ulHeight[cx])
        more = false;
    }
  } while(--lines && more); 
  // If this is the last line, gobble up all the
  // bits from bitstuffing the last byte may have left. 
  // As SkipStuffing is idempotent, we can also do that
  // all the time.
  m_Stream.SkipStuffing();
#endif
  return false;
}
///

/// LineInterleavedLSScan::WriteMCU
// Write a single MCU in this scan.
bool LineInterleavedLSScan::WriteMCU(void)
{
#if ACCUSOFT_CODE
  UBYTE cx;
  UBYTE preshift        = m_ucLowBit + FractionalColorBitsOf();
  struct Line *line[4]; // Pointer to the lines.
  UWORD mcuheight[4];   // number of lines in a MCU
  ULONG ypos[4];
  int lines = 8;        // lines to process in this block of lines (not really a MCU)
  bool more;

  assert(m_ucCount < 4);

  for(cx = 0;cx < m_ucCount;cx++) {
    class Component *comp = ComponentOf(cx);
    line[cx]      = CurrentLine(cx);
    mcuheight[cx] = comp->MCUHeightOf();
    ypos[cx]      = CurrentYOf(cx);
  }

  // Loop over lines, lines are coded independently, in groups where
  // each group contains MCUheight lines of each.
  do {
    // MCU is a group of lines.
    BeginWriteMCU(m_Stream.ByteStreamOf()); 
    //
    // Loop over components.
    for(cx = 0;cx < m_ucCount;cx++) {
      UWORD ymcu;
      // Loop over the MCU height of this component.
      for(ymcu = 0;line[cx] && ymcu < mcuheight[cx];ymcu++) {
        LONG length = m_ulWidth[cx];
        LONG *lp    = line[cx]->m_pData;

        StartLine(cx);
        do {
          LONG a,b,c,d,x; // neighbouring values.
          LONG d1,d2,d3;  // local gradients.
          
          GetContext(cx,a,b,c,d);
          x   = *lp >> preshift;
          
          d1  = d - b;    // compute local gradients
          d2  = b - c;
          d3  = c - a;
          
          if (isRunMode(d1,d2,d3)) {
            LONG runval = a;
            LONG runcnt = 0;
            do {
              x  = *lp >> preshift;
              if (x - runval < -m_lNear || x - runval > m_lNear)
                break;
              // Update so that the next process gets the correct value.
              // Also updates the line pointers.
              UpdateContext(cx,runval);
            } while(lp++,runcnt++,--length);
            // Encode the run. Depends on whether the run was interrupted
            // by the end of the line.
            EncodeRun(runcnt,length == 0,m_lRunIndex[cx]);
            // Continue the encoding of the end of the run if there are more
            // samples to encode.
            if (length) {
              bool negative; // the sign variable
              bool rtype;    // run interruption type
              LONG errval;   // the prediction error
              LONG merr;     // the mapped error (symbol)
              LONG rx;       // the reconstructed value
              UBYTE k;       // golomb parameter
              // Get the neighbourhood.
              GetContext(cx,a,b,c,d);
              // Get the prediction mode.
              rtype  = InterruptedPredictionMode(negative,a,b);
              // Compute the error value.
              errval = x - ((rtype)?(a):(b));
              if (negative)
                errval = -errval;
              // Quantize the error.
              errval = QuantizePredictionError(errval);
              // Compute the reconstructed value.
              rx     = Reconstruct(negative,rtype?a:b,errval);
              // Update so that the next process gets the correct value.
              UpdateContext(cx,rx);
              // Get the golomb parameter for run interruption coding.
              k      = GolombParameter(rtype);
              // Map the error into a symbol.
              merr   = ErrorMapping(errval,ErrorMappingOffset(rtype,errval != 0,k)) - rtype;
              // Golomb-coding of the error.
              GolombCode(k,merr,m_lLimit - m_lJ[m_lRunIndex[cx]] - 1);
              // Update the variables of the run mode.
              UpdateState(rtype,errval);
              // Update the run index now. This is not part of
              // EncodeRun because the non-reduced run-index is
              // required for the golomb coder length limit.
              if (m_lRunIndex[cx] > 0)
                m_lRunIndex[cx]--;
            } else break; // Line ended, abort the loop over the line.
          } else { 
            UWORD ctxt;
            bool  negative; // the sign variable.
            LONG  px;       // the predicted variable.
            LONG  rx;       // the reconstructed value.
            LONG  errval;   // the error value.
            LONG  merr;     // the mapped error value.
            UBYTE k;        // the Golomb parameter.
            // Quantize the gradients.
            d1     = QuantizedGradient(d1);
            d2     = QuantizedGradient(d2);
            d3     = QuantizedGradient(d3);
            // Compute the context.
            ctxt   = Context(negative,d1,d2,d3); 
            // Compute the predicted value.
            px     = Predict(a,b,c);
            // Correct the prediction.
            px     = CorrectPrediction(ctxt,negative,px);
            // Compute the error value.
            errval = x - px;
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
        } while(++lp,--length);
        EndLine(cx);
        // Go to the next line
        line[cx] = line[cx]->m_pNext;
      } // Of loop over MCU height
    } // of loop over components.
    // Advance the Y pos over the MCU
    for(more = true,cx = 0;cx < m_ucCount;cx++) {
      ypos[cx] += mcuheight[cx];
      if (m_ulHeight[cx] && ypos[cx] >= m_ulHeight[cx])
        more = false;
    }
  } while(--lines && more);
#endif
  return false;
}
///
