/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
*/

#ifndef DCT_DCT_HPP
#define DCT_DCT_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class Quantization;
struct ImageBitMap;
///

/// class DCT
// This class is the base class for all DCT implementations.
class DCT : public JKeeper {
  //
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
  virtual void DefineQuant(const UWORD *table) = 0;
  //
  // Run the DCT on a 8x8 block on the input data, giving the output table.
  virtual void TransformBlock(const LONG *source,LONG *target,LONG dcoffset) = 0;
  //
  // Run the inverse DCT on an 8x8 block reconstructing the data.
  virtual void InverseTransformBlock(LONG *target,const LONG *source,LONG dcoffset) = 0; 
};
///

///
#endif
