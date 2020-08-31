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
** This box defines the output process, namely whether data is casted 
** to float, whether a nonlinearity is applied, whether the data is
** clamped and several other options required as last processing
** step of the output conversion.
**
** It is a subbox of the merging specification box.
**
** $Id: outputconversionbox.hpp,v 1.6 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef BOXES_OUTPUTCONVERSIONBOX_HPP
#define BOXES_OUTPUTCONVERSIONBOX_HPP

/// Includes
#include "boxes/box.hpp"
#include "std/string.hpp"
///

/// Forwards
///

/// class OutputConversionBox
// This box defines the output process, namely whether data is casted 
// to float, whether a nonlinearity is applied, whether the data is
// clamped and several other options required as last processing
// step of the output conversion.
class OutputConversionBox : public Box {
  //
  // Number of additional output bits, to be added to eight (the usual JPEG output depth)
  // to get the bit depths of the output. Floating point is understood as 16 bit data
  // and requires an 8 here. The standard calls this value R_b.
  UBYTE m_ucExtraRangeBits;
  //
  // The lossless flag. If this is set, lossless coding is desired. The standard calls
  // this L_f.
  bool  m_bLossless;
  //
  // Enable casting to floating point. This flag enables the pseudo-exponential map.
  // The standard calls this flag O_c.
  bool  m_bCastToFloat;
  //
  // Enable clipping to range. This toggles between clipping and wrap-around arithmetic.
  // For lossless, wraparound is required. For IDR, clamping is required. This is called
  // Ce in the standard.
  bool  m_bEnableClamping;
  //
  // Enable an output lookup table. This is only required for some profiles of part 7
  // and is otherwise false. The standard calls this Ol
  bool  m_bEnableLookup;
  //
  // Lookup table indices for the output table, if any. This is enabled for Ol above.
  UBYTE m_ucOutputLookup[4];
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
  //
  enum {
    Type = MAKE_ID('O','C','O','N')
  };
  //
  OutputConversionBox(class Environ *env,class Box *&boxlist)
    : Box(env,boxlist,Type), m_ucExtraRangeBits(0),
      m_bLossless(false), m_bCastToFloat(false), 
      m_bEnableClamping(true), m_bEnableLookup(false)
  { 
    memset(m_ucOutputLookup,0,sizeof(m_ucOutputLookup));
  }
  //
  virtual ~OutputConversionBox(void)
  { }
  //
  //
  // Set the floating point coding flag.
  void DefineOutputConversion(bool convert)
  {
    m_bCastToFloat = convert;
  }
  //
  // Specify whether the output shall be clipped to range.
  void DefineClipping(bool clipping)
  {
    m_bEnableClamping = clipping;
  }
  //
  // Specify whether the process is lossy or not.
  void DefineLossless(bool lossless)
  {
    m_bLossless = lossless;
  }
  //
  // Define the additional number of bits in the spatial domain.
  // These are the number of bits on top of the bits in the legacy
  // domain, made available by other means. The total bit precision
  // of the image is r_b + 8.
  void DefineResidualBits(UBYTE residualbits)
  {
    assert(residualbits <= 8);
    
    m_ucExtraRangeBits = residualbits;
  }
  //
  // Check whether the encoded data uses output conversion from int to IEEE half float.
  // Requires clipping to be on.
  bool usesOutputConversion(void) const
  {
    return m_bCastToFloat;
  }
  //
  // Check whether the encoded data uses clipping. Required for int coding.
  bool usesClipping(void) const
  {
    return m_bEnableClamping;
  }
  //
  // Return the additional number of bits in the spatial domain.
  UBYTE ResidualBitsOf(void) const
  {
    return m_ucExtraRangeBits;
  }
  //
  // Return the state of the lossless flag.
  bool isLossless(void) const
  {
    return m_bLossless;
  }
  //
  // Defines the output conversion table index for component comp.
  void DefineOutputConversionTable(UBYTE comp,UBYTE table)
  {
    assert(!m_bLossless);
    assert(comp < 4);
    assert(table < 16);
    
    m_bEnableLookup        = true;
    m_ucOutputLookup[comp] = table;
  }
  //
  // Return the output conversion table responsible for component comp,
  // or MAX_UBYTE in case output conversion is disabled.
  UBYTE OutputConversionLookupOf(UBYTE comp) const
  {
    assert(comp < 4);

    if (m_bEnableLookup)
      return m_ucOutputLookup[comp];
    
    return MAX_UBYTE;
  }
};
///

///
#endif
