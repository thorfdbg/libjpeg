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
** This class represents one row of quantized data of coefficients, i.e. one
** row of 8x8 blocks.
**
** $Id: blockrow.hpp,v 1.4 2012-06-02 10:27:13 thor Exp $
**
*/

#ifndef CODING_BLOCKROW_HPP
#define CODING_BLOCKROW_HPP

/// Includes
#include "tools/environment.hpp"
///

/// class BlockRow
// This class represents one row of coefficients, i.e. one
// row of 8x8 blocks.
template<class T>
class BlockRow : public JKeeper {
  //
public:
  // the actual block. We always keep 32 bit data. In the future, this class
  // might be templated.
  struct Block  {
    T m_Data[64];
  };
  //
private:
  //
  // The block array itself.
  struct Block       *m_pBlocks;
  //
  // The extend in number of blocks.
  ULONG               m_ulWidth;
  //
  // The next row in a row stack.
  class QuantizedRow *m_pNext;
  //
public:
  BlockRow(class Environ *env);
  //
  ~BlockRow(void);
  //
  // Allocate a row of data, sufficient to hold the indicated number of cofficients. Note that
  // it is still up to the caller to include the subsampling factors.
  void AllocateRow(ULONG coefficients);
  //
  // Return the n'th block.
  struct Block *BlockAt(ULONG pos) const
  {
    assert(pos < m_ulWidth);
    return m_pBlocks + pos;
  }
  //
  // Return the next row.
  class QuantizedRow *NextOf(void) const
  {
    return m_pNext;
  }
  //
  // Return as lvalue.
  class QuantizedRow *&NextOf(void)
  {
    return m_pNext;
  }
  //
  // Width of this row in blocks.
  ULONG WidthOf(void) const
  {
    return m_ulWidth;
  }
  //
  // Tag a row on this row such that the passed argument is below the current row.
  void TagOn(class QuantizedRow *below)
  {
    m_pNext = below;
  }
};

///
#endif
