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
** This class is responsible for parsing the huffman specific part of the
** DHT marker and generating the corresponding decoder classes.
**
** $Id: huffmantemplate.hpp,v 1.18 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CODING_HUFFMANTEMPLATE_HPP
#define CODING_HUFFMANTEMPLATE_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Defines
//#define COLLECT_STATISTICS 1
///

/// Forwards
class HuffmanDecoder;
class HuffmanCoder;
class HuffmanStatistics;
///

/// class HuffmanTemplate
class HuffmanTemplate : public JKeeper {
  //
  // Number of huffman codes of length L_i.
  UBYTE                    m_ucLengths[16];
  //
  // Total number of codewords.
  ULONG                    m_ulCodewords;
  //
  // Array containing the huffman values for each code, ordered by code lengths.
  UBYTE                   *m_pucValues;
  //
  // The huffman encoder for this level.
  class HuffmanCoder      *m_pEncoder;
  //
  // The huffman decoder for this level.
  class HuffmanDecoder    *m_pDecoder;
  //
  // The huffman statistics collector, used to optimize the
  // huffman encoder.
  class HuffmanStatistics *m_pStatistics;
  //
#ifdef COLLECT_STATISTICS
  // The AC/DC switch. True for AC
  bool                     m_bAC;
  //
  // The chroma/luma switch. True for chroma.
  bool                     m_bChroma;
  //
#endif
  //
  // Reset the huffman table for an alphabet with N entries.
  void ResetEntries(ULONG count);
  //
  // Build the huffman encoder given the template data.
  void BuildEncoder(void);
  //
  // Build the huffman decoder given the template data.
  void BuildDecoder(void);
  //
  // Build the huffman statistics.
  void BuildStatistics(bool fordc);
  //
public:
  HuffmanTemplate(class Environ *env);
  //
  ~HuffmanTemplate(void);
  //
  // Parse the huffman specific part of the DHT table.
  void ParseMarker(class ByteStream *io);
  //
  // Write the huffman table stored here.
  void WriteMarker(class ByteStream *io);
  //
  // Return the space required to write this part of the marker
  UWORD MarkerOverhead(void) const;
  //
  // Default huffman code initializations. These methods just install
  // the default huffman tables found in the standard, other tables
  // might work as well, or should even perform better.
  //
  // Install the default Luminance DC default table.
  void InitDCLuminanceDefault(ScanType type,UBYTE depth,UBYTE hidden);
  //
  // Install the default Chrominance DC table.
  void InitDCChrominanceDefault(ScanType type,UBYTE depth,UBYTE hidden);
  //
  // Install the default Luminance AC default table.
  void InitACLuminanceDefault(ScanType type,UBYTE depth,UBYTE hidden);
  //
  // Install the default Chrominance AC table.
  void InitACChrominanceDefault(ScanType type,UBYTE depth,UBYTE hidden);
  //
  // Use the collected statistics to build an optimized
  // huffman table.
  void AdjustToStatistics(void);
  //
  // Return the decoder (chain).
  class HuffmanDecoder *DecoderOf(void)
  {
    if (m_pDecoder == NULL)
      BuildDecoder();
    return m_pDecoder;
  }
  //
  // Return the encoder.
  class HuffmanCoder *EncoderOf(void)
  {
    if (m_pEncoder == NULL)
      BuildEncoder();
    return m_pEncoder;
  }
  //
  // Return the statistics class.
  class HuffmanStatistics *StatisticsOf(bool fordc)
  {
    if (m_pStatistics == NULL)
      BuildStatistics(fordc);
    return m_pStatistics;
  }
};
///

///
#endif

  
