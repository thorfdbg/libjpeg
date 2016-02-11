/*
**
** Integer DCT operation plus scaled quantization.
**
** $Id: idct.hpp,v 1.19 2015/05/09 20:09:21 thor Exp $
**
*/

#ifndef DCT_IDCT_HPP
#define DCT_IDCT_HPP

/// Includes
#include "tools/environment.hpp"
#include "dct/dct.hpp"
#include "tools/traits.hpp"
///

/// Forwards
class Quantization;
struct ImageBitMap;
///

/// class IDCT
// This class implements the integer based DCT. The template parameter is the number of
// preshifted bits coming in from the color transformer.
template<int preshift,typename T,bool deadzone>
class IDCT : public DCT {
  //
  // Bit assignment
  enum {
    FIX_BITS          = 9,  // fractional bits for fixpoint in the calculation
    // 9 is the maximum for 16bpp input or 12 + 4(color bits)
    // This is because 12+4+3+3+9 = 31
    INTERMEDIATE_BITS = 0,  // fractional bits for representing the intermediate result
    // (none required)
    QUANTIZER_BITS    = 30  // bits for representing the quantizer
  };
  //
  // The (inverse) quantization tables, i.e. multipliers.
  LONG  m_plInvQuant[64];
  //
  // The quantizer tables.
  WORD  m_psQuant[64];
  //
  // Quantize a floating point number with a multiplier, round correctly.
  // Must remove FIX_BITS + INTER_BITS + 3
  static inline LONG Quantize(LONG n,LONG qnt,bool dc)
  {
    // Use the equi-quantizer if deadzone quantization is turned
    // off or we are quantizing the DC part.
    if (deadzone == false || dc) {
      return (n * QUAD(qnt) + (ULONG(-n) >> TypeTrait<LONG>::SignBit) + 
	      (QUAD(1) << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3 - 1)))
	>> (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3);
    } else {
      QUAD m = n >> TypeTrait<LONG>::SignBit;
      QUAD o = m << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3 - 2);
      return (n * QUAD(qnt) + ((~o) & m) +
	      (QUAD(3) << (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3 - 3)))
	>> (FIX_BITS + INTERMEDIATE_BITS + QUANTIZER_BITS + preshift + 3);
    }
  }
  //
  //
public:
  IDCT(class Environ *env);
  //
  ~IDCT(void);
  //
  // Use the quantization table defined here, scale them to the needs of the DCT and scale them
  // to the right size.
  virtual void DefineQuant(const UWORD *table);
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const LONG *source,LONG *target,LONG dcoffset);
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset);
};
///

///
#endif
