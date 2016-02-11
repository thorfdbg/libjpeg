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
