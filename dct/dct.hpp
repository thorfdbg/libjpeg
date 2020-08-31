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
**
** Generic DCT transformation plus quantization class.
** All DCT implementations should derive from this.
**
** $Id: dct.hpp,v 1.12 2016/10/28 13:58:53 thor Exp $
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
class QuantizationTable;
class HuffmanCoder;
///

/// class DCT
// This class is the base class for all DCT implementations.
class DCT : public JKeeper {
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
  virtual void DefineQuant(class QuantizationTable *table) = 0;
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const LONG *source,LONG *target,LONG dcoffset) = 0;
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset) = 0;
  //
  // Estimate a critical slope (lambda) from the unquantized data.
  // Or to be precise, estimate lambda/delta^2, the constant in front of
  // delta^2.
  virtual DOUBLE EstimateCriticalSlope(void) = 0;
  //
  // Return (in case optimization is enabled) a pointer to the unquantized
  // but DCT transformed data. The data is potentially preshifted.
  virtual const LONG *TransformedBlockOf(void) const = 0;
  //
  // Return (in case optimization is enabled) a pointer to the effective
  // quantization step sizes.
  virtual const LONG *BucketSizes(void) const = 0;
  //
  // The prescaling of the DCT. This is the number of bits the input data
  // is upshifted compared to the regular input.
  virtual int PreshiftOf(void) const = 0;
};
///

///
#endif
