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
**
** This class represents one row of quantized data of coefficients, i.e. one
** row of 8x8 blocks.
**
** $Id: blockrow.cpp,v 1.7 2014/09/30 08:33:16 thor Exp $
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
