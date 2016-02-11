/*
**
** This class represents one row of quantized data of coefficients, i.e. one
** row of 8x8 blocks.
**
** $Id: blockrow.hpp,v 1.9 2014/09/30 08:33:16 thor Exp $
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
