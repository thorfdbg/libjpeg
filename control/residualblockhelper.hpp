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
**
** This class computes, prepares or includes residual data for block
** based processing. It abstracts parts of the residual coding
** process.
**
** $Id: residualblockhelper.hpp,v 1.13 2012-09-14 18:56:35 thor Exp $
**
*/


#ifndef CONTROL_RESIDUALBLOCKHELPER_HPP
#define CONTROL_RESIDUALBLOCKHELPER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "marker/frame.hpp"
///

/// Forwards
struct ImageBitMap;
class DCT;
///

/// Class ResidualBlockHelper
// This class computes, prepares or includes residual data for block
// based processing. It abstracts parts of the residual coding
// process.
class ResidualBlockHelper : public JKeeper {
  //
  // The frame that contains the image.
  class Frame         *m_pFrame;
  //
  // The color transformer.
  class ColorTrafo    *m_pOriginalData;
  //
  // Temporary image bitmaps.
  struct ImageBitMap **m_ppTransformed;
  //
  // Reconstructed data for computing the residual.
  LONG               **m_ppReconstructed;
  //
  // Number of components in the frame, number of components we handle.
  UBYTE                m_ucCount;
  //
  // One point pixel transform for higher bitdepth coding, to
  // be applied before color transformation. This creates
  // residual data potentially coded by the residual scan.
  // Or it creates loss.
  UBYTE                m_ucPointShift;
  //
  // The identity tone mapping transform for the HDR encoding.
  UWORD               *m_pusIdentity;
  //
  // The Hadamard transformation.
  class DCT          **m_ppTrafo;
  //
  // The luma DC quantization factor if Hadamard transformation
  // is disabled.
  UWORD                m_usLumaQuant;
  //
  // The chroma DC quantization factor if Hadamard transformation
  // is disabled.
  UWORD                m_usChromaQuant;
  //
  // Create a trivial color transformation that pulls the data from the source
  class ColorTrafo *CreateTrivialTrafo(class ColorTrafo *ctrafo,UBYTE bpp);
  //
  // Allocate the temporary buffers to hold the residuals and their bitmaps.
  // Only required during encoding.
  void AllocateBuffers(void);
  //
  // Allocate the Hadamard transformations
  // and initialize their quantization factors. Returns false
  // in case the Hadamard transformation is disabled.
  bool BuildTransformations(void);
  //
public:
  //
  ResidualBlockHelper(class Frame *frame);
  //
  ~ResidualBlockHelper(void);
  //
  // Add residual coding errors from a side-channel if there is such a thing.
  void AddResidual(const RectAngle<LONG> &r,const struct ImageBitMap *const *dest,
		   LONG * const *residual,ULONG max);
  //
  // Compute the residuals of a block given the DCT data
  void ComputeResiduals(const RectAngle<LONG> &r,class BlockBitmapRequester *req,
			const struct ImageBitMap *const *source,
			LONG *const *dct,LONG **residual,ULONG max);
  //
};
///

///
#endif
