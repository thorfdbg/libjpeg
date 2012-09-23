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
** This class represents the APP9 marker carrying the specifications
** how the residual data is to be interpreted.
**
** $Id: residualspecsmarker.hpp,v 1.2 2012-09-15 10:11:41 thor Exp $
**
*/

#ifndef MARKER_RESIDUALSPECSMARKER_HPP
#define MARKER_RESIDUALSPECSMARKER_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
class MemoryStream;
class ByteStream;
///

/// class ResidualSpecsMarker
// This marker carries information on how the residual
// data (if any) is encoded
class ResidualSpecsMarker : public JKeeper {
  //
  // Quantization parameter for the extensions marker.
  // Bit 7 is the enable bit (separate quantization),
  // Bits 5,4: Chroma table, bits 0,1: Luma table.
  UBYTE               m_ucQuantization;
  //
  // The preshift value for HD coding. Zero for no preshift.
  // This allows coding of >8 or != 12bpp images with traditional
  // JPEG by pushing the LSBs into this marker.
  UBYTE               m_ucPreshift;
  //
  // The following flags define which tone mapping curve is enabled.
  // If disabled, preshifting is used.
  UBYTE               m_ucToneEnable;
  //
  // The tone mapping curves: This gives for each component X the tone mapping
  // table index.
  UBYTE               m_ucToneMapping[4];
  //
  // The number of hidden DCT bits.
  UBYTE               m_ucHiddenBits;
  //
public:
  //
  ResidualSpecsMarker(class Environ *env);
  //
  ~ResidualSpecsMarker(void);
  //
  // Parse the residual marker from the stream
  // This will throw in case the marker is not
  // recognized. The caller will have to catch
  // the exception.
  void ParseMarker(class ByteStream *io,UWORD length);
  //
  // Return the point preshift, an additional upshift on reconstruction
  // that allows coding of high dynamic range images by traditional
  // JPEG.
  UBYTE PointPreShiftOf(void) const
  {
    return m_ucPreshift;
  }
  //
  // Return the number of hidden bits
  UBYTE HiddenBitsOf(void) const
  {
    return m_ucHiddenBits;
  }
  //
  // Return an indicator whether tone mapping is enabled for the i'th
  // component.
  bool isToneMapped(UBYTE comp) const
  {
    assert(comp < 4);
    
    return (m_ucToneEnable & (1 << comp))?true:false;
  }
  //
  // Return the index of the tone mapping curve for the i'th component
  // if there is one. (Check first!)
  UBYTE ToneMappingTableOf(UBYTE comp) const
  {
    assert(comp < 4);
    assert(m_ucToneEnable & (1 << comp));

    return m_ucToneMapping[comp];
  }
  //
  // Install parameters - here only the maximum coding error.
  void InstallPreshift(UBYTE preshift);
  //
  // Install parameters - here the number of hidden DCT bits.
  void InstallHiddenBits(UBYTE hiddenbits);
  //
  // Install the tone mapping for component X to use the table Y.
  void InstallToneMapping(UBYTE comp,UBYTE table)
  {
    assert(comp < 4);
    
    m_ucToneEnable |= (1 << comp);
    m_ucToneMapping[comp] = table;
  }
  //
  // Install the quantization parameters for luma and chroma
  // for the extensions layer. The arguments are the quantization
  // tables for both luma and chroma.
  void InstallQuantization(UBYTE luma,UBYTE chroma)
  {
    m_ucQuantization &= ~0x33;
    m_ucQuantization  = 0x80 | (luma) | (chroma << 4);
  }
  //
  // Install the hadamard transformation parameter.
  void InstallHadamardTrafo(bool enable)
  {
    m_ucQuantization &= ~0x08;
    if (enable)
      m_ucQuantization |= 0x08;
  }
  //
  // Install the noise shaping option.
  void InstallNoiseShaping(bool enable)
  {
    m_ucQuantization &= ~0x04;
    if (enable)
      m_ucQuantization |= 0x04;
  }
  //
  // Return the chroma quantization matrix index or MAX_UBYTE if it
  // is not defined.
  UBYTE ChromaQuantizationMatrix(void) const
  {
    if (m_ucQuantization & 0x80) {
      return (m_ucQuantization >> 4) & 0x03;
    }
    return MAX_UBYTE;
  }
  //
  // Return the luma quantization matrix index or MAX_UBYTE if it is
  // not defined.
  UBYTE LumaQuantizationMatrix(void) const
  {
    if (m_ucQuantization & 0x80) {
      return m_ucQuantization & 0x03;
    }
    return MAX_UBYTE;
  }
  //
  // Return an indicator whether the Hadamard transformation shall be run.
  bool isHadamardEnabled(void) const
  {
    return (m_ucQuantization & 0x08)?true:false;
  }
  //
  // Return an indicator whether noise shaping is enabled.
  bool isNoiseShapingEnabled(void) const
  {
    return (m_ucQuantization & 0x04)?true:false;
  }
  //
  // Write the marker, where the raw data comes buffered from the
  // indicated memory stream.
  void WriteMarker(class ByteStream *io);
};
///

///
#endif
