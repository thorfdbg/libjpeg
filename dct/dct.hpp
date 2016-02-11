/*
**
** Generic DCT transformation plus quantization class.
** All DCT implementations should derive from this.
**
*/

#ifndef DCT_DCT_HPP
#define DCT_DCT_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class Quantization;
struct ImageBitMap;
///

/// class DCT
// This class is the base class for all DCT implementations.
class DCT : public JKeeper {
  //
  //
public: 
  //
  // The scan order within a block.
  static const int ScanOrder[64];
  //
  DCT(class Environ *env)
    : JKeeper(env)
  { }
  //
  virtual ~DCT(void)
  { }
  //
  // Use the quantization table defined here, scale them to the needs of the DCT and scale them
  // to the right size.
  virtual void DefineQuant(const UWORD *table) = 0;
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const LONG *source,LONG *target,LONG dcoffset) = 0;
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset) = 0; 
};
///

///
#endif
