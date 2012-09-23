/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** This class represents the APP9 marker carrying the tone mapping curve
** required to restore the HDR data from the LDR approximation.
**
** $Id: tonemappingmarker.cpp,v 1.4 2012-09-15 21:45:51 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "std/assert.hpp"
#include "io/bytestream.hpp"
#include "marker/tonemappingmarker.hpp"
///

/// ToneMappingMarker::ToneMappingMarker
ToneMappingMarker::ToneMappingMarker(class Environ *env)
  : JKeeper(env), m_pNext(NULL), m_ucIndex(0), m_ucDepth(0), 
    m_pusMapping(NULL), m_pusInverseMapping(NULL)
{
}
///

/// ToneMappingMarker::~ToneMappingMarker
ToneMappingMarker::~ToneMappingMarker(void)
{
  if (m_pusMapping)
    m_pEnviron->FreeMem(m_pusMapping,(1 << m_ucInternalDepth) * sizeof(UWORD));

  if (m_pusInverseMapping)
    m_pEnviron->FreeMem(m_pusInverseMapping,(1 << m_ucDepth) * sizeof(UWORD));
}
///

/// ToneMappingMarker::BuildInverseMapping
// Build the encoding tone mapper from the inverse mapping.
void ToneMappingMarker::BuildInverseMapping(void)
{ 
  UWORD j,lastj,lastanchor;
  UWORD last,current,mid;
  UWORD outmax = (1 << m_ucDepth)         - 1;
  UWORD inmax  = (1 << m_ucInternalDepth) - 1;
  bool lastfilled;

  // Check the luts
  assert(m_pusMapping);
  assert(m_pusInverseMapping == NULL);
  
  m_pusInverseMapping = (UWORD *)m_pEnviron->AllocMem((1 << m_ucDepth) * sizeof(UWORD));
  // Nnot guaranteed that the mapping is surjective onto the output
  // range. There is nothing that says how to handle this case. We just define
  // "undefined" outputs to zero, and try our best to continue the missing parts
  // continously along the output range. 
  memset(m_pusInverseMapping,0,(1 << m_ucDepth) * sizeof(UWORD));
  //
  // Loop over positive coefficients.
  lastj      = inmax;
  lastanchor = inmax;
  lastfilled = false;
  j          = inmax;
  last       = m_pusMapping[j];
  //
  // Try some guesswork whether we should extend this to fullrange.
  // This avoids trouble in case the backwards mapping is not
  // onto, but we find the corresponding out-of-range pixels in the
  // input image.
  if (last < (((outmax + 1) * 3) >> 2)) {
    last = outmax;
  }
  //
  //
  // Go from max to zero. This direction is intentional.
  do {
    // Get the next possible output for the given input.
    // Swap back to get the proper endianness.
    current = m_pusMapping[j];
    // If the function jumps, fill in half the values with the old
    // the other half with the new values. The output is never
    // swapped here, otherwise the table would grow out of range
    // too easily.
    if (current == last) {
      // Found a "flat" area, i.e. had the same external value for 
      // similar internal values. If so, fill in the midpoint 
      // into the table. If lastanchor + j overflows, then our
      // tables are far too huge in first place.
      m_pusInverseMapping[last] = (lastanchor + j) >> 1;
      lastfilled                = true;
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
	  m_pusInverseMapping[last] = lastj;
	if (last > mid)    last--;
	else               last++;
	lastfilled = false;
      }
      while(last != current) {
	if (lastfilled == false) // Do not overwrite the flat area from the last time.
	  m_pusInverseMapping[last]  = j;
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
  if (lastfilled == false)
    m_pusInverseMapping[last] = lastj;
  //
  // Fixup the ends of the table. If the start or the end of the LUT have a very low slope,
  // we will find jumps in the table that are likely undesired. Fix them up here to avoid
  // artefacts in the image.
  if (outmax > 4) {
    UBYTE i1,i2,i3;
    UBYTE d1,d2;
    //
    i1 = m_pusInverseMapping[0];
    i2 = m_pusInverseMapping[1];
    i3 = m_pusInverseMapping[2];
    //
    d1 = (i1 > i2)?(i1 - i2):(i2 - i1);
    d2 = (i3 > i2)?(i3 - i2):(i2 - i3);
    //
    // If the first jump is too large, clip it.
    if (d1 > 2 * d2)
      m_pusInverseMapping[0] = 2 * i2 - i3;
    //
    // Now at the other end. Note that max is inclusive.
    i1 = m_pusInverseMapping[outmax];
    i2 = m_pusInverseMapping[outmax - 1];
    i3 = m_pusInverseMapping[outmax - 2];
    //
    d1 = (i1 > i2)?(i1 - i2):(i2 - i1);
    d2 = (i3 > i2)?(i3 - i2):(i2 - i3);
    //
    if (d1 > 2 * d2)
      m_pusInverseMapping[outmax] = 2 * i2 - i3;
  }
  //
#if 0
  for(j = 0;j <= outmax;j++)
    fprintf(stderr,"%d\t%d\n",j,m_pusInverseMapping[j]);
  fprintf(stderr,"\n\n");
#endif
}
///

/// ToneMappingMarker::ParseMarker
// Parse the tone mapping marker from the stream
// This will throw in case the marker is not
// recognized. The caller will have to catch
// the exception.
void ToneMappingMarker::ParseMarker(class ByteStream *io,UWORD len)
{
  UBYTE dt;
  UWORD i;

  assert(m_pusMapping == NULL);
  
  if (len < 2 + 2 + 4 + 1) // Marker length, index, depth
    JPG_THROW(MALFORMED_STREAM,"ToneMappingMarker::ParseMarker","APP9 tone mapping information marker size too short");

  dt           = io->Get();
  m_ucIndex    = dt >> 4;
  m_ucDepth    = (dt & 0x0f) + 8;

  len         -= 2 + 2 + 4 + 1;
  if (len < 256 * 2)
    JPG_THROW(MALFORMED_STREAM,"ToneMappingMarker::ParseMarker",
	      "APP9 tone mapping information marker size invalid");
  
  for(m_ucInternalDepth = 0;m_ucInternalDepth <= 14;m_ucInternalDepth++) {
    if (len == 2 << m_ucInternalDepth)
      break; // Found the right depth
  }

  //
  // Must be a power of two.
  if (m_ucInternalDepth > 14)
    JPG_THROW(MALFORMED_STREAM,"ToneMappingMarker::ParseMarker",
	      "APP9 tone mapping information marker size invalid");

  m_pusMapping = (UWORD *)m_pEnviron->AllocMem((1 << m_ucInternalDepth) * sizeof(UWORD));

  for(i = 0;i < (1 << m_ucInternalDepth);i++) {
    m_pusMapping[i] = io->GetWord();
  }
}
///

/// ToneMappingMarker::InstallDefaultParameters
// Install parameters - here the bpp value and the tone mapping curve.
void ToneMappingMarker::InstallDefaultParameters(UBYTE idx,UBYTE bpp,UBYTE hidden,
						 const UWORD *curve)
{
  assert(m_pusMapping == NULL);

  if (bpp > 16 || bpp < 8)
    JPG_THROW(INVALID_PARAMETER,"ToneMappingMarker::InstallDefaultParameters",
	      "tone mapping bitdepth is out of range, must be between 8 and 16");
  if (idx > 15)
    JPG_THROW(INVALID_PARAMETER,"ToneMappingMarker::InstallDefaultParameters",
	      "tone mapping identifier is out of range, must be between 0 and 15");
  
  m_ucIndex         = idx;
  m_ucDepth         = bpp;
  m_ucInternalDepth = 8 + hidden;

  // Size limitation of the marker: We can spend at most 15 bits in total.
  if (m_ucInternalDepth > 14)
    JPG_THROW(OVERFLOW_PARAMETER,"ToneMappingMarker::InstallDefaultParameters",
	      "the total number of bits available for the internal sample representation "
	      "must not exceed 14");
  
  m_pusMapping      = (UWORD *)m_pEnviron->AllocMem((1UL << m_ucInternalDepth) * sizeof(UWORD));
  
  memcpy(m_pusMapping,curve,(1UL << m_ucInternalDepth) * sizeof(UWORD));
}
///

/// ToneMappingMarker::WriteMarker
void ToneMappingMarker::WriteMarker(class ByteStream *target)
{  
  UWORD i;
  
  assert(m_pusMapping);
  // Check above ensured that this cannot exceed 64K
  target->PutWord(2 + 2 + 4 + 1 + (1UL << m_ucInternalDepth) * 2);
  // Write the ID
  target->Put('J');
  target->Put('P');
  target->Put('T');
  target->Put('O');
  target->Put('N');
  target->Put('E'); 
  target->Put((m_ucIndex << 4) | (m_ucDepth - 8));
  
  for(i = 0;i < (1 << m_ucInternalDepth);i++) {
    target->PutWord(m_pusMapping[i]);
  }
}
///

