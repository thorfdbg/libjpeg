/*
** This header provides the interface for the bitmap hook that 
** delivers the bitmap data to the core library.
**
** $Id: bitmaphook.cpp,v 1.11 2015/03/24 08:39:08 thor Exp $
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
  UWORD comp = tags->GetTagData(JPGTAG_BIO_COMPONENT);
  ULONG miny = tags->GetTagData(JPGTAG_BIO_MINY);
  ULONG maxy = tags->GetTagData(JPGTAG_BIO_MAXY);
  assert(comp < bmm->bmm_usDepth);
  assert(maxy - miny < bmm->bmm_ulHeight);
  
  switch(tags->GetTagData(JPGTAG_BIO_ACTION)) {
  case JPGFLAG_BIO_REQUEST:
    {
      if (bmm->bmm_ucPixelType == CTYP_UBYTE) {
	UBYTE *mem = (UBYTE *)(bmm->bmm_pMemPtr);
	mem += comp;
	mem -= miny * bmm->bmm_usDepth * bmm->bmm_ulWidth;
	tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
	tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
	tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
	tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * bmm->bmm_ulWidth * sizeof(UBYTE));
	tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,bmm->bmm_usDepth * sizeof(UBYTE));
	tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucPixelType);
      } else if (bmm->bmm_ucPixelType == CTYP_UWORD) {	
	UWORD *mem = (UWORD *)(bmm->bmm_pMemPtr);
	mem += comp;
	mem -= miny * bmm->bmm_usDepth * bmm->bmm_ulWidth;
	tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
	tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
	tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
	tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * bmm->bmm_ulWidth * sizeof(UWORD));
	tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,bmm->bmm_usDepth * sizeof(UWORD));
	tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucPixelType);
     } else if (bmm->bmm_ucPixelType == CTYP_FLOAT) {	
	FLOAT *mem = (FLOAT *)(bmm->bmm_pMemPtr);
	mem += comp;
	mem -= miny * bmm->bmm_usDepth * bmm->bmm_ulWidth;
	tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
	tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
	tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
	tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_usDepth * bmm->bmm_ulWidth * sizeof(FLOAT));
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
	if (bmm->bmm_ucPixelType == CTYP_UBYTE || 
	    bmm->bmm_ucPixelType == CTYP_UWORD || 
	    bmm->bmm_ucPixelType == CTYP_FLOAT) {
	  if (bmm->bmm_pLDRSource && bmm->bmm_pLDRMemPtr) {
	    // A designated LDR source is available. Read from here rather than using
	    // our primitive tone mapper.
	    fread(bmm->bmm_pLDRMemPtr,sizeof(UBYTE),bmm->bmm_ulWidth * height * bmm->bmm_usDepth,
		  bmm->bmm_pLDRSource);
	  }
	  //
	  if (bmm->bmm_pSource) {
	    if (bmm->bmm_bFloat) {
	      if (bmm->bmm_bNoOutputConversion) {
		ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
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
		ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
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
	      fread(bmm->bmm_pMemPtr,bmm->bmm_ucPixelType & CTYP_SIZE_MASK,
		    bmm->bmm_ulWidth * height * bmm->bmm_usDepth,bmm->bmm_pSource);
#ifdef JPG_LIL_ENDIAN
	      // On those bloddy little endian machines, an endian swap is necessary
	      // as PNM is big-endian.
	      if (bmm->bmm_ucPixelType == CTYP_UWORD) {
		ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
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
		  ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
		  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
		  UBYTE *ldr  = (UBYTE *)bmm->bmm_pLDRMemPtr;
		  do {
		    *ldr++ = bmm->bmm_HDR2LDR[*data++];
		  } while(--count);
		} else { // Huh, why tone mapping on 8 bit input? Ok, anyhow....
		  ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
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
      if (comp == bmm->bmm_usDepth - 1) {
	ULONG height = maxy + 1 - miny;
	if (bmm->bmm_ucPixelType == CTYP_UBYTE || 
	    bmm->bmm_ucPixelType == CTYP_UWORD || 
	    bmm->bmm_ucPixelType == CTYP_FLOAT) {
	  if (bmm->bmm_pTarget) {
	    if (bmm->bmm_bFloat) {
	      if (bmm->bmm_bNoOutputConversion) {
		ULONG count = bmm->bmm_ulWidth * height;
		FLOAT *data = (FLOAT *)bmm->bmm_pMemPtr;
		double r = 0.0,g = 0.0,b = 0.0; // shut up the compiler.
		do {
		  switch(bmm->bmm_usDepth) {
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
		ULONG count = bmm->bmm_ulWidth * height;
		UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
		double r = 0.0,g = 0.0,b = 0.0; // shut up the compiler.
		do {
		  switch(bmm->bmm_usDepth) {
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
	      switch(bmm->bmm_usDepth) {
	      case 1:
	      case 3: // The direct cases, can write PPM right away.
#ifdef JPG_LIL_ENDIAN
		// On those bloddy little endian machines, an endian swap is necessary
		// as PNM is big-endian.
		if (bmm->bmm_ucPixelType == CTYP_UWORD) {
		  ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
		  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
		  do {
		    *data = (*data >> 8) | ((*data & 0xff) << 8);
		    data++;
		  } while(--count);
		}
#endif
		fwrite(bmm->bmm_pMemPtr,bmm->bmm_ucPixelType & CTYP_SIZE_MASK,
		       bmm->bmm_ulWidth * height * bmm->bmm_usDepth,bmm->bmm_pTarget);
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
	mem -= miny * bmm->bmm_ulWidth;
	tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
	tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
	tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
	tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_ulWidth * sizeof(UBYTE));
	tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,sizeof(UBYTE));
	tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucAlphaType);
      } else if (bmm->bmm_ucAlphaType == CTYP_UWORD) {	
	UWORD *mem = (UWORD *)(bmm->bmm_pAlphaPtr);
	mem -= miny * bmm->bmm_ulWidth;
	tags->SetTagPtr(JPGTAG_BIO_MEMORY        ,mem);
	tags->SetTagData(JPGTAG_BIO_WIDTH        ,bmm->bmm_ulWidth);
	tags->SetTagData(JPGTAG_BIO_HEIGHT       ,8 + miny);
	tags->SetTagData(JPGTAG_BIO_BYTESPERROW  ,bmm->bmm_ulWidth * sizeof(UWORD));
	tags->SetTagData(JPGTAG_BIO_BYTESPERPIXEL,sizeof(UWORD));
	tags->SetTagData(JPGTAG_BIO_PIXELTYPE    ,bmm->bmm_ucAlphaType);
     } else if (bmm->bmm_ucAlphaType == CTYP_FLOAT) {	
	FLOAT *mem = (FLOAT *)(bmm->bmm_pAlphaPtr);
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
	      fread(bmm->bmm_pAlphaPtr,bmm->bmm_ucAlphaType & CTYP_SIZE_MASK,
		    bmm->bmm_ulWidth * height,bmm->bmm_pAlphaSource);
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
