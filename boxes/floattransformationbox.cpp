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
** This box keeps a linear transformation that can be used either as L
** or C transformation. This is the floating point version.
**
** $Id: floattransformationbox.cpp,v 1.4 2015/03/26 10:03:27 thor Exp $
**
*/

/// Includes
#include "std/string.hpp"
#include "boxes/box.hpp"
#include "boxes/floattransformationbox.hpp"
#include "colortrafo/colortrafo.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "tools/numerics.hpp"
///

/// FloatTransformationBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. 
bool FloatTransformationBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  int i;
  LONG b;

  if (boxsize != 1 + 9 * 4)
    JPG_THROW(MALFORMED_STREAM,"FloatTransformationBox::ParseBoxContent",
              "malformed JPEG stream, size of the linear transformation box is inccorrect");
  
  b = stream->Get();
  if (b == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"FloatTransformationBox::ParseBoxContent",
              "malformed JPEG stream, unexpected EOF while parsing the linear transformation box");
  
  //
  // The ID must be between 5 and 15.
  m_ucID = b >> 4;
  if (m_ucID < 5 || m_ucID > 15)
    JPG_THROW(MALFORMED_STREAM,"FloatTransformationBox::ParseBoxContent",
              "malformed JPEG stream, the M value of a linear transformation box is out of range");

  //
  // The t value (fractional bits) must be 0.
  if ((b & 0x0f) != 0)
    JPG_THROW(MALFORMED_STREAM,"FloatTransformationBox::ParseBoxContent",
              "malformed JPEG stream, the t value of a linear transformation box is invalid");

  for(i = 0;i < 9;i++) {
    LONG lo,hi;
    hi = stream->GetWord();
    lo = stream->GetWord();
    if (lo == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"FloatTransformationBox::ParseBoxContent",
                "malformed JPEG stream, unexpected EOF while parsing the linear transformation box");

    m_fMatrix[i] = IEEEDecode(ULONG((hi << 16) | lo));
  }

  return true;
}
///

/// FloatTransformationBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
bool FloatTransformationBox::CreateBoxContent(class MemoryStream *target)
{
  int i;

  assert(m_ucID >= 5 && m_ucID <= 15);

  target->Put((m_ucID << 4) | 0);

  for(i = 0;i < 9;i++) {
    ULONG v = IEEEEncode(m_fMatrix[i]);
    
    target->PutWord(v >> 16);
    target->PutWord(v & 0xffff);
  }

  return true;
}
///

/// FloatTransformationBox::InvertMatrix
// Compute the inverse matrix and put it into m_lInverse. If it does not
// exist, throw.
void FloatTransformationBox::InvertMatrix(void)
{
  int x1,x;
  int x2,y2;
  int xpiv,ypiv;
  bool pivot[3] = {false,false,false};
  int srcrow[3];
  int dstrow[3]; // collects column and row swaps
  
  memcpy(m_fInverse,m_fMatrix,sizeof(m_fMatrix));

  // Loop over the columns.
  for(x = 0;x < 3;x++) {
    DOUBLE max = 0; 
    //
    // Select some element as pivot. If this turns out to
    // be zero, a problem will be reported anyhow.
    xpiv = ypiv = 0;
    //
    // Check all rows for a suitable pivot element.
    for(y2 = 0;y2 < 3;y2++) {
      // This row already handled?
      if (pivot[y2] == false) {
        // Nope.
        for(x2 = 0;x2 < 3;x2++) {
          // Is this row and column already handled?
          if (pivot[x2] == false) {
            DOUBLE here = m_fInverse[x2 + y2 * 3];
            // Compute the max as absolute value of the entry.
            if (here < 0.0)
              here = -here;
            if (here > max) {
              max = here;
              xpiv = x2;
              ypiv = y2;
            }
          } 
        }
      }
    }
    // Now we have the pivot element, this diagonal is handled.
    pivot[xpiv] = true;
    //
    // Check whether the pivot is on the diagonal. In case it is not, swap
    // rows to move in place.
    if (xpiv != ypiv) {
      for(x1 = 0;x1 < 3;x1++) {
        FLOAT t = m_fInverse[x1 + xpiv * 3];
        m_fInverse[x1 + xpiv * 3] = m_fInverse[x1 + ypiv * 3];
        m_fInverse[x1 + ypiv * 3] = t;
      }
    }
    // Remember the column swap we performed implicitly, need to fixup
    // this later after we're done. The swap operations are indexed by
    // the column here.
    srcrow[x] = xpiv;
    dstrow[x] = ypiv;
    // Now the pivot is on the diagonal at xpiv,xpiv
    //
    // Ready for division by pivot element.
    max = m_fInverse[xpiv + xpiv * 3];
    if (max == 0.0)
      JPG_THROW(INVALID_PARAMETER,"FloatTransformationBox::InvertMatrix",
                "Invalid decorrelation matrix provided, the matrix is not invertible");
    //
    // Divide the pivot row by the pivot element to create a one there,
    // already insert the resulting identity matrix into the target
    // position. At the pivot position, this is a one.
    {
      DOUBLE pivinv = 1.0 / max;
      m_fInverse[xpiv + xpiv * 3] = 1.0f;
      for(x2 = 0;x2 < 3;x2++) {
        // Now perform the division by the pivot.
        m_fInverse[x2 + xpiv * 3] *= pivinv;
      }
    }
    //
    // Reduce rows, except for the pivot row.
    for(y2 = 0;y2 < 3;y2++) {
      if (y2 != xpiv) {
        DOUBLE tmp = m_fInverse[xpiv + y2 * 3];
        m_fInverse[xpiv + y2 * 3] = 0.0f;
        //
        // Subtract a multiple of the pivot row from this row.
        for(x2 = 0;x2 < 3;x2++) {
          m_fInverse[x2 + y2 * 3] -= m_fInverse[x2 + xpiv * 3] * tmp;
        }
      }
    }
  }
  // 
  // Now swap columns back from the column reordering we did.
  for(x = 2;x >= 0;x--) {
    if (srcrow[x] != dstrow[x]) {
      x1 = srcrow[x];
      x2 = dstrow[x];
      for(y2 = 0;y2 < 3;y2++) {
        FLOAT t = m_fInverse[x1 + y2 * 3];
        m_fInverse[x1 + y2 * 3] = m_fInverse[x2 + y2 * 3];
        m_fInverse[x2 + y2 * 3] = t;
      }
    }
  }

  m_bInverseValid = true;
}
///
