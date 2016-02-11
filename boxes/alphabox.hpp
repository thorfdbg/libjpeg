/*
** This box keeps all the information for opacity coding, the alpha mode
** and the matte color.
**
** $Id: alphabox.hpp,v 1.1 2015/03/11 16:02:38 thor Exp $
**
*/

#ifndef BOXES_ALPHABOX_HPP
#define BOXES_ALPHABOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// Forwards
class MemoryStream;
class ByteStream;
///

/// Class AlphaBox
class AlphaBox : public Box {
  //
public:
  enum Method {
    Opaque        = 0,   // no merging with alpha, treat as if alpha is absent
    Regular       = 1,   // regular alpha blending as convex combination of foreground and background
    Premultiplied = 2,   // premultiplied alpha, i.e. alpha multiplication is included in the foreground
    MatteRemoval  = 3    // foreground is merged with constant matte color
  };
  //
private:
  //
  // The Alpha mode recorded here.
  UBYTE m_ucAlphaMode;
  //
  // The matte color for red,green and blue.
  ULONG m_ulMatteRed;
  ULONG m_ulMatteGreen;
  ULONG m_ulMatteBlue;
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
  //
public:
  enum {
    Type = MAKE_ID('A','M','U','L')
  };
  //
  AlphaBox(class Environ *env,class Box *&boxlist)
    : Box(env,boxlist,Type), m_ucAlphaMode(Regular),
      m_ulMatteRed(0), m_ulMatteGreen(0), m_ulMatteBlue(0)
  { }
  //
  ~AlphaBox(void)
  { }
  //
  // Return the current alpha compositing method.
  Method CompositingMethodOf(void) const
  {
    return Method(m_ucAlphaMode);
  }
  //
  // Set the alpha mode.
  void SetCompositingMethod(Method m)
  {
    UBYTE method = m;
    assert(method <= MatteRemoval);

    m_ucAlphaMode = method;
  }
  //
  // Return the matte color for component n, n = 0,1,2
  ULONG MatteColorOf(UBYTE comp) const
  {
    assert(comp <= 2);

    switch(comp) {
    case 0:
      return m_ulMatteRed;
    case 1:
      return m_ulMatteGreen;
    case 2:
      return m_ulMatteBlue;
    default:
      return 0;
    }
  }
  //
  // Set the matte color of component n, n = 0,1,2
  void SetMatteColor(UBYTE comp,ULONG value)
  {
    assert(comp <= 2);

    switch(comp) {
    case 0:
      m_ulMatteRed = value;
      break;
    case 1:
      m_ulMatteGreen = value;
      break;
    case 2:
      m_ulMatteBlue = value;
      break;
    }
  }
};
///

///
#endif

  
  
