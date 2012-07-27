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
** $Id: blockrow.cpp,v 1.2 2012-06-02 10:27:13 thor Exp $
**
*/

/// Includes
#include "coding/blockrow.hpp"
#include "std/string.hpp"
///

/// BlockRow::BlockRow
template<class T>
BlockRow<T>::BlockRow(class Environ *env)
  : JKeeper(env), m_pBlocks(NULL), m_pNext(NULL)
{
}
///

/// BlockRow::~BlockRow
template<class T>
BlockRow<T>::~BlockRow(void)
{
  if (m_pBlocks) {
    m_pEnviron->FreeMem(m_pBlocks,sizeof(struct Block) * m_ulWidth);
  }
}
///

/// BlockRow::AllocateRow
// Allocate a row of data, sufficient to hold the indicated number of cofficients. Note that
// it is still up to the caller to include the subsampling factors.
template<class T>
void BlockRow<T>::AllocateRow(ULONG coefficients)
{
  if (m_pBlocks == NULL) {
    m_ulWidth = (coefficients + 7) >> 3;
    m_pBlocks = (struct Block *)m_pEnviron->AllocMem(sizeof(struct Block) * m_ulWidth);
    memset(m_pBlocks,0,sizeof(struct Block) * m_ulWidth);
  } else {
    assert(m_ulWidth == (coefficients + 7) >> 3);
  }
}
///

/// Explicit template instanciation
template class BlockRow<LONG>;
template class BlockRow<FLOAT>;
///
