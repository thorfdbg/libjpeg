/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select beween these two options.

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
** This class contains and maintains the huffman code parsers.
**
** $Id: huffmantable.hpp,v 1.17 2017/06/06 10:51:41 thor Exp $
**
*/

#ifndef MARKER_HUFFMANTABLE_HPP
#define MARKER_HUFFMANTABLE_HPP

/// Includes
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Forwards
class ByteStream;
class DecderTemplate;
///

/// HuffmanTable
class HuffmanTable : public JKeeper {
  //
  // Table specification. 4 DC and 4 AC tables.
  class HuffmanTemplate *m_pCoder[8];
  //
public:
  HuffmanTable(class Environ *env);
  //
  ~HuffmanTable(void);
  //
  // Check whether the tables are empty or not
  // In such a case, do not write the tables.
  bool isEmpty(void) const;
  //
  // Write the marker contents to a DHT marker.
  void WriteMarker(class ByteStream *io);
  //
  // Parse the marker contents of a DHT marker.
  void ParseMarker(class ByteStream *io);
  //
  // Get the template for the indicated DC table or NULL if it doesn't exist.
  class HuffmanTemplate *DCTemplateOf(UBYTE idx,ScanType type,
                                      UBYTE depth,UBYTE hidden,UBYTE scan);
  //
  // Get the template for the indicated AC table or NULL if it doesn't exist.
  class HuffmanTemplate *ACTemplateOf(UBYTE idx,ScanType type,
                                      UBYTE depth,UBYTE hidden,UBYTE scan);
  //
  // Adjust all coders in here to the statistics collected before, i.e.
  // find optimal codes.
  void AdjustToStatistics(void);
};
///

///
#endif
