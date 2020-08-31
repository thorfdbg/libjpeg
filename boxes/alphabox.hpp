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

  
  
