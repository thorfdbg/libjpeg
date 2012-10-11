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
** This is the hidden version of the (huffman) refinement scan whose
** data goes into a special APP9 marker. It works otherwise like any other
** refinement scan.
**
** $Id: hiddenrefinementscan.hpp,v 1.4 2012-09-28 10:53:49 thor Exp $
**
*/

#ifndef CODESTREAM_HIDDENREFINEMENTSCAN_HPP
#define CODESTREAM_HIDDENREFINEMENTSCAN_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scan.hpp"
#include "io/bitstream.hpp"
#include "io/memorystream.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/entropyparser.hpp"
#include "codestream/sequentialscan.hpp"
#include "codestream/acsequentialscan.hpp"
#include "codestream/refinementscan.hpp"
#include "codestream/acrefinementscan.hpp"
///

/// Forwards
class Tables;
class ByteStream;
class Frame;
class MemoryStream;
class ByteStream;
class Scan;
class ResidualMarker;
///

/// class RefinementScan
template<class BaseScan>
class HiddenScan : public BaseScan {
  //
  //
  // Buffers the output before it is split into APP4 markers.
  class MemoryStream         *m_pResidualBuffer;
  //
  // Where the data finally ends up.
  class ByteStream           *m_pTarget;
  //
  // Where the data is taken from on parsing.
  class ResidualMarker       *m_pMarker;
  //
  // True if this is part of the residual scan.
  bool                        m_bResiduals;
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final);
  // 
  // Restart the parser at the next restart interval
  virtual void Restart(void);
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io);
  //
  // Return the data to process for component c. Will be overridden
  // for residual scan types.
  virtual class QuantizedRow *GetRow(UBYTE idx) const;
  //
  // Check whether there are more rows to process, return true
  // if so, false if not. Start with the row if there are.
  virtual bool StartRow(void) const;
  //
public:
  HiddenScan(class Frame *frame,class Scan *scan,class ResidualMarker *marker,
	     UBYTE start,UBYTE stop,
	     UBYTE lowbit,UBYTE highbit,
	     bool residuals,
	     bool differential);
  //
  ~HiddenScan(void);
  // 
  // Fill in the tables for decoding and decoding parameters in general.
  virtual void StartParseScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Write the default tables for encoding 
  virtual void StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl);
  //
  // Measure scan statistics.
  virtual void StartMeasureScan(class BufferCtrl *ctrl);
};
///

/// Typedefs for convenience
typedef HiddenScan<RefinementScan> HiddenRefinementScan;
typedef HiddenScan<ACRefinementScan> HiddenACRefinementScan;
typedef HiddenScan<SequentialScan> ResidualHuffmanScan;
typedef HiddenScan<ACSequentialScan> ResidualACScan;
///

///
#endif
