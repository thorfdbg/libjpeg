/*
** This class represents multiple boxes that all contain specifications
** on the DCT process. These boxes are only used by part-8.
**
** $Id: dctbox.hpp,v 1.3 2014/09/30 08:33:14 thor Exp $
**
*/

#ifndef BOXES_DCTBOX_HPP
#define BOXES_DCTBOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// class DCTBox
// This class represents multiple boxes that all specify
// the DCT operations.
class DCTBox : public Box {
  //
  // The type of the DCT to use.
  UBYTE m_ucDCTType;
  //
  // Enable or disable noise shaping. Only if the DCT is disabled.
  bool  m_bNoiseShaping;
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box. Returns true in case the contents is
  // parsed and the stream can go away.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize);
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  // Returns whether the box content is already complete and the stream
  // can go away.
  virtual bool CreateBoxContent(class MemoryStream *target);
  //
public:
  enum {
    Base_Type        = MAKE_ID('L','D','C','T'), // base DCT
    Residual_Type    = MAKE_ID('R','D','C','T')  // residual DCT
  };
  //
  // Possible DCT types.
  enum DCTType {
    FDCT   = 0, // the fixpoint version
    IDCT   = 2, // the integer version
    Bypass = 3  // the DCT bypass
  };
  //
  // Create a DCT box. This also requires a type since there are
  // multiple boxes that all share the same syntax.
  DCTBox(class Environ *env,class Box *&boxlist,ULONG type)
    : Box(env,boxlist,type), m_ucDCTType(FDCT), m_bNoiseShaping(false)
  { 
  }
  //
  virtual ~DCTBox(void)
  { }
  //
  // Return the type of the DCT to be used.
  DCTType DCTTypeOf(void) const
  {
    return DCTType(m_ucDCTType);
  }
  //
  // Return a flag indicating whether noise shaping is enabled or disabled.
  bool isNoiseShapingEnabled(void) const
  {
    return m_bNoiseShaping;
  }
  //
  // Define the DCT operation
  void DefineDCT(DCTType t)
  {
    assert(t == FDCT || t == IDCT || t == Bypass);

    m_ucDCTType = t;
  }
  //
  // Enable or disable the noise shaping.
  void DefineNoiseShaping(bool onoff)
  {
    m_bNoiseShaping = onoff;
  }
};
///

///
#endif

    
