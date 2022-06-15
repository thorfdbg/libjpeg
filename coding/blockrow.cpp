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
** This class represents one row of quantized data of coefficients, i.e. one
** row of 8x8 blocks.
**
** $Id: blockrow.cpp,v 1.8 2022/06/14 06:18:30 thor Exp $
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

/// Explicit template instantiation
template class BlockRow<LONG>;
template class BlockRow<FLOAT>;
///
