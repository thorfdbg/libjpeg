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
** This header provides the interface for the bitmap hook that 
** delivers the bitmap data to the core library.
**
** $Id: bitmaphook.cpp,v 1.18 2024/01/15 06:47:15 thor Exp $
**
*/

/// Includes
#include "cmd/bitmaphook.hpp"
#include "cmd/iohelpers.hpp"
#include "std/stdio.hpp"
#include "std/stdlib.hpp"
#include "std/string.hpp"
#include "std/assert.hpp"
#include "tools/traits.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
///

/// LDRBitmapHook
// The bitmap hook that supplies the LDR legacy version of the image.
JPG_LONG LDRBitmapHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
  struct BitmapMemory *bmm  = (struct BitmapMemory *)(hook->hk_pData);
  UWORD comp = tags->GetTagData(JPGTAG_BIO_COMPONENT);
  ULONG miny = tags->GetTagData(JPGTAG_BIO_MINY);
#if CHECK_LEVEL > 0
  ULONG maxy = tags->GetTagData(JPGTAG_BIO_MAXY);
#endif
  assert(comp < bmm->bmm_usDepth);
  assert(maxy - miny < bmm->bmm_ulHeight); 
  
  switch(tags->GetTagData(JPGTAG_BIO_ACTION)) {
  case JPGFLAG_BIO_REQUEST:
    if (bmm->bmm_pSource) { // this should only be used on encoding.
      // As this is the legacy stream that is being requested, the data type that is being
      // requested should better be 8 bit unsigned integer.
      UBYTE *mem = (UBYTE *)(bmm->bmm_pLDRMemPtr);
      mem += comp;
      mem -= miny * bmm->bmm_usDepth * bmm->bmm_ulWidth;
      tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
      tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
      tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
      tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * bmm->bmm_ulWidth * sizeof(UBYTE));
      tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,bmm->bmm_usDepth * sizeof(UBYTE));
      tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,CTYP_UBYTE);
    }
    break;
  case JPGFLAG_BIO_RELEASE:
    // Nothing to do here.
    break;
  }

  return 0;
}
///

/// BitmapHook
// The Bitmap hook function
JPG_LONG BitmapHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
  static ULONG OpenComponents = 0;
  struct BitmapMemory *bmm  = (struct BitmapMemory *)(hook->hk_pData);
  UWORD comp  = tags->GetTagData(JPGTAG_BIO_COMPONENT);
  ULONG miny  = tags->GetTagData((bmm->bmm_bUpsampling)?(JPGTAG_BIO_MINY):(JPGTAG_BIO_PIXEL_MINY));
  ULONG maxy  = tags->GetTagData((bmm->bmm_bUpsampling)?(JPGTAG_BIO_MAXY):(JPGTAG_BIO_PIXEL_MAXY));
  ULONG width = 1 + (tags->GetTagData((bmm->bmm_bUpsampling)?(JPGTAG_BIO_MAXX):(JPGTAG_BIO_PIXEL_MAXX)));
  assert(comp < bmm->bmm_usDepth);
  assert(maxy - miny < bmm->bmm_ulHeight);
  
  switch(tags->GetTagData(JPGTAG_BIO_ACTION)) {
  case JPGFLAG_BIO_REQUEST:
    {
      if (bmm->bmm_ucPixelType == CTYP_UBYTE) {
        UBYTE *mem = (UBYTE *)(bmm->bmm_pMemPtr);
        mem += comp;
        mem -= miny * bmm->bmm_usDepth * width;
        tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
        tags->SetTagData(JPGTAG_BIO_WIDTH        ,width);
        tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
        tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * width * sizeof(UBYTE));
        tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,bmm->bmm_usDepth * sizeof(UBYTE));
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucPixelType);
      } else if (bmm->bmm_ucPixelType == CTYP_UWORD) {  
        UWORD *mem = (UWORD *)(bmm->bmm_pMemPtr);
        mem += comp;
        mem -= miny * bmm->bmm_usDepth * width;
        tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
        tags->SetTagData(JPGTAG_BIO_WIDTH        ,width);
        tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
        tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * width * sizeof(UWORD));
        tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,bmm->bmm_usDepth * sizeof(UWORD));
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucPixelType);
     } else if (bmm->bmm_ucPixelType == CTYP_FLOAT) {   
        FLOAT *mem = (FLOAT *)(bmm->bmm_pMemPtr);
        mem += comp;
        mem -= miny * bmm->bmm_usDepth * width;
        tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
        tags->SetTagData(JPGTAG_BIO_WIDTH        ,width);
        tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
        tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * width * sizeof(FLOAT));
        tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,bmm->bmm_usDepth * sizeof(FLOAT));
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucPixelType);
      } else {
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,0);
      }
      // Read the source data.
      if (comp == 0) {
        ULONG height = maxy + 1 - miny;
        // Since we are here indicating the size of the available data, clip to the eight
        // lines available.
        if (height > 8)
          height = 8;
        //
        if (bmm->bmm_ucPixelType == CTYP_UBYTE || 
            bmm->bmm_ucPixelType == CTYP_UWORD || 
            bmm->bmm_ucPixelType == CTYP_FLOAT) {
          if (bmm->bmm_pLDRSource && bmm->bmm_pLDRMemPtr) {
            // A designated LDR source is available. Read from here rather than using
            // our primitive tone mapper.
            size_t cnt = fread(bmm->bmm_pLDRMemPtr,sizeof(UBYTE),width * height * bmm->bmm_usDepth,
                               bmm->bmm_pLDRSource);
            if (cnt != width * height * bmm->bmm_usDepth)
              return JPGERR_UNEXPECTED_EOF;
          }
          //
          if (bmm->bmm_pSource) {
            if (bmm->bmm_bFloat) {
              if (bmm->bmm_bNoOutputConversion) {
                ULONG count = width * height * bmm->bmm_usDepth;
                FLOAT *data = (FLOAT *)bmm->bmm_pMemPtr;
                UBYTE *ldr  = (UBYTE *)bmm->bmm_pLDRMemPtr;
                do {
                  double in = readFloat(bmm->bmm_pSource,bmm->bmm_bBigEndian);
                  UWORD half;
                  if (bmm->bmm_bClamp && in < 0.0)
                    in = 0.0; 
                  half = DoubleToHalf(in);
                  // Tone-map the input unless there is an LDR source.
                  if (bmm->bmm_pLDRMemPtr && bmm->bmm_pLDRSource == NULL)
                    *ldr  = bmm->bmm_HDR2LDR[half];
                  *data = FLOAT(in);
                  data++,ldr++;
                } while(--count);
              } else {
                ULONG count = width * height * bmm->bmm_usDepth;
                UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
                UBYTE *ldr  = (UBYTE *)bmm->bmm_pLDRMemPtr;
                do {
                  double in = readFloat(bmm->bmm_pSource,bmm->bmm_bBigEndian);
                  if (bmm->bmm_bClamp && in < 0.0)
                    in = 0.0;
                  *data = DoubleToHalf(in);
                  // Tone-map the input unless there is an LDR source.
                  if (bmm->bmm_pLDRMemPtr && bmm->bmm_pLDRSource == NULL) {
                    if (in >= 0.0) {
                      *ldr  = bmm->bmm_HDR2LDR[*data];
                    } else {
                      *ldr  = 0;
                    }
                  }
                  data++,ldr++;
                } while(--count);
              }
            } else {
              size_t cnt = fread(bmm->bmm_pMemPtr,bmm->bmm_ucPixelType & CTYP_SIZE_MASK,
                                 width * height * bmm->bmm_usDepth,bmm->bmm_pSource);
              if (cnt != width * height * bmm->bmm_usDepth)
                return JPGERR_UNEXPECTED_EOF;
#ifdef JPG_LIL_ENDIAN
              // On those bloddy little endian machines, an endian swap is necessary
              // as PNM is big-endian.
              if (bmm->bmm_ucPixelType == CTYP_UWORD) {
                ULONG count = width * height * bmm->bmm_usDepth;
                UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
                do {
                  *data = (*data >> 8) | ((*data & 0xff) << 8);
                  data++;
                } while(--count);
              }
#endif
              // Construct the tone-mapped LDR version of the image
              // if there is no designated LDR input.
              if (bmm->bmm_pLDRMemPtr && bmm->bmm_pLDRSource == NULL) {
                if (bmm->bmm_ucPixelType == CTYP_UWORD) {
                  ULONG count = width * height * bmm->bmm_usDepth;
                  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
                  UBYTE *ldr  = (UBYTE *)bmm->bmm_pLDRMemPtr;
                  do {
                    *ldr++ = bmm->bmm_HDR2LDR[*data++];
                  } while(--count);
                } else { // Huh, why tone mapping on 8 bit input? Ok, anyhow....
                  ULONG count = width * height * bmm->bmm_usDepth;
                  UBYTE *data = (UBYTE *)bmm->bmm_pMemPtr;
                  UBYTE *ldr  = (UBYTE *)bmm->bmm_pLDRMemPtr;
                  do {
                    *ldr++ = bmm->bmm_HDR2LDR[*data++];
                  } while(--count);
                }
              }
            }
          }
        }
      }
      assert((OpenComponents & (1UL << comp)) == 0);
      OpenComponents |= 1UL << comp;
    }
    break;
  case JPGFLAG_BIO_RELEASE:
    {
      assert(OpenComponents & (1UL << comp));
      // PGX writes plane-interleaved, not line-interleaved.
      if (bmm->bmm_bWritePGX || comp == bmm->bmm_usDepth - 1) {
        ULONG height = maxy + 1 - miny;
        if (bmm->bmm_ucPixelType == CTYP_UBYTE || 
            bmm->bmm_ucPixelType == CTYP_UWORD || 
            bmm->bmm_ucPixelType == CTYP_FLOAT) {
          if (bmm->bmm_pTarget) {
            if (bmm->bmm_bFloat) {
              if (bmm->bmm_bNoOutputConversion) {
                ULONG count = width * height;
                FLOAT *data = (FLOAT *)bmm->bmm_pMemPtr;
                double r = 0.0,g = 0.0,b = 0.0; // shut up the compiler.
                do {
                  if (bmm->bmm_bWritePGX) {
                    writeFloat(bmm->bmm_PGXFiles[comp],data[comp],bmm->bmm_bBigEndian);
                    data += bmm->bmm_usDepth;
                  } else switch(bmm->bmm_usDepth) {
                    case 1:
                      writeFloat(bmm->bmm_pTarget,*data++,bmm->bmm_bBigEndian);
                      break;
                    case 3:
                      r = *data++;
                      g = *data++;
                      b = *data++;
                      writeFloat(bmm->bmm_pTarget,r,bmm->bmm_bBigEndian);
                      writeFloat(bmm->bmm_pTarget,g,bmm->bmm_bBigEndian);
                      writeFloat(bmm->bmm_pTarget,b,bmm->bmm_bBigEndian);
                      break;
                    }
                } while(--count);
              } else {
                ULONG count = width * height;
                UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
                double r = 0.0,g = 0.0,b = 0.0; // shut up the compiler.
                do {
                  if (bmm->bmm_bWritePGX) {
                    writeFloat(bmm->bmm_PGXFiles[comp],HalfToDouble(data[comp]),bmm->bmm_bBigEndian);
                    data += bmm->bmm_usDepth;
                  } else switch(bmm->bmm_usDepth) {
                    case 1:
                      writeFloat(bmm->bmm_pTarget,HalfToDouble(*data++),bmm->bmm_bBigEndian);
                      break;
                    case 3:
                      r = HalfToDouble(*data++);
                      g = HalfToDouble(*data++);
                      b = HalfToDouble(*data++);
                      writeFloat(bmm->bmm_pTarget,r,bmm->bmm_bBigEndian);
                      writeFloat(bmm->bmm_pTarget,g,bmm->bmm_bBigEndian);
                      writeFloat(bmm->bmm_pTarget,b,bmm->bmm_bBigEndian);
                      break;
                    }
                } while(--count);
              }
            } else {
              if (bmm->bmm_bWritePGX) {
                if (bmm->bmm_ucPixelType == CTYP_UWORD) {
                  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
                  ULONG count = width * height;
                  do {
                    fputc(data[comp] >> 8,bmm->bmm_PGXFiles[comp]);
                    fputc(data[comp]     ,bmm->bmm_PGXFiles[comp]);
                    data += bmm->bmm_usDepth;
                  } while(--count);
                } else {
                  UBYTE *data = (UBYTE *)bmm->bmm_pMemPtr;
                  ULONG count = width * height;
                  do {
                    fputc(data[comp],bmm->bmm_PGXFiles[comp]);
                    data += bmm->bmm_usDepth;
                  } while(--count);
                }
              } else switch(bmm->bmm_usDepth) {
              case 1:
              case 3: // The direct cases, can write PPM right away.
#ifdef JPG_LIL_ENDIAN
                // On those bloddy little endian machines, an endian swap is necessary
                // as PNM is big-endian.
                if (bmm->bmm_ucPixelType == CTYP_UWORD) {
                  ULONG count = width * height * bmm->bmm_usDepth;
                  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
                  do {
                    *data = (*data >> 8) | ((*data & 0xff) << 8);
                    data++;
                  } while(--count);
                }
#endif
                fwrite(bmm->bmm_pMemPtr,bmm->bmm_ucPixelType & CTYP_SIZE_MASK,
                       width * height * bmm->bmm_usDepth,bmm->bmm_pTarget);
                break;
              }
            }
          }
        }
      }
      OpenComponents &= ~(1UL << comp);
    }
    break;
  }
  return 0;
}
///

/// AlphaHook
// This hook reads and writes opacity information.
// There is only one component here, and there is never LDR data.
JPG_LONG AlphaHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
  static ULONG OpenComponents = 0;
  struct BitmapMemory *bmm  = (struct BitmapMemory *)(hook->hk_pData);
  ULONG miny = tags->GetTagData(JPGTAG_BIO_MINY);
  ULONG maxy = tags->GetTagData(JPGTAG_BIO_MAXY);
  assert(maxy - miny < bmm->bmm_ulHeight);
  
  switch(tags->GetTagData(JPGTAG_BIO_ACTION)) {
  case JPGFLAG_BIO_REQUEST:
    {
      if (bmm->bmm_ucAlphaType == CTYP_UBYTE) {
        UBYTE *mem = (UBYTE *)(bmm->bmm_pAlphaPtr);
        if (mem)
          mem -= miny * bmm->bmm_ulWidth;
        tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
        tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
        tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
        tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_ulWidth * sizeof(UBYTE));
        tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,sizeof(UBYTE));
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucAlphaType);
      } else if (bmm->bmm_ucAlphaType == CTYP_UWORD) {  
        UWORD *mem = (UWORD *)(bmm->bmm_pAlphaPtr);
        if (mem)
          mem -= miny * bmm->bmm_ulWidth;
        tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
        tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
        tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
        tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_ulWidth * sizeof(UWORD));
        tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,sizeof(UWORD));
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucAlphaType);
     } else if (bmm->bmm_ucAlphaType == CTYP_FLOAT) {   
        FLOAT *mem = (FLOAT *)(bmm->bmm_pAlphaPtr);
        if (mem)
          mem -= miny * bmm->bmm_ulWidth;
        tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
        tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
        tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
        tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_ulWidth * sizeof(FLOAT));
        tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,sizeof(FLOAT));
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucAlphaType);
      } else {
        tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,0);
      }
      // Read the source data.
      {
        ULONG height = maxy + 1 - miny;
        // Since we are here indicating the size of the available data, clip to the eight
        // lines available.
        if (height > 8)
          height = 8;
        if (bmm->bmm_ucAlphaType == CTYP_UBYTE || 
            bmm->bmm_ucAlphaType == CTYP_UWORD || 
            bmm->bmm_ucAlphaType == CTYP_FLOAT) {
          if (bmm->bmm_pAlphaSource) {
            if (bmm->bmm_bAlphaFloat) {
              if (bmm->bmm_bNoAlphaOutputConversion) {
                ULONG count = bmm->bmm_ulWidth * height;
                FLOAT *data = (FLOAT *)bmm->bmm_pAlphaPtr;
                do {
                  double in = readFloat(bmm->bmm_pAlphaSource,bmm->bmm_bAlphaBigEndian);
                  if (bmm->bmm_bAlphaClamp) {
                    if (in < 0.0)
                      in = 0.0; 
                    if (in > 1.0)
                      in = 1.0;
                  }
                  // No LDR mapping here.
                  *data = FLOAT(in);
                  data++;
                } while(--count);
              } else {
                ULONG count = bmm->bmm_ulWidth * height;
                UWORD *data = (UWORD *)bmm->bmm_pAlphaPtr;
                do {
                  double in = readFloat(bmm->bmm_pAlphaSource,bmm->bmm_bAlphaBigEndian);
                  if (bmm->bmm_bAlphaClamp) {
                    if (in < 0.0)
                      in = 0.0;
                    if (in > 1.0)
                      in = 1.0;
                  }
                  *data = DoubleToHalf(in);
                  // No TMO here.
                  data++;
                } while(--count);
              }
            } else {
              size_t cnt = fread(bmm->bmm_pAlphaPtr,bmm->bmm_ucAlphaType & CTYP_SIZE_MASK,
                                 bmm->bmm_ulWidth * height,bmm->bmm_pAlphaSource);
              if (cnt != bmm->bmm_ulWidth * height)
                return JPGERR_UNEXPECTED_EOF;
#ifdef JPG_LIL_ENDIAN
              // On those bloddy little endian machines, an endian swap is necessary
              // as PNM is big-endian.
              if (bmm->bmm_ucAlphaType == CTYP_UWORD) {
                ULONG count = bmm->bmm_ulWidth * height;
                UWORD *data = (UWORD *)bmm->bmm_pAlphaPtr;
                do {
                  *data = (*data >> 8) | ((*data & 0xff) << 8);
                  data++;
                } while(--count);
              }
#endif
            }
          }
        }
      }
      assert((OpenComponents & (1UL << 4)) == 0);
      OpenComponents |= 1UL << 4;
    }
    break;
  case JPGFLAG_BIO_RELEASE:
    {
      assert(OpenComponents & (1UL << 4));
      {
        ULONG height = maxy + 1 - miny;
        if (bmm->bmm_ucAlphaType == CTYP_UBYTE || 
            bmm->bmm_ucAlphaType == CTYP_UWORD || 
            bmm->bmm_ucAlphaType == CTYP_FLOAT) {
          if (bmm->bmm_pAlphaTarget) {
            if (bmm->bmm_bAlphaFloat) {
              if (bmm->bmm_bNoAlphaOutputConversion) {
                ULONG count = bmm->bmm_ulWidth * height;
                FLOAT *data = (FLOAT *)bmm->bmm_pAlphaPtr;
                do {
                  writeFloat(bmm->bmm_pAlphaTarget,*data++,bmm->bmm_bAlphaBigEndian);
                } while(--count);
              } else {
                ULONG count = bmm->bmm_ulWidth * height;
                UWORD *data = (UWORD *)bmm->bmm_pAlphaPtr;
                do {
                  writeFloat(bmm->bmm_pAlphaTarget,HalfToDouble(*data++),bmm->bmm_bAlphaBigEndian);
                } while(--count);
              }
            } else {
#ifdef JPG_LIL_ENDIAN
              // On those bloddy little endian machines, an endian swap is necessary
              // as PNM is big-endian.
              if (bmm->bmm_ucAlphaType == CTYP_UWORD) {
                ULONG count = bmm->bmm_ulWidth * height;
                UWORD *data = (UWORD *)bmm->bmm_pAlphaPtr;
                do {
                  *data = (*data >> 8) | ((*data & 0xff) << 8);
                  data++;
                } while(--count);
              }
#endif
              fwrite(bmm->bmm_pAlphaPtr,bmm->bmm_ucAlphaType & CTYP_SIZE_MASK,
                     bmm->bmm_ulWidth * height,bmm->bmm_pAlphaTarget);
            }
          }
        }
      }
      OpenComponents &= ~(1UL << 4);
    }
    break;
  }
  return 0;
}
///
