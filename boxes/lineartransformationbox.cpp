/*
** This box keeps a linear transformation that can be used either as L
** or C transformation.
**
** $Id: lineartransformationbox.cpp,v 1.5 2015/03/26 09:28:00 thor Exp $
**
*/

/// Includes
#include "std/string.hpp"
#include "boxes/box.hpp"
#include "boxes/lineartransformationbox.hpp"
#include "colortrafo/colortrafo.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// LinearTransformationBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. 
bool LinearTransformationBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  int i;
  LONG b;

  if (boxsize != 1 + 9 * 2)
    JPG_THROW(MALFORMED_STREAM,"LinearTransformationBox::ParseBoxContent",
	      "malformed JPEG stream, size of the linear transformation box is inccorrect");
  
  b = stream->Get();
  if (b == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"LinearTransformationBox::ParseBoxContent",
	      "malformed JPEG stream, unexpected EOF while parsing the linear transformation box");
  
  //
  // The ID must be between 5 and 15.
  m_ucID = b >> 4;
  if (m_ucID < 5 || m_ucID > 15)
    JPG_THROW(MALFORMED_STREAM,"LinearTransformationBox::ParseBoxContent",
	      "malformed JPEG stream, the M value of a linear transformation box is out of range");

  //
  // The t value (fractional bits) must be 13.
  if ((b & 0x0f) != ColorTrafo::FIX_BITS)
    JPG_THROW(MALFORMED_STREAM,"LinearTransformationBox::ParseBoxContent",
	      "malformed JPEG stream, the t value of a linear transformation box is invalid");

  for(i = 0;i < 9;i++) {
    b = stream->GetWord();
    if (b == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"LinearTransformationBox::ParseBoxContent",
		"malformed JPEG stream, unexpected EOF while parsing the linear transformation box");

    m_lMatrix[i] = WORD(b);
  }

  return true;
}
///

/// LinearTransformationBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
bool LinearTransformationBox::CreateBoxContent(class MemoryStream *target)
{
  int i;

  assert(m_ucID >= 5 && m_ucID <= 15);

  target->Put((m_ucID << 4) | ColorTrafo::FIX_BITS);

  for(i = 0;i < 9;i++) {
    LONG v = m_lMatrix[i];
    assert(v >= MIN_WORD && v <= MAX_WORD);
    
    target->PutWord(v);
  }

  return true;
}
///

/// LinearTransformationBox::InvertMatrix
// Compute the inverse matrix and put it into m_lInverse. If it does not
// exist, throw.
void LinearTransformationBox::InvertMatrix(void)
{
  int x1,x;
  int x2,y2;
  int xpiv,ypiv;
  bool pivot[3] = {false,false,false};
  int srcrow[3];
  int dstrow[3]; // collects column and row swaps
  
  memcpy(m_lInverse,m_lMatrix,sizeof(m_lMatrix));

  // Loop over the columns.
  for(x = 0;x < 3;x++) {
    LONG max = 0;
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
	    LONG here = m_lInverse[x2 + y2 * 3];
	    // Compute the max as absolute value of the entry.
	    if (here < 0)
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
	LONG t = m_lInverse[x1 + xpiv * 3];
	m_lInverse[x1 + xpiv * 3] = m_lInverse[x1 + ypiv * 3];
	m_lInverse[x1 + ypiv * 3] = t;
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
    max = m_lInverse[xpiv + xpiv * 3];
    if (max == 0)
      JPG_THROW(INVALID_PARAMETER,"LinearTransformationBox::InvertMatrix",
		"Invalid decorrelation matrix provided, the matrix is not invertible");
    //
    // Divide the pivot row by the pivot element to create a one there,
    // already insert the resulting identity matrix into the target
    // position. At the pivot position, this is a one.
    m_lInverse[xpiv + xpiv * 3] = 1L << ColorTrafo::FIX_BITS;
    for(x2 = 0;x2 < 3;x2++) {
      QUAD tmp = (max >> 1) + (QUAD(m_lInverse[x2 + xpiv * 3]) << ColorTrafo::FIX_BITS);
      //
      // Now perform the division by the pivot.
      tmp /= max;
      if (tmp < MIN_LONG || tmp > MAX_LONG)
	JPG_THROW(INVALID_PARAMETER,"LinearTransformationBox::InvertMatrix",
		"Invalid decorrelation matrix provided, the matrix is close to singlar, cannot invert");
      m_lInverse[x2 + xpiv * 3] = LONG(tmp);
    }
    //
    // Reduce rows, except for the pivot row.
    for(y2 = 0;y2 < 3;y2++) {
      if (y2 != xpiv) {
	QUAD tmp = m_lInverse[xpiv + y2 * 3];
	m_lInverse[xpiv + y2 * 3] = 0;
	//
	// Subtract a multiple of the pivot row from this row.
	for(x2 = 0;x2 < 3;x2++) {
	  m_lInverse[x2 + y2 * 3] -= (m_lInverse[x2 + xpiv * 3] * tmp) >> ColorTrafo::FIX_BITS;
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
	LONG t = m_lInverse[x1 + y2 * 3];
	m_lInverse[x1 + y2 * 3] = m_lInverse[x2 + y2 * 3];
	m_lInverse[x2 + y2 * 3] = t;
      }
    }
  }

  m_bInverseValid = true;
}
///
