/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** This class represents the quantization tables.
**
** $Id: quantization.hpp,v 1.14 2015/05/22 09:55:29 thor Exp $
**
*/

#ifndef MARKER_QUANTIZATION_HPP
#define MARKER_QUANTIZATION_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// class Quantization
// This class describes the quantization tables for lossless JPEG coding.
class Quantization : public JKeeper {
  //
  // The actual quantization tables. This marker can hold up to four of them.
  UWORD *m_pDelta[4];
  //
public:
  Quantization(class Environ *env);
  //
  ~Quantization(void);
  //
  // Write the DQT marker to the stream.
  void WriteMarker(class ByteStream *io);
  //
  // Parse off the contents of the DQT marker
  void ParseMarker(class ByteStream *io);
  //
  // Initialize the quantization table to the standard example
  // tables for quality q, q=0...100
  // If "addresidual" is set, additional quantization tables for 
  // residual coding are added into the legacy quantization matrix.
  // If "foresidual" is set, the quantization table is for the residual
  // codestream, using the hdrquality parameter (with known ldr parameters)
  // but injected into the residual codestream.
  // If "rct" is set, the residual color transformation is the RCT which
  // creates one additional bit of precision for lossless. In lossy modes,
  // this bit can be stripped off.
  // The table selector argument specifies which of the build-in
  // quantization table to use. CUSTOM is then a pointer to a custom
  // table if the table selector is custom.
  void InitDefaultTables(UBYTE quality,UBYTE hdrquality,bool colortrafo,
                         bool addresidual,bool forresidual,bool rct,
                         LONG tableselector,
                         const LONG customluma[64],
                         const LONG customchroma[64]);
  //
  const UWORD *QuantizationTable(UBYTE idx) const
  {
    if (idx < 4)
      return m_pDelta[idx];
    return NULL;
  }
};
///

///
#endif
