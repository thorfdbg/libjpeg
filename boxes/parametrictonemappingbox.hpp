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
** This box keeps an inverse tone mapping curve, however, this box here
** is defined by parameters, not by actual providing the table explicitly
** as a LUT.
**
** $Id: parametrictonemappingbox.hpp,v 1.25 2018/07/27 06:56:42 thor Exp $
**
*/

#ifndef BOXES_PARAMETRICTONEMAPPINGBOX_HPP
#define BOXES_PARAMETRICTONEMAPPINGBOX_HPP

/// Includes
#include "box.hpp"
#include "tonemapperbox.hpp"
///

/// class ParametricToneMappingBox
// This box keeps an inverse tone mapping curve, however, this box here
// is defined by parameters, not by actual providing the table explicitly
// as a LUT.
class ParametricToneMappingBox : public ToneMapperBox {
  //
  // Implementations of the table for various bit-depths.
  struct TableImpl : public JObject {
    // 
    // Pointer to the next implementation.
    struct TableImpl *m_pNext;
    //
    // The integer scaled version of the table.
    LONG  *m_plTable;
    //
    // The inverse integer scaled version of the table.
    LONG  *m_plInverseTable;
    //
    // Floating point version of the table if required.
    FLOAT *m_pfTable;
    //
    // Size of the table in entries.
    ULONG  m_ulTableEntries;
    //
    // Size of the inverse tables in entries.
    ULONG  m_ulInverseTableEntries;
    //
    // For consistency check: The data with which the table has been created.
    UBYTE  m_ucInputBits;
    UBYTE  m_ucOutputBits;
    UBYTE  m_ucInputFracts;
    UBYTE  m_ucOutputFracts;
    ULONG  m_ulInputOffset; // additional offset to be added to the input before applying the LUT
    UBYTE  m_ucTableBits;   // 1<<m_cuTableBits gives the size of the table.
    //
    TableImpl(UBYTE inbits,UBYTE outbits,UBYTE infract,UBYTE outfract,
              ULONG offset,UBYTE tablebits)
      : m_pNext(NULL), m_plTable(NULL), m_plInverseTable(NULL), m_pfTable(NULL), 
        m_ulTableEntries(0), m_ulInverseTableEntries(0),
        m_ucInputBits(inbits), m_ucOutputBits(outbits), 
        m_ucInputFracts(infract), m_ucOutputFracts(outfract),
        m_ulInputOffset(offset), m_ucTableBits(tablebits)
    { } 
    //
    // For non-extended tables, this is the simpler constructor.
    TableImpl(UBYTE inbits,UBYTE outbits,UBYTE infract,UBYTE outfract)
      : m_pNext(NULL), m_plTable(NULL), m_plInverseTable(NULL), m_pfTable(NULL), 
        m_ulTableEntries(0), m_ulInverseTableEntries(0),
        m_ucInputBits(inbits), m_ucOutputBits(outbits), 
        m_ucInputFracts(infract), m_ucOutputFracts(outfract),
        m_ulInputOffset(0), m_ucTableBits(outbits)
    { }
  }        *m_pImpls;
  //
  // The curve type.
public:
  enum CurveType {
    Zero        = 0,   // the zero mapping, disables this curve
    Constant    = 1,   // output is constant
    Identity    = 2,   // mapping is the identity
    Gamma       = 4,   // inverse gamma mapping with toe threshold
    Linear      = 5,   // linear ramp
    Exponential = 6,
    Logarithmic = 7,
    GammaOffset = 8
  };
  //
private:
  CurveType m_Type;
  //
  // The value of the rouding parameter, 0 or 1.
  UBYTE     m_ucE;
  //
  // The parameters of the curve.
  FLOAT     m_fP1;
  FLOAT     m_fP2;
  FLOAT     m_fP3;
  FLOAT     m_fP4;
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
  // Compute the output table value for the input value
  DOUBLE TableValue(DOUBLE v) const;
  //
  // Return the table value of the inverse table. If it exists.
  DOUBLE InverseTableValue(DOUBLE v) const;
  //
  // Find the table for the given number of dct, spatial and fractional bits and return
  // the table implementation, or NULL in case it does not (yet) exist.
  struct TableImpl *FindImpl(UBYTE dctbits,UBYTE spatialbits,UBYTE dctfract,UBYTE spatialfract)
  {
    return FindImpl(dctbits,spatialbits,dctfract,spatialfract,0,spatialbits);
  }
  //
  // The exended version that also takes an offset and the table size.
  struct TableImpl *FindImpl(UBYTE dctbits,UBYTE spatialbits,UBYTE dctfract,UBYTE spatialfract,
                             ULONG offset,UBYTE tablebits) const;
  //
public:
  enum {
    Type = MAKE_ID('C','U','R','V')
  };
  //
  ParametricToneMappingBox(class Environ *env,class Box *&boxlist)
    : ToneMapperBox(env,boxlist,Type), m_pImpls(NULL)
  { }
  //
  virtual ~ParametricToneMappingBox(void);
  //
  // Check whether this box fits the selected parameters. Returns true in case of a match.
  bool CompareCurve(CurveType curve,UBYTE rounding_mode,
                    FLOAT p1 = 0.0,FLOAT p2 = 0.0,FLOAT p3 = 0.0,FLOAT p4 = 0.0) const;
  //
  // Same as above, but this is the scaled version with
  // outputs in the range 0..2^outputbits-1.
  virtual const LONG *ScaledTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract);
  //
  // This is the floating point version of the above. It returns floating point sample
  // values instead of integer sample values.
  virtual const FLOAT *FloatTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE infract,UBYTE outfract);
  //
  // Return the inverse of the table, where the first argument is the number
  // of bits in the DCT domain (the output bits) and the second argument is
  // the number of bits in the spatial (image) domain, i.e. the argument
  // order is identical to that of the backwards table generated above.
  virtual const LONG *InverseScaledTableOf(UBYTE dctbits,UBYTE spatialbits,UBYTE infract,UBYTE outfract)
  {
    return ExtendedInverseScaledTableOf(dctbits,spatialbits,infract,outfract,0,spatialbits);
  }
  //
  // This works like the above, though builds a potentially larger table to cover a larger input range.
  // For that, it takes one offset - the offset added to the samples before going into the LUT -
  // and the true number of bits to allocate for the table. For regular tables, the offset would be
  // zero and the extended bits would be equal to the spatialbits.
  // The first entry the table is thus able to cover is at -inputoffset, and there are in total
  // 1<<truebits entries in the table.
  const LONG *ExtendedInverseScaledTableOf(UBYTE dctbits,UBYTE spatialbits,UBYTE infract,UBYTE outfract,
                                           ULONG inputoffset,UBYTE truebits);
  //
  // Compute the table from the parameters.
  // Define the table from an external source.
  void DefineTable(UBYTE tableidx,CurveType type,UBYTE rounding_mode,
                   FLOAT p1 = 0.0,FLOAT p2 = 0.0,FLOAT p3 = 0.0,FLOAT p4 = 0.0);
  //
  // Return the curve type of this box.
  CurveType CurveTypeOf(void) const
  {
    return m_Type;
  }
  //
  // Retrieve the curve parameter P1.
  FLOAT P1Of(void) const
  {
    assert(m_Type != Zero && m_Type != Identity);
    return m_fP1;
  }
  //
  // Retrieve the curve parameter P2.
  FLOAT P2Of(void) const
  {
    assert(m_Type == Linear || m_Type == Gamma || m_Type == Exponential);
    return m_fP2;
  }
  //
  // Retrieve the curve parameter P3.
  FLOAT P3Of(void) const
  {
    assert(m_Type == Gamma);
    return m_fP3;
  }
  //
  // Retrieve the curve parameter P4.
  FLOAT P4Of(void) const
  {
    assert(false);
    return m_fP4;
  }
  //
  // Apply the curve directly to a value, perform input and output scaling as described in 
  // Annex C of ISO/IEC 18477-3:2015. The input parameters are the value to apply the curve to,
  // the input scale (2^Rw-1 in terms of the standard, *not* Rw itself), the number of
  // fractional input bits (Re in standard speech)
  // the output scale (2^Rt-1 as denoted by the symbols of the standard, not Rt itself)
  // and the output fractional bits (Rf in standard speech).
  // If inrange (or outrange) is one, the curve remains unscaled (on the input or output)
  // and only the fractional bits are considered for scaling.
  DOUBLE ApplyCurve(DOUBLE x,LONG inrange,UBYTE infract,LONG outrange,UBYTE outfract);
  //
  // Apply the inverse of a curve as required by encoding. The parameters are as above,
  // namely the input scale (the scale of the first argument), the input fractional bits,
  // the output scale, and the output fractional bits.
  // If inrange (or outrange) is one, the curve remains unscaled (on the input or output)
  // and only the fractional bits are considered for scaling.
  DOUBLE ApplyInverseCurve(DOUBLE x,LONG inrange,UBYTE infract,LONG outrange,UBYTE outfract);
  //
  // For encoding, we have the special case that x can be a quotient p/q, and I want to avoid
  // the potential danger of a division by zero. Thus, InverseOfQuotient has the same range
  // and domain as ApplyInverseCurve, though the first two arguments define the numerator and
  // denominator of a fraction, where denomiator = 0 results in a saturated output. The input
  // scale is in this case not given as the argument is expected unscaled (no range), only
  // the output range is given.
  DOUBLE InverseOfQuotient(DOUBLE p,DOUBLE q,LONG outrange,UBYTE outfract);
};
///

///
#endif
