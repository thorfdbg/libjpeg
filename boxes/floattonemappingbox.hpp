/*
** This box keeps an inverse tone mapping curve, as required for the
** R and L transformations. This is the floating point version, it is
** a box that contains floating point data indexed by integers.
**
** $Id: floattonemappingbox.hpp,v 1.4 2015/03/28 19:46:35 thor Exp $
**
*/

#ifndef BOXES_FLOATTONEMAPPINGBOX_HPP
#define BOXES_FLOATTONEMAPPINGBOX_HPP

/// Includes
#include "boxes/box.hpp"
#include "boxes/tonemapperbox.hpp"
///

/// class FloatToneMappingBox
class FloatToneMappingBox : public ToneMapperBox {
  //
  // The table itself, indexed by the decoded sample value.
  FLOAT *m_pfTable;
  // 
  // Inverse (encoding) tone mapping curve, if available. This is
  // indexed by floating point values preshifted by spacialfrac bits.
  LONG  *m_plInverseMapping;
  // 
  // Upscaled version of the table.
  FLOAT *m_pfInterpolated;
  //
  // Number of additional residual bits bits. Actually, this is only used to
  // test whether the table fits to the data (since it is always
  // unscaled). This is called R_d in the standard. Note that this is not
  // recorded in the stream.
  UBYTE  m_ucResidualBits;
  //
  // In case we have an interpolated table, here are its fractional bits.
  UBYTE  m_ucFractionalBits;
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize);
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  virtual bool CreateBoxContent(class MemoryStream *target);
  //
public:
  enum {
    Type = MAKE_ID('F','T','O','N')
  };
  //
  FloatToneMappingBox(class Environ *env,class Box *&boxlist)
    : ToneMapperBox(env,boxlist,Type), m_pfTable(NULL), m_plInverseMapping(NULL), 
      m_pfInterpolated(NULL), m_ucResidualBits(MAX_UBYTE), m_ucFractionalBits(0)
  { }
  //
  virtual ~FloatToneMappingBox(void);
  //
  //
  // Return the size of the table in entries.
  ULONG EntriesOf(void) const
  {
    return m_ulTableEntries;
  }
  //
  // Return the table itself.
  const FLOAT *TableOf(void) const
  {
    return m_pfTable;
  }
  //
  // Define the table from an external source.
  void DefineTable(UBYTE tableidx,const FLOAT *table,ULONG size,UBYTE residualbits);
  //
  // Check whether the given table is identical to the table stored here, and thus
  // no new index is required (to save rate). Returns true if the two are equal.
  bool CompareTable(const FLOAT *table,ULONG size,UBYTE residualbits) const;
  //
  // Return a table that maps inputs in the range 0..2^inputbits-1
  // to output bits in the range 0..2^outputbits-1.
  virtual const LONG *ScaledTableOf(UBYTE,UBYTE,UBYTE,UBYTE)
  {
    // There is currently no integer version of the floating point workflow.
    return NULL;
  }
  //
  // This is the floating point version of the above. It returns floating point sample
  // values instead of integer sample values.
  virtual const FLOAT *FloatTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract);
  //
  // This call is similar to the above, except that additional fractional bits are created for
  // the input, i.e. the table is upscaled. This is only required if we come in with more
  // fractional bits than documented, and this only happens for S (i.e. postscaling).
  const FLOAT *UpscaleTable(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract);
  //
  // Return the inverse of the table, where the first argument is the number
  // of bits in the DCT domain (the output bits) and the second argument is
  // the number of bits in the spatial (image) domain, i.e. the argument
  // order is identical to that of the backwards table generated above.
  virtual const LONG *InverseScaledTableOf(UBYTE dctbits,UBYTE spatialbits,UBYTE dctfract,UBYTE spatialfract);
};
///

///
#endif
