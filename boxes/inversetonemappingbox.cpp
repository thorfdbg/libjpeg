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
** This box keeps an inverse tone mapping curve, as required for the
** R and L transformations.
**
** $Id: inversetonemappingbox.cpp,v 1.24 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "inversetonemappingbox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "io/decoderstream.hpp"
#include "std/stdlib.hpp"
///

/// InverseToneMappingBox::~InverseToneMappingBox
InverseToneMappingBox::~InverseToneMappingBox(void)
{
  if (m_plTable)
    m_pEnviron->FreeMem(m_plTable,m_ulTableEntries * sizeof(LONG));

  if (m_plInverseMapping)
    m_pEnviron->FreeMem(m_plInverseMapping,(1UL << (8 + m_ucResidualBits)) * sizeof(LONG));
}
///

/// InverseToneMappingBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box.
bool InverseToneMappingBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG v;
  LONG entries;
  LONG *dt;
  
  if (boxsize > (MAX_UWORD + 1) * 2 + 1)
    JPG_THROW(MALFORMED_STREAM,"InverseToneMappingBox::ParseBoxContent",
              "Malformed JPEG stream, inverse tone mapping box is too large");

  // Size - 1 must be divisible by two - this is the number of entries - and a power of two.
  if ((boxsize & 1) == 0 || boxsize < 256 * 2)
    JPG_THROW(MALFORMED_STREAM,"InverseToneMappingBox::ParseBoxContent",
              "Malformed JPEG stream, number of table entries in the inverse tone mapping box is invalid");

  v                = stream->Get();
  m_ucTableIndex   = v >> 4;
  m_ucResidualBits = v & 0x0f;

  entries = (boxsize - 1) >> 1; // cannot overflow.
  //
  // Is this a power of two?
  if (entries & (entries - 1))
    JPG_THROW(MALFORMED_STREAM,"InverseToneMappingBox::ParseBoxContent",
              "Malformed JPEG stream, number of table entries in the inverse tone mapping box must be a power of two");

  assert(m_plTable == NULL);

  m_ulTableEntries = entries;
  dt = m_plTable   = (LONG *)(m_pEnviron->AllocMem(entries * sizeof(LONG)));
  
  if (m_ucResidualBits <= 8) {
    while(entries) {
      *dt++ = stream->GetWord();
      entries--;
    }
  } else { 
    while(entries) {
      LONG hi = stream->GetWord();
      LONG lo = stream->GetWord();
      *dt++   = (hi << 16) | lo;
      entries--;
    }
  }
  
  return true; // is parsed off
}
///

/// InverseToneMappingBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns the buffer where the data is in - the box may use its own buffer.
bool InverseToneMappingBox::CreateBoxContent(class MemoryStream *target)
{
  LONG *dt      = m_plTable;
  ULONG entries = m_ulTableEntries;

  assert(m_plTable);

  target->Put((m_ucTableIndex << 4) | m_ucResidualBits);

  if (m_ucResidualBits <= 8) {
    while(entries) {
      target->PutWord(*dt++);
      entries--;
    }
  } else { 
    while(entries) {
      LONG e = *dt++;
      target->PutWord(WORD(e >> 16));
      target->PutWord(WORD(e >>  0));
      entries--;
    }
  }
  
  return true;
}
///

/// InverseToneMappingBox::DefineTable
// Define the table from an external source.
void InverseToneMappingBox::DefineTable(UBYTE tableidx,const UWORD *table,ULONG size,UBYTE residualbits)
{
  ULONG i;

  assert(m_plTable == NULL);
  assert((size & (size - 1)) == 0);
  assert(size);

  m_plTable        = (LONG *)m_pEnviron->AllocMem(size * sizeof(LONG));
  m_ulTableEntries = size;
  for(i = 0;i < size;i++)
    m_plTable[i]   = table[i];

  m_ucTableIndex   = tableidx;
  m_ucResidualBits = residualbits;
}
///

/// InverseToneMappingBox::CompareTable
// Check whether the given table is identical to the table stored here, and thus
// no new index is required (to save rate). Returns true if the two are equal.
bool InverseToneMappingBox::CompareTable(const UWORD *table,ULONG size,UBYTE residualbits) const
{
  if (m_ulTableEntries == size && m_ucResidualBits == residualbits && table && m_plTable) {
    ULONG i;
    for(i = 0;i < size;i++) {
      if (m_plTable[i] != table[i])
        return false;
    }
    return true;
  }
  return false;
}
///

/// InverseToneMappingBox::ScaledTableOf
// Return a table that maps inputs in the range 0..2^inputbits-1
// to output bits in the range 0..2^oututbits-1.
const LONG *InverseToneMappingBox::ScaledTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract)
{
  // Check whether the table fits. The output bits must fit to the residual bits
  // generated by the LUT.
  if (outputbits + outfract != 8 + m_ucResidualBits)
    JPG_THROW(INVALID_PARAMETER,"InverseToneMappingBox::ScaledTableOf",
              "Codestream is requesting a tone mapping that does not fit to the output bit precision.");
  //
  // Check whether the size of the table fits.
  if (inputbits > 16 || (1UL << inputbits) != m_ulTableEntries)
    JPG_THROW(INVALID_PARAMETER,"InverseToneMappingBox::ScaledTableOf",
              "Codestream is requesting a tone mapping that does not fit to the input bit precision.");

  if (infract != 0)
    JPG_THROW(INVALID_PARAMETER,"InverseToneMappingBox::ScaledTableOf",
              "Codestream is requesting a lookup table in a path that requires fractional bits");

  assert(m_plTable);

  return m_plTable;
}
///

/// InverseToneMappingBox::InverseScaledTableOf
// Return the inverse of the table, where the first argument is the number
// of bits in the DCT domain (the output bits) and the second argument is
// the number of bits in the spatial (image) domain, i.e. the argument
// order is identical to that of the backwards table generated above.
const LONG *InverseToneMappingBox::InverseScaledTableOf(UBYTE dctbits,UBYTE spatialbits,UBYTE dctfract,UBYTE spatialfract)
{
  // Check whether the table fits. The output bits must fit to the residual bits
  // generated by the LUT.
  if (spatialbits + spatialfract != 8 + m_ucResidualBits)
    JPG_THROW(INVALID_PARAMETER,"InverseToneMappingBox::InverseScaledTableOf",
              "Codestream is requesting a tone mapping that does not fit to the output bit precision.");

  // Check whether the size of the table fits.
  if (dctbits > 16 || (1UL << dctbits) != m_ulTableEntries)
    JPG_THROW(INVALID_PARAMETER,"InverseToneMappingBox::InverseScaledTableOf",
              "Codestream is requesting a tone mapping that does not fit to the input bit precision.");
  
  if (dctfract != 0)
    JPG_THROW(INVALID_PARAMETER,"InverseToneMappingBox::InverseScaledTableOf",
              "Codestream is requesting a lookup table in a path that requires fractional bits");

  assert(m_plTable);

  if (m_plInverseMapping == NULL) {
    LONG j,lastj,lastanchor;
    LONG last,current,mid;
    LONG outmax = (1L << (spatialbits + spatialfract)) - 1;
    LONG inmax  = (1L << (dctbits     + dctfract))     - 1;
    bool lastfilled;
    
    m_plInverseMapping = (LONG *)m_pEnviron->AllocMem((1 << (spatialbits + spatialfract)) * sizeof(LONG));
    // Nnot guaranteed that the mapping is surjective onto the output
    // range. There is nothing that says how to handle this case. We just define
    // "undefined" outputs to zero, and try our best to continue the missing parts
    // continuously along the output range. 
    memset(m_plInverseMapping,0,(1 << (spatialbits + spatialfract)) * sizeof(LONG));
    //
    // Loop over positive coefficients.
    lastj      = inmax;
    lastanchor = inmax;
    lastfilled = false;
    j          = inmax;
    last       = outmax;
    //
    // Go from max to zero. This direction is intentional.
    do {
      // Get the next possible output for the given input.
      // Swap back to get the proper endianness.
      current = m_plTable[j];
      // If the function jumps, fill in half the values with the old
      // the other half with the new values. The output is never
      // swapped here, otherwise the table would grow out of range
      // too easily.
      if (current == last) {
        // Found a "flat" area, i.e. had the same external value for 
        // similar internal values. If so, fill in the midpoint 
        // into the table. If lastanchor + j overflows, then our
        // tables are far too huge in first place.
        m_plInverseMapping[last] = (lastanchor + j) >> 1;
        lastfilled               = true;
      } else {
        // Found a "steep" part of the output curve. 
        // If the function jumps, fill in half the values with the old
        // the other half with the new values. The output is never
        // swapped here, otherwise the table would grow out of range
        // too easily.
      if (last > current) {
        mid = ((current + last + 1) >> 1) - 1;
      } else {
        mid = ((current + last - 1) >> 1) - 1;
      }
      while(last != mid) {
        if (lastfilled == false) // Do not overwrite the flat area from the last time.
          m_plInverseMapping[last] = lastj;
        if (last > mid)    last--;
        else               last++;
        lastfilled = false;
      }
      while(last != current) {
        if (lastfilled == false) // Do not overwrite the flat area from the last time.
          m_plInverseMapping[last]  = j;
        if (last > current) last--;
        else                last++;
        lastfilled = false;
      }
      lastanchor = j;
      }
      lastj = j;
      last  = current;
    } while(j--); // This includes the case j = 0 in the inner loop
    // Now we could have the situation that "lastfilled" is still false,
    // thus lut[last] is not yet filled. j is now -1 and thus not
    // usable, lastj == 0, and there is no further point to extrapolate
    // to. Thus, set to the exact end-point.
    if (lastfilled == false || lastj == 0) /* make the zero exactly reproducible */
      m_plInverseMapping[last] = lastj;
    //
    // Fixup the ends of the table. If the start or the end of the LUT have a very low slope,
    // we will find jumps in the table that are likely undesired. Fix them up here to avoid
    // artefacts in the image.
    if (outmax > 4) {
      LONG i1,i2,i3;
      LONG d1,d2;
      //
      i1 = m_plInverseMapping[0];
      i2 = m_plInverseMapping[1];
      i3 = m_plInverseMapping[2];
      //
      d1 = (i1 > i2)?(i1 - i2):(i2 - i1);
      d2 = (i3 > i2)?(i3 - i2):(i2 - i3);
      //
      // If the first jump is too large, clip it.
      if (d1 > 2 * d2)
        m_plInverseMapping[0] = 2 * i2 - i3;
      //
      // Now at the other end. Note that max is inclusive.
      i1 = m_plInverseMapping[outmax];
      i2 = m_plInverseMapping[outmax - 1];
      i3 = m_plInverseMapping[outmax - 2];
      //
      d1 = (i1 > i2)?(i1 - i2):(i2 - i1);
      d2 = (i3 > i2)?(i3 - i2):(i2 - i3);
      //
      if (d1 > 2 * d2)
        m_plInverseMapping[outmax] = 2 * i2 - i3;
    }
  }
  //
  return m_plInverseMapping;
}
///
