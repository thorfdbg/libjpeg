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
** This class represents the quantization tables.
**
** $Id: quantization.hpp,v 1.16 2021/11/15 07:39:43 thor Exp $
**
*/

#ifndef MARKER_QUANTIZATION_HPP
#define MARKER_QUANTIZATION_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
class QuantizationTable;
///

/// class Quantization
// This class describes the quantization tables for lossless JPEG coding.
class Quantization : public JKeeper {
  //
  // The actual quantization tables. This marker can hold up to four of them.
  class QuantizationTable *m_pTables[4];
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
                         LONG tableselector,UBYTE precision,
                         const LONG customluma[64],
                         const LONG customchroma[64]);
  //
  class QuantizationTable *QuantizationTable(UBYTE idx) const
  {
    assert(idx < 4);
    return m_pTables[idx];
  }
};
///

///
#endif
