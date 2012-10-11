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
** This class keeps all the coding tables, huffman, AC table, quantization
** and other side information.
**
** $Id: tables.hpp,v 1.31 2012-10-07 15:58:08 thor Exp $
**
*/

#ifndef CODESTREAM_TABLES_HPP
#define CODESTREAM_TABLES_HPP

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Forwards
class ByteStream;
class Quantization;
class HuffmanTemplate;
class JFIFMarker;
class AdobeMarker;
class ResidualMarker;
class ResidualSpecsMarker;
class ToneMappingMarker;
class ColorTrafo;
class EXIFMarker;
class LosslessMarker;
class ACTable;
class RestartIntervalMarker;
class NumberOfLinesMarker;
class Thresholds;
///

/// class Tables
class Tables: public JKeeper {
  //
  // The quantization table.
  class Quantization          *m_pQuant;
  //
  // The huffman table.
  class HuffmanTable          *m_pHuffman;
  //
  // The AC table.
  class ACTable               *m_pConditioner;
  //
  // The restart interval definition if there is one.
  class RestartIntervalMarker *m_pRestart;
  //
  // The adobe color marker.
  class AdobeMarker           *m_pColorInfo;
  //
  // The JFIF marker.
  class JFIFMarker            *m_pResolutionInfo;
  //
  // Exif data.
  class EXIFMarker            *m_pCameraInfo;
  //
  // The marker containing the residual data information
  // for lossless compression.
  class ResidualMarker        *m_pResidualData;
  //
  // This marker contains the hidden refinement data if
  // this feature is used
  class ResidualMarker        *m_pRefinementData;
  //
  // Specifications of the residual data.
  class ResidualSpecsMarker   *m_pResidualSpecs;
  //
  // The color transformer
  class ColorTrafo            *m_pColorTrafo;
  //
  // The lossless indication marker.
  class LosslessMarker        *m_pLosslessMarker;
  //
  // The tone mapping markers.
  class ToneMappingMarker     *m_pToneMappingMarker;
  //
  // The thresholds for JPEG LS
  class Thresholds            *m_pThresholds;
  //
  // The extended reversible color transformation information coming
  // from JPEG LS
  class LSColorTrafo          *m_pLSColorTrafo;
  //
  // This is the tone mapping curve for the simple shift operation, i.e. in
  // case a preshift is present and no tone mapping marker is available.
  UWORD                       *m_pusToneMapping;
  //
  // Mapping on encoding, downshifting as a table.
  UWORD                       *m_pusInverseMapping;
  //
  // Boolean indicator whether the fixpoint code must be used.
  bool                         m_bForceFixpoint;
  //
  // Boolean indicator that the color trafo must be off.
  bool                         m_bDisableColor;
  //
  // Find the tone mapping curve for encoding that performs the HDR to LDR mapping
  const UWORD *FindEncodingToneMappingCurve(UBYTE comp,UBYTE ldrbpp);
  //
  // Find the tone mapping curve for decoding that runs the LDR to HDR mapping.
  const UWORD *FindDecodingToneMappingCurve(UBYTE comp,UBYTE ldrbpp);
  //
public:
  Tables(class Environ *env);
  //
  ~Tables(void);
  //
  // Parse off tables, including an application marker,
  // comment, huffman tables or quantization tables.
  // Returns on the first unknown marker.
  void ParseTables(class ByteStream *io);
  //
  // Write the tables to the codestream.
  void WriteTables(class ByteStream *io);
  //
  // For writing, install the standard suggested tables
  // for the given quality index between 0 and 100.
  // If colortrafo is set, the image is transformed into
  // YCbCr space first.
  void InstallDefaultTables(const struct JPG_TagItem *tags);
  //
  // Find the DC huffman table of the indicated index.
  class HuffmanTemplate *FindDCHuffmanTable(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden,bool residual) const;
  //
  // Find the AC huffman table of the indicated index.
  class HuffmanTemplate *FindACHuffmanTable(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden,bool residual) const;
  //
  // Find the AC conditioner table for the indicated index
  // and the DC band.
  class ACTemplate *FindDCConditioner(UBYTE idx) const;
  //
  // The same for the AC band.
  class ACTemplate *FindACConditioner(UBYTE idx) const;
  //
  // Find the quantization table of the given index.
  const UWORD *FindQuantizationTable(UBYTE idx) const;
  //
  // Return the residual data if any.
  class ResidualMarker *ResidualDataOf(void) const;
  //
  // Return the refinement data if any.
  class ResidualMarker *RefinementDataOf(void) const;
  //
  // Return the information on the residual data if any.
  class ResidualSpecsMarker *ResidualSpecsOf(void) const;
  //
  // Return the thresholds of JPEG LS or NULL if there are none.
  class Thresholds *ThresholdsOf(void) const
  {
    return m_pThresholds;
  }
  //
  // Return the color transformer suitable for the external data
  // type and the color space indicated in the application markers.
  class ColorTrafo *ColorTrafoOf(class Frame *frame,
				 UBYTE external_type,UBYTE bpp,UBYTE count,bool encoding); 
  //
  // Check whether the integer reversible SERMS based DCT shall be used.
  bool UseReversibleDCT(void) const;
  //
  // Check whether residual data in the APP9 marker shall be written.
  bool UseResiduals(void) const;
  //
  // Check whether the refinement data in APP9 shall be written.
  bool UseRefinements(void) const;
  //
  // Check how many fractional bits the color transformation will use.
  UBYTE FractionalColorBitsOf(UBYTE count) const;
  //
  // Check how many bits are hidden in invisible refinement scans.
  UBYTE HiddenDCTBitsOf(void) const;
  //
  // Check whether color tranformation is enabled.
  bool UseColortrafo(void) const; 
  //
  // Force to use the integer DCT even if the headers say otherwise.
  void ForceIntegerCodec(void);
  //
  // Force to use the fixpoint codec even if the headers say otherwise.
  void ForceFixpointCodec(void);
  //
  // Disable the color transformation even in the absense of the Adobe marker.
  void ForceColorTrafoOff(void);
  //
  // Build the proper DCT transformation for the specification
  // recorded in this class. The DCT is not owned by this class
  // and must be deleted by the caller.
  // Arguments are the quantizer index to use, and the total
  // number of components.
  class DCT *BuildDCT(UBYTE quantizer,UBYTE count) const;
  //
  // Return the currently active restart interval in MCUs or zero
  // in case restart markers are disabled.
  UWORD RestartIntervalOf(void) const;
};
///

///
#endif
