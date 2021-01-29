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
** $Id: parametrictonemappingbox.cpp,v 1.47 2018/07/27 06:56:42 thor Exp $
**
*/

/// Includes
#include "boxes/parametrictonemappingbox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "io/decoderstream.hpp"
#include "tools/numerics.hpp"
#include "std/math.hpp"
///

/// ParametricToneMappingBox::~ParametricToneMappingBox
ParametricToneMappingBox::~ParametricToneMappingBox(void)
{
  struct TableImpl *impl;

  while ((impl = m_pImpls)) {
    m_pImpls = impl->m_pNext;
    
    if (impl->m_plTable)
      m_pEnviron->FreeMem(impl->m_plTable,impl->m_ulTableEntries * sizeof(LONG)); 

    if (impl->m_pfTable)
      m_pEnviron->FreeMem(impl->m_pfTable,impl->m_ulTableEntries * sizeof(FLOAT));

    if (impl->m_plInverseTable)
      m_pEnviron->FreeMem(impl->m_plInverseTable,impl->m_ulInverseTableEntries * sizeof(LONG));

    delete impl;
  }
}
///

/// ParametricToneMappingBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box.
bool ParametricToneMappingBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  UBYTE m;
  ULONG p;

  if (boxsize != 2 + 4 * 4)
    JPG_THROW(MALFORMED_STREAM,"ParametricToneMappingBox::ParseBoxContent",
              "Malformed JPEG file, CURV box size is invalid");

  
  m              = stream->Get();
  m_ucTableIndex = m >> 4;

  switch(m & 0x0f) {
  case Zero:
  case Constant:
  case Identity:
  case Gamma:
  case Linear:
  case Exponential:
  case Logarithmic:
  case GammaOffset:
    m_Type = (CurveType)(m & 0x0f);
    break;
  default:
    JPG_THROW(MALFORMED_STREAM,"ParametricToneMappingBox::ParseBoxContent",
              "Malformed JPEG file, curve type in CURV box is invalid");
  }

  m     = stream->Get();
  // The lower nibble must be zero.
  if (m & 0x0f)
    JPG_THROW(MALFORMED_STREAM,"ParametricToneMappingBox::ParseBoxContent",
              "Malformed JPEG file, the r parameter of the CURV box must be zero");
  //
  // The upper nibble decodes to the rounding parameter.
  switch(m >> 4) {
  case 0:
  case 1:
    m_ucE = m >> 4;
    break;
  default:
    JPG_THROW(MALFORMED_STREAM,"ParametricToneMappingBox::ParseBoxcontent",
              "Malformed JPEG file, rounding parameter e must be zero or one");
  }
  //
  // Decode the parameters.
  p     = stream->GetWord() << 16;
  p    |= stream->GetWord();
  m_fP1 = IEEEDecode(p);

  p     = stream->GetWord() << 16;
  p    |= stream->GetWord();
  m_fP2 = IEEEDecode(p);

  p     = stream->GetWord() << 16;
  p    |= stream->GetWord();
  m_fP3 = IEEEDecode(p);

  p     = stream->GetWord() << 16;
  p    |= stream->GetWord();
  m_fP4 = IEEEDecode(p);

  return true; // is parsed off
}
///

/// ParametricToneMappingBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
bool ParametricToneMappingBox::CreateBoxContent(class MemoryStream *target)
{
  ULONG p;

  target->Put((m_ucTableIndex << 4) | m_Type);
  target->Put((m_ucE << 4));

  p = IEEEEncode(m_fP1);
  target->PutWord(p >> 16);
  target->PutWord(p);

  p = IEEEEncode(m_fP2);
  target->PutWord(p >> 16);
  target->PutWord(p);

  p = IEEEEncode(m_fP3);
  target->PutWord(p >> 16);
  target->PutWord(p);

  p = IEEEEncode(m_fP4);
  target->PutWord(p >> 16);
  target->PutWord(p);

  return true;
}
///

/// ParametricToneMappingBox::DefineTable
// Compute the table from the parameters.
// Define the table from an external source.
void ParametricToneMappingBox::DefineTable(UBYTE tableidx,CurveType type,UBYTE e,FLOAT p1,FLOAT p2,FLOAT p3,FLOAT p4)
{
  m_ucTableIndex = tableidx;
  m_Type         = type;
  m_ucE          = e;
  m_fP1          = p1;
  m_fP2          = p2;
  m_fP3          = p3;
  m_fP4          = p4;
}
///

/// ParametricToneMappingBox::TableValue
// Compute the output table value for the input value
DOUBLE ParametricToneMappingBox::TableValue(DOUBLE v) const
{
  DOUBLE w = 0.0;

  switch(m_Type) {
  case Zero:
    w = 0.0;
    break;
  case Constant:
    w = 1.0;
    break;
  case Identity:
    w = v;
    break;
  case Gamma:
    if (v >= m_fP1) {
      w = pow(double(v     + m_fP3) / (1.0 + m_fP3),double(m_fP2));
    } else {
      w = pow(double(m_fP1 + m_fP3) / (1.0 + m_fP3),double(m_fP2)) * v / m_fP1;
    }
    break;
  case Linear:
    if (m_fP2 >= m_fP1) {
      w = v * (m_fP2 - m_fP1) + m_fP1;
    } else {
      JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::TableValue",
                "Parametric tone mapping definition is invalid, linear slope must be non-negative.");
    }
    break;
  case Exponential:
    if (m_fP2 > m_fP1) {
      w = m_fP3 * exp(v * (m_fP2 - m_fP1) + m_fP1) + m_fP4;
    } else {
      JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::TableValue",
                "Parametric tone mapping definition is invalid, exponent slope must be strictly positive.");
    }
    break;
  case Logarithmic:
    if (m_fP1 > 0.0) {
      if (v > 0.0 || (m_fP3 > 0.0 && v >= 0.0)) {
        w = log(pow(m_fP1 * v,double(m_fP2)) + m_fP3)+m_fP4;
      } else {
        w = -HUGE_VAL;
      }
      assert(!isnan(w));
    } else {
      if (v > 0.0 || (m_fP3 > 0.0 && v >= 0.0)) {
        w = -log(pow(-m_fP1 * v,double(m_fP2)) + m_fP3)+m_fP4;
      } else {
        w = HUGE_VAL;
      }
      assert(!isnan(w));
    }
    break;
  case GammaOffset:
    const double inscale = 1.0; //256.0 / 255.0 * 65535.0 / 65536.0;
    if (v > 0.0) {
      w = (m_fP2 - m_fP1) * pow(v * inscale,double(m_fP3)) + m_fP1;
    } else {
      w = m_fP1;
    }
    assert(!isnan(w));
    break;
  }
  
  /*
  ** NO, no clamping.
  if (w < 0.0)
    w = 0.0;
  if (w > 1.0)
    w = 1.0;
  */

  return w;
}
///

/// ParametricToneMappingBox::InverseTableValue
// Return the table value of the inverse table. If it exists.
DOUBLE ParametricToneMappingBox::InverseTableValue(DOUBLE v) const
{
  DOUBLE w = 0.0;

  switch(m_Type) {
  case Zero:
    JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::InverseTableValue",
              "Tried to build the inverse of the zero tone mapping marker - inverse does not exist");
    break;
  case Constant:  
    JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::InverseTableValue",
              "Tried to build the inverse of the constant tone mapping marker - inverse does not exist");
    break;
  case Identity:
    w = v;
    break;
  case Gamma:
    if (v > pow((m_fP1 + m_fP3) / (1.0 + m_fP3),double(m_fP2))) {
      w = pow(v,1.0 / m_fP2) * (1.0 + m_fP3) - m_fP3;
    } else {
      w = v * m_fP1 / (pow(double(m_fP1 + m_fP3) / (1.0 + m_fP3),double(m_fP2)));
    }
    break;
  case Linear:
    if (m_fP2 > m_fP1) {
      w = (v - m_fP1) / (DOUBLE(m_fP2) - m_fP1);
    } else {
      JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::InverseTableValue",
                "Tried to build the inverse of a constant linear tone mapping - inverse does not exist");
    }
    break;
  case Exponential:
    if (m_fP2 > m_fP1) {
      v = (v - m_fP4) / m_fP3;
      if (v > 0.0) {
        w = (log(v) - m_fP1) / (m_fP2 - m_fP1);
      } else if (v == 0.0) {
        return -HUGE_VAL;
      } else {
        JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::InverseTableValue",
                  "The specified exponential mapping is not invertible on the source domain.");
      }
    } else {
      JPG_THROW(INVALID_PARAMETER,"ParametricToneMappingBox::InverseTableValue",
                "Tried to build the inverse of a constant exponential tone mapping - inverse does not exist");
    }
    break;
  case Logarithmic:
    if (m_fP1 > 0.0) {
      w = pow(exp(v - m_fP4) - m_fP3,1.0 / m_fP2) / m_fP1;
    } else {
      w = -pow(exp(m_fP4 - v) - m_fP3,1.0 / m_fP2) / m_fP1;
    }
    assert(!isnan(w));
    break;
  case GammaOffset:
    const double outscale = 1.0; // / (256.0 / 255.0 * 65535.0 / 65536.0);
    if (v > m_fP1) {
      w = outscale * (pow((v - m_fP1) / (m_fP2 - m_fP1),1.0 / m_fP3));
    } else {
      w = 0.0;
    }
    assert(!isnan(w));
    break;
  }
  
  /*
  ** No, no clamping, done outside after scaling.
  if (w < 0.0)
    w = 0.0;
  if (w > 1.0)
    w = 1.0;
  */

  return w;
}
///

/// ParametricToneMappingBox::FindImpl
// Find the table for the given number of dct, spatial and fractional bits and return
// the table implementation, or NULL in case it does not (yet) exist.
struct ParametricToneMappingBox::TableImpl *ParametricToneMappingBox::FindImpl(UBYTE dctbits,
                                                                               UBYTE spatialbits,
                                                                               UBYTE dctfract,
                                                                               UBYTE spatialfract,
                                                                               ULONG offset,
                                                                               UBYTE tablebits) const
{
  struct TableImpl *tab = m_pImpls;

  while(tab) {
    if (tab->m_ucInputBits      == dctbits && 
        tab->m_ucOutputBits     == spatialbits && 
        tab->m_ucInputFracts    == dctfract &&
        tab->m_ucOutputFracts   == spatialfract &&
        tab->m_ulInputOffset    == offset &&
        tab->m_ucTableBits      == tablebits)
      return tab;
    tab = tab->m_pNext;
  }

  return NULL;
}
///


/// ParametricToneMappingBox::ScaleTableOf
// Same as above, but this is the scaled version with
// outputs in the range 0..2^outputbits-1.
const LONG *ParametricToneMappingBox::ScaledTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE inputfract,UBYTE outputfract)
{ 
  struct TableImpl *impl = FindImpl(inputbits,outputbits,inputfract,outputfract);

  if (impl == NULL) {
    impl          = new(m_pEnviron) struct TableImpl(inputbits,outputbits,inputfract,outputfract);
    impl->m_pNext = m_pImpls;
    m_pImpls      = impl;
  }

  if (impl->m_plTable == NULL) {
    ULONG i    = 0;
    ULONG max  = 1UL << (inputbits  + inputfract);
    //LONG omax  = 1UL << (outputbits + outputfract);
    double inscale  = (inputbits  > 1)?(1.0 / (((1UL <<  inputbits) - m_ucE) <<  inputfract)):(1.0 / (1 <<  inputfract));
    double outscale = (outputbits > 1)?(1.0 * (((1UL << outputbits) - m_ucE) << outputfract)):(1.0 * (1 << outputfract));
    
    assert(inputbits  <= 16);
    assert(outputbits <= 16);
    assert(impl->m_ulTableEntries == 0 || impl->m_ulTableEntries == max);
    
    impl->m_ulTableEntries = max;
    impl->m_plTable        = (LONG *)m_pEnviron->AllocMem(max * sizeof(LONG));

    do {
      LONG out = LONG(floor(outscale * TableValue(i * inscale)+0.5));
      /*
      ** The standard does not say anything about clamping, so
      ** don't clamp. Previous versions (1.40 and above) did clamp,
      ** but profile 2, R2-tables require full range.
      if (out < 0)     out = 0;
      if (out >= omax) out = omax - 1;
      **
      */
      impl->m_plTable[i]   = out;
    } while(++i < max);
  } 
  
  return impl->m_plTable;
}
///

/// ParametricToneMappingBox::FloatTableOf
// This returns the floating point scaled version of the table.
const FLOAT *ParametricToneMappingBox::FloatTableOf(UBYTE inputbits,UBYTE outputbits,UBYTE inputfract,UBYTE outputfract)
{ 
  struct TableImpl *impl = FindImpl(inputbits,outputbits,inputfract,outputfract);

  if (impl == NULL) {
    impl          = new(m_pEnviron) struct TableImpl(inputbits,outputbits,inputfract,outputfract);
    impl->m_pNext = m_pImpls;
    m_pImpls      = impl;
  }

  if (impl->m_pfTable == NULL) {
    ULONG i    = 0;
    ULONG max  = 1UL << (inputbits  + inputfract);
    //LONG omax  = 1UL << (outputbits + outputfract);
    //FLOAT fmax = FLOAT(omax - 1);
    double inscale  = (inputbits  > 1)?(1.0 / (((1UL <<  inputbits) - m_ucE) <<  inputfract)):(1.0 / (1 <<  inputfract));
    double outscale = (outputbits > 1)?(1.0 * (((1UL << outputbits) - m_ucE) << outputfract)):(1.0 * (1 << outputfract));
    
    assert(inputbits  <= 16);
    assert(outputbits <= 16);
    assert(impl->m_ulTableEntries == 0 || impl->m_ulTableEntries == max);
    
    impl->m_ulTableEntries = max;
    impl->m_pfTable        = (FLOAT *)m_pEnviron->AllocMem(max * sizeof(FLOAT));

    do {
      FLOAT out = outscale * TableValue(i * inscale);
      /*
      ** The specs do not say a word about clamping, so we don't.
      if (out < 0.0)   out = 0.0;
      if (out > fmax)  out = fmax;
      */
      impl->m_pfTable[i]   = out;
    } while(++i < max);
  } 
  
  return impl->m_pfTable;
}
///

/// ParametricToneMappingBox::ExtendedInverseScaledTableOf
// Return the inverse of the table, where the first argument is the number
// of bits in the DCT domain (the output bits) and the second argument is
// the number of bits in the spatial (image) domain, i.e. the argument
// order is identical to that of the backwards table generated above.
// This takes, in addition, an input offset by which input samples are
// shifted before the mapping is applied, and the size of the table in
// bits.
const LONG *ParametricToneMappingBox::ExtendedInverseScaledTableOf(UBYTE dctbits,UBYTE spatialbits,
                                                                   UBYTE dctfract,UBYTE spatialfract,
                                                                   ULONG offset,UBYTE tablebits)
{
  struct TableImpl *impl = FindImpl(dctbits,spatialbits,dctfract,spatialfract,offset,tablebits);
  
  if (impl == NULL) {
    impl          = new(m_pEnviron) struct TableImpl(dctbits,spatialbits,dctfract,spatialfract,offset,tablebits);
    impl->m_pNext = m_pImpls;
    m_pImpls      = impl;
  }
  
  if (impl->m_plInverseTable == NULL) {
    LONG i     = 0; 
    LONG max   = 1UL << (tablebits + spatialfract);
    LONG omax  = 1UL << (dctbits   + dctfract);
    double inscale  = (spatialbits > 1)?(1.0 / (((1UL << spatialbits) - m_ucE) << spatialfract)):(1.0 / (1 << spatialfract));
    double outscale = (dctbits     > 1)?(1.0 * (((1UL << dctbits    ) - m_ucE) << dctfract    )):(1.0 * (1 << dctfract));

    assert(dctbits <= 16);
    assert(spatialbits <= 16);

    impl->m_ulInverseTableEntries = max;
    impl->m_plInverseTable        = (LONG *)m_pEnviron->AllocMem(max * sizeof(LONG));

    do {
      LONG out = LONG(floor(outscale * InverseTableValue((i - LONG(offset)) * inscale)+0.5));
      if (out < 0)     out = 0;
      if (out >= omax) out = omax - 1;
      impl->m_plInverseTable[i] = out;
    } while(++i < max);
  }

  return impl->m_plInverseTable;
}
///

/// ParametricToneMappingBox::ApplyCurve
// Apply the curve directly to a value, perform input and output scaling as described in 
// Annex C of ISO/IEC 18477-3:2015. The input parameters are the value to apply the curve to,
// the input scale (2^Rw-1 in terms of the standard, *not* Rw itself), the number of
// fractional input bits (Re in standard speech)
// the output scale (2^Rt-1 as denoted by the symbols of the standard, not Rt itself)
// and the output fractional bits (Rf in standard speech).
// If inrange (or outrange) is one, the curve remains unscaled (on the input or output)
// and only the fractional bits are considered for scaling.
DOUBLE ParametricToneMappingBox::ApplyCurve(DOUBLE x,LONG inrange,UBYTE infract,LONG outrange,UBYTE outfract)
{
  DOUBLE y;
  //
  // This follows Annex C of ISO/IEC 18477-3:2015. First, apply input clamping if the input
  // has a limited range. This is indicated by inrange > 1.
  if (inrange > 1) {
    LONG max = ((inrange + 1) << infract) - 1;
    // Yes, perform clamping.
    if (x < 0.0) x = 0.0;
    if (x > max) x = max;
    //
    // Scale to range.
    x  /= (inrange + 1 - m_ucE) << infract;
  } else {
    // Otherwise, just scale out the fractional bits.
    x  /= 1 << infract;
  }
  // Apply now the transformation.
  y = TableValue(x);
  //
  // Perform output scaling? There is no clamping here.
  if (outrange > 1) {
    y  *= (outrange + 1 - m_ucE) << outfract;
  } else {
    y  *= 1 << outfract;
  }

  return y;
}
///

/// ParametricToneMappingBox::ApplyInverseCurve
// Apply the inverse of a curve as required by encoding. The parameters are as above,
// namely the input scale (the scale of the first argument), the input fractional bits,
// the output scale, and the output fractional bits.
// If inrange (or outrange) is one, the curve remains unscaled (on the input or output)
// and only the fractional bits are considered for scaling.
DOUBLE ParametricToneMappingBox::ApplyInverseCurve(DOUBLE x,LONG inrange,UBYTE infract,LONG outrange,UBYTE outfract)
{
  DOUBLE y;
  //
  // Compute the inverse of a curve. This is not covered by the specs, but for symmetry, perform clamping
  // at the output if required y outrange > 1.
  if (inrange > 1) {
    // Scale to range.
    x /= (inrange + 1 - m_ucE) << infract;
  } else {
    // Otherwise, just scale out the fractional bits.
    x /= 1 << infract;
  }
  // Apply now the transformation.
  y = InverseTableValue(x);
  //
  // Apply output scaling.
  if (outrange > 1) {
    LONG max = ((outrange + 1) << outfract) - 1;
    y *= (outrange + 1 - m_ucE) << outfract;
    //
    // Perform clamping on the output.
    if (y < 0.0) y = 0.0;
    if (y > max) y = max;
  } else {
    y *= 1 << outfract;
  }

  return y;
}
///

/// ParametricToneMappingBox::InverseOfQuotient
// For encoding, we have the special case that x can be a quotient p/q, and I want to avoid
// the potential danger of a division by zero. Thus, InverseOfQuotient has the same range
// and domain as ApplyInverseCurve, though the first two arguments define the numerator and
// denominator of a fraction, where denomiator = 0 results in a saturated output. The input
// scale is in this case not given as the argument is expected unscaled (no range), only
// the output range is given.
DOUBLE ParametricToneMappingBox::InverseOfQuotient(DOUBLE p,DOUBLE q,LONG outrange,UBYTE outfract)
{
  LONG max = ((outrange + 1) << outfract) - 1;
  DOUBLE y;
  //
  assert(outrange > 1); // No unscaled output accepted...
  // The only special case is here if q = 0 where the maximum output is generated.
  if (q <= 0.0) {
    return max;
  }
  //
  // Apply now the transformation.
  y  = InverseTableValue(p / q);
  //
  // Apply output scaling.
  y *= (outrange + 1 - m_ucE) << outfract;
  //
  // Perform clamping on the output.
  if (y < 0.0) y = 0.0;
  if (y > max) y = max;
  
  return y;
}
///

/// ParametricToneMappingBox::CompareCurve
// Check whether this box fits the selected parameters. Returns true in case of a match.
bool ParametricToneMappingBox::CompareCurve(CurveType curve,UBYTE e,FLOAT p1,FLOAT p2,FLOAT p3,FLOAT p4) const
{
  if (m_ucE == e && m_Type == curve) {
    switch(m_Type) {
    case Zero:
    case Constant:
    case Identity:
      return true;
    case Gamma:
    case GammaOffset:
      return (p1 == m_fP1 && p2 == m_fP2 && p3 == m_fP3);
    case Linear:
      return (p1 == m_fP1 && p2 == m_fP2);
    case Exponential:
    case Logarithmic:
      return (p1 == m_fP1 && p2 == m_fP2 && p3 == m_fP3 && p4 == m_fP4);
    }
  }
  return false;
}
///


