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

    
