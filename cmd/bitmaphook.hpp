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
** This header provides the interface for the bitmap hook that 
** delivers the bitmap data to the core library.
**
** $Id: bitmaphook.hpp,v 1.5 2016/10/28 13:58:52 thor Exp $
**
*/

#ifndef CMD_BITMAPHOOK
#define CMD_BITMAPHOOK

/// Includes
#include "interface/types.hpp"
#include "std/stdio.hpp"
#include "cmd/iohelpers.hpp"
///

/// Forwards
struct JPG_Hook;
struct JPG_TagItem;
///

/// Administration of bitmap memory.
struct BitmapMemory {
  APTR         bmm_pMemPtr;     // interleaved memory for the HDR image
  APTR         bmm_pLDRMemPtr;  // interleaved memory for the LDR version of the image
  APTR         bmm_pAlphaPtr;   // memory for the alpha channel
  ULONG        bmm_ulWidth;     // width in pixels.
  ULONG        bmm_ulHeight;    // height in pixels; this is only one block in our application.
  UWORD        bmm_usDepth;     // number of components.
  UBYTE        bmm_ucPixelType; // precision etc.
  UBYTE        bmm_ucAlphaType; // pixel type of the alpha channel
  HookDataAccessor *bmm_pTarget;     // where to write the data to.
  FILE        *bmm_pSource;     // where the data comes from on reading (encoding)
  FILE        *bmm_pLDRSource;  // if there is a separate source for the LDR image, this is non-NULL.
  HookDataAccessor *bmm_pAlphaTarget;// where the alpha (if any) goes to on decoding
  FILE        *bmm_pAlphaSource;// where the alpha data (if any) comes from. There is no dedicated alpha LDR file
  const UWORD *bmm_HDR2LDR;     // the (simple global) tone mapper used for encoding the image.
  bool         bmm_bFloat;      // is true if the input is floating point
  bool         bmm_bAlphaFloat; // is true if the opacity information is floating point
  bool         bmm_bBigEndian;  // is true if the floating point input is big endian
  bool         bmm_bAlphaBigEndian;     // if true, the floating point alpha channel is big endian
  bool         bmm_bNoOutputConversion; // if true, the FLOAT stays float and the half-map is not applied.
  bool         bmm_bNoAlphaOutputConversion; // ditto for alpha
  bool         bmm_bClamp;      // if set, clamp negative values to zero.
  bool         bmm_bAlphaClamp; // if set, alpha values outside [0,1] will be clamped to range
  DecodedFormat bmm_decodedFormat; // decoded format(ppm or raw)
};
///

/// Prototypes
// Pull the LDR data if there is a separate LDR image
extern JPG_LONG LDRBitmapHook(struct JPG_Hook *hook, struct JPG_TagItem *tags);
// Pull the HDR image or push it on decoding.
extern JPG_LONG BitmapHook(struct JPG_Hook *hook, struct JPG_TagItem *tags);
// Pull the opacity information
extern JPG_LONG AlphaHook(struct JPG_Hook *hook, struct JPG_TagItem *tags);
///

///
#endif
