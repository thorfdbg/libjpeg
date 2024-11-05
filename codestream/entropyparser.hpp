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
**
** This class represents the interface for parsing the
** entropy coded data in JPEG as part of a single scan.
**
** $Id: entropyparser.hpp,v 1.23 2024/11/05 06:39:25 thor Exp $
**
*/


#ifndef CODESTREAM_ENTROPYPARSER_HPP
#define CODESTREAM_ENTROPYPARSER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "interface/imagebitmap.hpp"
#include "std/assert.hpp"
///

/// Forwards
class ByteStream;
class Scan;
class Frame;
class BitmapCtrl;
class LineAdapter;
class BufferCtrl;
class Checksum;
///

/// class EntropyParser
// This class represents the interface for parsing the
// entropy coded data in JPEG as part of a single scan.
class EntropyParser : public JKeeper {
  // 
  // The restart interval in MCUs
  ULONG                 m_ulRestartInterval;
  //
  // The next restart marker expected or to be written.
  UWORD                 m_usNextRestartMarker;
  //
  // Number of MCUs to be handled before the next MCU
  // is to be written. If this becomes zero before a MCU
  // the restart marker will be written.
  ULONG                 m_ulMCUsToGo;
  //
  // Boolean indicator whether the next entropy coded segment
  // up to the next restart marker or SOF/SOS is valid. If true,
  // continue parsing. Otherwise, replace just be zero/grey.
  bool                  m_bSegmentIsValid;
  //
  // Must scan for the DNL marker because the frame does not 
  // know our size.
  bool                  m_bScanForDNL;
  //
  // Set if parsing has come to an halt because DNL has been hit.
  bool                  m_bDNLFound;
  //
  // Flush the entropy coder, write the restart marker and
  // restart the MCU counter.
  void WriteRestartMarker(class ByteStream *io);
  //
  // Parse the restart marker or resync at the restart marker.
  void ParseRestartMarker(class ByteStream *io);
  //
  // Parse the DNL marker, update the frame height. If the
  // return is true, the DNL marker has been found.
  bool ParseDNLMarker(class ByteStream *io);
  //
protected:
  //
  // The scan this is part of.
  class Scan           *m_pScan;
  //
  // The frame we are part of.
  class Frame          *m_pFrame;
  //
  // The components in the scan - at most four.
  class Component      *m_pComponent[4];
   //
  // The number of components we have here.
  UBYTE                 m_ucCount;
  //
  // Create a new parser.
  EntropyParser(class Frame *frame,class Scan *scan);
  //
  // Return the number of fractional bits due to color
  // transformation.
  UBYTE FractionalColorBitsOf(void) const;
  //
  // Start writing a new MCU. Potentially, a restart marker
  // is emitted.
  void BeginWriteMCU(class ByteStream *io)
  {
    if (m_ulRestartInterval) {
      if (m_ulMCUsToGo == 0) {
        WriteRestartMarker(io);
      }
      m_ulMCUsToGo--;
    }
  }
  //
  // Start parsing/reading a MCU. Might expect a restart marker.
  // If so, Restart() (below) is called. Returns true if the next MCU
  // is valid, or false if the parser should replace the next MCU with
  // grey.
  bool BeginReadMCU(class ByteStream *io)
  {   
    if (m_bScanForDNL) {
      if (ParseDNLMarker(io))
        return false;
    }
    if (m_ulRestartInterval) {
      if (m_ulMCUsToGo == 0) {
        ParseRestartMarker(io);
      }
      m_ulMCUsToGo--;
    } 
    return m_bSegmentIsValid;
  }
  //
  // Return if the DNL marker has recently been found.
  bool hasFoundDNL(void) const
  {
    return m_bDNLFound;
  }
  //
  // Define the image size if it is not yet known here.
  // This is called whenever the DNL marker is parsed in.
  virtual void PostImageHeight(ULONG)
  { }
  //
  //
public:
  //
  virtual ~EntropyParser(void);
  //
  // Flush the remaining bits out to the stream on writing.
  virtual void Flush(bool final) = 0;
  //
  // Restart the statistics/prediction at the next restart marker on reading.
  virtual void Restart(void) = 0;
  //
  // Return the i'th component of the scan.
  class Component *ComponentOf(UBYTE i)
  {
    assert(i < 4);
    return m_pComponent[i];
  }
  //
  // Return the number of the components in the scan.
  UBYTE ComponentsInScan(void) const
  {
    return m_ucCount;
  }
  //
  // Parse the marker contents.
  virtual void StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl) = 0;
  //
  // Write the marker to the stream.
  virtual void StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl) = 0;
  //
  // Write the marker that indicates the frame type fitting to this scan.
  virtual void WriteFrameType(class ByteStream *io) = 0;
  //
  // Start making a measurement run to optimize the
  // huffman tables.
  virtual void StartMeasureScan(class BufferCtrl *) = 0;
  //
  // Start making an optimization run to adjust the coefficients.
  virtual void StartOptimizeScan(class BufferCtrl *) = 0;
  //
  // Start a MCU scan.
  virtual bool StartMCURow(void) = 0;  
  //
  // Parse a single MCU in this scan.
  virtual bool ParseMCU(void) = 0;
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void) = 0; 
  //
  // Make an R/D optimization for the given scan by potentially pushing
  // coefficients into other bins. This runs an optimization for a single
  // block and requires external control to run over the blocks.
  // component is the component, critical is the critical slope for
  // the R/D optimization of the functional J = \lambda D + R, i.e.
  // this is lambda.
  // Quant are the quantization parameters, i.e. deltas. These are eventually
  // preshifted by "preshift".
  // transformed are the dct-transformed but unquantized data. These are also pre-
  // shifted by "preshift".
  // quantized is the quantized data. These are potentially (and likely) adjusted.
  virtual void OptimizeBlock(LONG bx,LONG by,UBYTE component,double critical,
                             class DCT *dct,LONG quantized[64]) = 0;  
  //
  // Make an R/D optimization of the DC scan. This includes all DC blocks in
  // total, not just a single block. This is because the coefficients are not
  // coded independently.
  virtual void OptimizeDC(void) = 0;
};
///

///
#endif
