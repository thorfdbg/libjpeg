/*
**
** This class represents one row of quantized data of coefficients, i.e. one
** row of 8x8 blocks.
**
** $Id: quantizedrow.hpp,v 1.10 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CODING_QUANTIZEDROW_HPP
#define CODING_QUANTIZEDROW_HPP

/// Includes
#include "tools/environment.hpp"
#include "coding/blockrow.hpp"
///

/// class QuantizedRow
// This class represents one row of quantized data of coefficients, i.e. one
// row of 8x8 blocks.
class QuantizedRow : public BlockRow<LONG> {
  // Nothing new in it.
public:
  QuantizedRow(class Environ *env)
    : BlockRow<LONG>(env)
  { }
  //
  ~QuantizedRow(void)
  { }
};
///

#endif
