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
** This class defines the arbitrary color transformation defined
** in JPEG-LS part-2. It is - in a sense - a special case of the 
** JPEG 2000 part-2 reversible color transformation.
**
** $Id: lscolortrafo.cpp,v 1.3 2012-07-20 16:30:00 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "marker/lscolortrafo.hpp"
#include "io/bytestream.hpp"
///

/// LSColorTrafo::LSColorTrafo
// Clean the marker.
LSColorTrafo::LSColorTrafo(class Environ *env)
  : JKeeper(env), m_ucDepth(0), m_usNear(0), m_usMaxTrans(0),
    m_pucInputLabels(NULL), m_pucShift(NULL), m_pbCentered(NULL),
    m_pusMatrix(NULL)
{ }
///

/// LSColorTrafo::~LSColorTrafo
// Release all the memory.
LSColorTrafo::~LSColorTrafo(void)
{
  if (m_pucInputLabels) 
    m_pEnviron->FreeMem(m_pucInputLabels,m_ucDepth * sizeof(UBYTE));

  if (m_pucShift)
    m_pEnviron->FreeMem(m_pucShift      ,m_ucDepth * sizeof(UBYTE));

  if (m_pbCentered)
    m_pEnviron->FreeMem(m_pbCentered    ,m_ucDepth * sizeof(bool));

  if (m_pusMatrix)
    m_pEnviron->FreeMem(m_pusMatrix     ,m_ucDepth * sizeof(UWORD) * (m_ucDepth - 1));
}
///

/// LSColorTrafo::WriteMarker
// Write the marker contents to a LSE marker.
void LSColorTrafo::WriteMarker(class ByteStream *io)
{
  UBYTE i,j;
  LONG len = 2 + 1 + 2 + 1 + 2 * m_ucDepth * m_ucDepth;

  if (len > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"LSColorTrafo::WriteMarker",
	      "too many components, cannot create a LSE color transformation marker");

  io->PutWord(len);
  io->Put(0x0d);    // Type of the LSE marker.
  io->PutWord(m_usMaxTrans);
  io->Put(m_ucDepth);

  // Write the component labels.
  for(i = 0;i < m_ucDepth;i++) {
    io->Put(m_pucInputLabels[i]);
  }

  // Write the transformation matrix.
  for(i = 0;i < m_ucDepth;i++) {
    UBYTE v = m_pucShift[i];
    if (m_pbCentered[i])
      v |= 0x80;
    io->Put(v);
    for(j = 0;j < m_ucDepth - 1;j++) {
      io->PutWord(m_pusMatrix[i * (m_ucDepth - 1) + j]);
    }
  }
}
///

/// LSColorTrafo::ParseMarker
// Parse the marker contents of a LSE marker.
// marker length and ID are already parsed off.
void LSColorTrafo::ParseMarker(class ByteStream *io,UWORD len)
{
  UBYTE i,j;
  
  if (len < 6)
    JPG_THROW(MALFORMED_STREAM,"LSColorTrafo::ParseMarker",
	      "length of the LSE color transformation marker is invalid, "
	      "must be at least six bytes long");
  
  m_usMaxTrans = io->GetWord();
  m_ucDepth    = io->Get();
  len         -= 6;

  if (len != 2 * m_ucDepth * m_ucDepth)
    JPG_THROW(MALFORMED_STREAM,"LSColorTrafo::ParseMarker",
	      "length of the LSE color transformation marker is invalid");

  if (m_ucDepth == 0)
    JPG_THROW(MALFORMED_STREAM,"LSColorTrafo::ParseMarker",
	      "number of components in the LSE color transformation marker must not be zero");

  // Read the input labels of the components to be transformed.
  assert(m_pucInputLabels == NULL);
  m_pucInputLabels = (UBYTE *)m_pEnviron->AllocMem(m_ucDepth * sizeof(UBYTE));
  
  for(i = 0;i < m_ucDepth;i++) {
    m_pucInputLabels[i] = io->Get();
  }

  // Allocate the shift, centered and matrix arrays.
  assert(m_pucShift == NULL && m_pbCentered == NULL && m_pusMatrix == NULL);
  
  m_pucShift   = (UBYTE *)m_pEnviron->AllocMem(m_ucDepth * sizeof(UBYTE));
  m_pbCentered = (bool  *)m_pEnviron->AllocMem(m_ucDepth * sizeof(bool));
  m_pusMatrix  = (UWORD *)m_pEnviron->AllocMem(m_ucDepth * sizeof(UWORD) * (m_ucDepth - 1));

  for(i = 0;i < m_ucDepth;i++) {
    // The flags & shift byte.
    UBYTE v         = io->Get();
    m_pbCentered[i] = (v & 0x80)?true:false;
    m_pucShift[i]   = v & 0x7f;
    if (m_pucShift[i] > 32)
      JPG_THROW(OVERFLOW_PARAMETER,"LSColorTrafo::ParseMarker",
		"LSE color transformation marker shift value is too large, must be < 32");
    // And the matrix itself.
    for(j = 0;j < m_ucDepth - 1;j++) {
      m_pusMatrix[j + i * (m_ucDepth - 1)] = io->GetWord();
    }
  }
}
///

/// LSColorTrafo::InstallDefaults
// Install the defaults for a given sample count. This
// installs the example pseudo-RCT given in the specs.
void LSColorTrafo::InstallDefaults(UBYTE bpp,UBYTE near)
{
  assert(m_pucInputLabels == NULL);
  assert(m_pucShift == NULL && m_pbCentered == NULL && m_pusMatrix == NULL);

  // The default is here the 3x3 pseudo-RCT
  m_ucDepth        = 3;
  m_usMaxTrans     = (1 << bpp) - 1;
  // Error bound on the transformed components: This is the worst case
  // error created for the pseudo-RCT transformation: The output of
  // the R and G components can differ by the error of the restored
  // green plus the error of the restored component itself, as the
  // reconstructed R is:
  // R = R' + G' - floor(R' + G' / 4). 
  // Thus, the worst case is as given below.
  m_usNear         = near + ((3 * near + 3) >> 2);
  // Allocate the labels.
  m_pucInputLabels = (UBYTE *)m_pEnviron->AllocMem(m_ucDepth * sizeof(UBYTE));
  m_pucShift   = (UBYTE *)m_pEnviron->AllocMem(m_ucDepth * sizeof(UBYTE));
  m_pbCentered = (bool  *)m_pEnviron->AllocMem(m_ucDepth * sizeof(bool));
  m_pusMatrix  = (UWORD *)m_pEnviron->AllocMem(m_ucDepth * sizeof(UWORD) * (m_ucDepth - 1));

  // This code assigns input labels identical to the component index, thus
  // the input labels are 0,1,2, though not in that order. Green requires
  // the components red and blue, thus has to go first.
  m_pucInputLabels[0] = 1; // green
  m_pucInputLabels[1] = 0; // red
  m_pucInputLabels[2] = 2; // blue

  // Shift by two and centered. 
  m_pucShift[0]       = 2;
  m_pbCentered[0]     = true; // subtract
  m_pusMatrix[0]      = 1; // add red with factor of one
  m_pusMatrix[1]      = 1; // and blue with a factor of one

  // Compute the output from R = G + Cr
  m_pucShift[1]       = 0;
  m_pbCentered[1]     = false;
  m_pusMatrix[2]      = 1; // add G
  m_pusMatrix[3]      = 0; // Cb is not used
  
  // Compute the output from B = G + Cb
  m_pucShift[2]       = 0;
  m_pbCentered[2]     = false;
  m_pusMatrix[4]      = 1; // add G
  m_pusMatrix[5]      = 0; // B is not used
}
///

