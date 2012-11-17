/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** This header provides the main function.
**
** $Id: main.cpp,v 1.84 2012-11-16 19:47:27 thor Exp $
**
*/

/// Includes
#include "main.hpp"
#include "std/stdio.hpp"
#include "std/stdlib.hpp"
#include "std/string.hpp"
#include "std/math.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "interface/jpeg.hpp"
///

/// HalfToDouble
// Interpret a 16-bit integer as half-float casted to int and
// return its double interpretation.
double HalfToDouble(UWORD h)
{
  bool sign      = (h & 0x8000)?(true):(false);
  UBYTE exponent = (h >> 10) & ((1 << 5) - 1);
  UWORD mantissa = h & ((1 << 10) - 1);
  double v;

  if (exponent == 0) { // denormalized
    v = ldexp(mantissa,-14-10);
  } else if (exponent == 31) {
    v = HUGE_VAL;
  } else {
    v = ldexp(mantissa | (1 << 10),-15-10+exponent);
  }

  return (sign)?(-v):(v);
}
///

/// DoubleToHalf
// Convert a double to half-precision IEEE and return the bit-pattern
// as a 16-bit unsigned integer.
UWORD DoubleToHalf(double v)
{
  bool sign = (v < 0.0)?(true):(false);
  int  exponent;
  int  mantissa;

  if (v < 0.0) v = -v;

  if (isinf(v)) {
    exponent = 31;
    mantissa = 0;
  } else if (v == 0.0) {
    exponent = 0;
    mantissa = 0;
  } else {
    double man = 2.0 * frexp(v,&exponent); // must be between 1.0 and 2.0, not 0.5 and 1.
    // Add the exponent bias.
    exponent  += 15 - 1; // exponent bias
    // Normalize the exponent by modifying the mantissa.
    if (exponent >= 31) { // This must be denormalized into an INF, no chance.
      exponent = 31;
      mantissa = 0;
    } else if (exponent <= 0) {
      man *= 0.5; // mantissa does not have an implicit one bit.
      while(exponent < 0) {
	man *= 0.5;
	exponent++;
      }
      mantissa = int(man * (1 << 10));
    } else {
      mantissa = int(man * (1 << 10)) & ((1 << 10) - 1);
    }
  }

  return ((sign)?(0x8000):(0x0000)) | (exponent << 10) | mantissa;
}
///

/// readFloat
// Read an IEEE floating point number from a PFM file
double readFloat(FILE *in,bool bigendian)
{
  LONG dt1,dt2,dt3,dt4;
  union {
    LONG  long_buf;
    FLOAT float_buf;
  } u;

  dt1 = fgetc(in);
  dt2 = fgetc(in);
  dt3 = fgetc(in);
  dt4 = fgetc(in);

  if (bigendian) {
    u.long_buf = (ULONG(dt1) << 24) | (ULONG(dt2) << 16) | 
      (ULONG(dt3) <<  8) | (ULONG(dt4) <<  0);
  } else {
    u.long_buf = (ULONG(dt4) << 24) | (ULONG(dt3) << 16) | 
      (ULONG(dt2) <<  8) | (ULONG(dt1) <<  0);
  }

  return u.float_buf;
}
///

/// writeFloat
// Write a floating point number to a file
void writeFloat(FILE *out,FLOAT f,bool bigendian)
{
  union {
    LONG  long_buf;
    FLOAT float_buf;
  } u;

  u.float_buf = f;

  if (bigendian) {
    fputc(u.long_buf >> 24,out);
    fputc(u.long_buf >> 16,out);
    fputc(u.long_buf >>  8,out);
    fputc(u.long_buf >>  0,out);
  } else {
    fputc(u.long_buf >>  0,out);
    fputc(u.long_buf >>  8,out);
    fputc(u.long_buf >> 16,out);
    fputc(u.long_buf >> 24,out);
  }
}
///

/// Administration of bitmap memory.
struct BitmapMemory {
  APTR   bmm_pMemPtr;     // interleaved memory.
  ULONG  bmm_ulWidth;     // width in pixels.
  ULONG  bmm_ulHeight;    // height in pixels; this is only one block in our application.
  UWORD  bmm_usDepth;     // number of components.
  UBYTE  bmm_ucPixelType; // depth etc.
  FILE  *bmm_pTarget;     // where to write the data to.
  FILE  *bmm_pSource;     // where the data comes from on reading (encoding)
  bool   bmm_bFloat;      // is true if the input is floating point
  bool   bmm_bBigEndian;  // is true if the floating point input is big endian
};
///

/// The Bitmap hook function
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
	if (bmm->bmm_ucPixelType == CTYP_UBYTE || bmm->bmm_ucPixelType == CTYP_UWORD) {
	  if (bmm->bmm_pSource) {
	    if (bmm->bmm_bFloat) {
	      ULONG count = bmm->bmm_ulWidth * height * bmm->bmm_usDepth;
	      UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
	      do {
		double in = readFloat(bmm->bmm_pSource,bmm->bmm_bBigEndian);
		if (in < 0.0)
		  in = 0.0; // Clamp. Negative is not allowed and makes no sense here.
		*data++   = DoubleToHalf(in);
	      } while(--count);
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
	    }
#endif
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
	if (bmm->bmm_ucPixelType == CTYP_UBYTE || bmm->bmm_ucPixelType == CTYP_UWORD) {
	  if (bmm->bmm_pTarget) {
	    if (bmm->bmm_bFloat) {
	      ULONG count = bmm->bmm_ulWidth * height;
	      UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
	      double r = 0.0,g = 0.0,b = 0.0; // shut up the compiler.
	      do {
		switch(bmm->bmm_usDepth) {
		case 1:
		  r = g = b = HalfToDouble(*data++);
		  break;
		case 2:
		  r = HalfToDouble(*data++);
		  g = b = HalfToDouble(*data++);
		  break;
		case 3:
		case 4:
		  r = HalfToDouble(*data++);
		  g = HalfToDouble(*data++);
		  b = HalfToDouble(*data++);
		  break;
		}
		writeFloat(bmm->bmm_pTarget,r,bmm->bmm_bBigEndian);
		writeFloat(bmm->bmm_pTarget,g,bmm->bmm_bBigEndian);
		writeFloat(bmm->bmm_pTarget,b,bmm->bmm_bBigEndian);
	      } while(--count);
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
	      case 2:
		// Bummer! What is this supposed to be?
		if (bmm->bmm_ucPixelType == CTYP_UWORD) {
		  ULONG count = bmm->bmm_ulWidth * height;
		  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
		  do {
		    fputc(*data >> 8,bmm->bmm_pTarget); // PPM is big endian.
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    fputc(*data >> 8,bmm->bmm_pTarget); // PPM is big endian.
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    // Oh well, invent a third channel.
		    fputc(0         ,bmm->bmm_pTarget);
		    fputc(0         ,bmm->bmm_pTarget);
		  } while(--count);
		} else if (bmm->bmm_ucPixelType == CTYP_UBYTE) {
		  ULONG count = bmm->bmm_ulWidth * height;
		  UBYTE *data = (UBYTE *)bmm->bmm_pMemPtr;
		  do {
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    // Oh well, invent a third channel.
		    fputc(0         ,bmm->bmm_pTarget);
		  } while(--count);
		}
		break;
	      case 4:
		// Ignore the alpha channel at this time (if it is alpha at all)
		// Bummer! What is this supposed to be?
		if (bmm->bmm_ucPixelType == CTYP_UWORD) {
		  ULONG count = bmm->bmm_ulWidth * height;
		  UWORD *data = (UWORD *)bmm->bmm_pMemPtr;
		  do {
		    fputc(*data >> 8,bmm->bmm_pTarget); // PPM is big endian.
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    fputc(*data >> 8,bmm->bmm_pTarget); // PPM is big endian.
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++; 
		    fputc(*data >> 8,bmm->bmm_pTarget); // PPM is big endian.
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    data++;
		  } while(--count);
		} else if (bmm->bmm_ucPixelType == CTYP_UBYTE) {
		  ULONG count = bmm->bmm_ulWidth * height;
		  UBYTE *data = (UBYTE *)bmm->bmm_pMemPtr;
		  do {
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    fputc(*data     ,bmm->bmm_pTarget);
		    data++;
		    data++;
		  } while(--count);
		}
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

/// The IO hook function
JPG_LONG FileHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
  FILE *in = (FILE *)(hook->hk_pData);

  switch(tags->GetTagData(JPGTAG_FIO_ACTION)) {
  case JPGFLAG_ACTION_READ:
    {
      UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
      ULONG  size   = (ULONG  )tags->GetTagData(JPGTAG_FIO_SIZE);
      
      return fread(buffer,1,size,in);
    }
  case JPGFLAG_ACTION_WRITE:
    {
      UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
      ULONG  size   = (ULONG  )tags->GetTagData(JPGTAG_FIO_SIZE);
      
      return fwrite(buffer,1,size,in);
    }
  case JPGFLAG_ACTION_SEEK:
    {
      LONG mode   = tags->GetTagData(JPGTAG_FIO_SEEKMODE);
      LONG offset = tags->GetTagData(JPGTAG_FIO_OFFSET);

      switch(mode) {
      case JPGFLAG_OFFSET_CURRENT:
	return fseek(in,offset,SEEK_CUR);
      case JPGFLAG_OFFSET_BEGINNING:
	return fseek(in,offset,SEEK_SET);
      case JPGFLAG_OFFSET_END:
	return fseek(in,offset,SEEK_END);
      }
    }
  case JPGFLAG_ACTION_QUERY:
    return 0;
  }
  return -1;
}
///

/// Reconstruct
// This reconstructs an image from the given input file
// and writes the output ppm.
void Reconstruct(const char *infile,const char *outfile,
		 bool forceint,bool forcefix,int colortrafo,bool pfm)
{  
  FILE *in = fopen(infile,"rb");
  if (in) {
    struct JPG_Hook filehook(FileHook,in);
    class JPEG *jpeg = JPEG::Construct(NULL);
    if (jpeg) {
      int ok = 1;
      struct JPG_TagItem tags[] = {
	JPG_PointerTag(JPGTAG_HOOK_IOHOOK,&filehook),
	JPG_PointerTag(JPGTAG_HOOK_IOSTREAM,in), 
	JPG_ValueTag(JPGTAG_DECODER_FORCEINTEGERDCT,forceint),
	JPG_ValueTag(JPGTAG_DECODER_FORCEFIXPOINTDCT,forcefix),
	JPG_ValueTag(JPGTAG_IMAGE_COLORTRANSFORMATION,colortrafo),
	JPG_EndTag
      };

      if (jpeg->Read(tags)) {
	struct JPG_TagItem itags[] = {
	  JPG_PointerTag(JPGTAG_IMAGE_WIDTH,0),
	  JPG_PointerTag(JPGTAG_IMAGE_HEIGHT,0),
	  JPG_PointerTag(JPGTAG_IMAGE_DEPTH,0),
	  JPG_PointerTag(JPGTAG_IMAGE_PRECISION,0),
	  JPG_EndTag
	};
	if (jpeg->GetInformation(itags)) {
	  ULONG width  = itags->GetTagData(JPGTAG_IMAGE_WIDTH);
	  ULONG height = itags->GetTagData(JPGTAG_IMAGE_HEIGHT);
	  UBYTE depth  = itags->GetTagData(JPGTAG_IMAGE_DEPTH);
	  UBYTE prec   = itags->GetTagData(JPGTAG_IMAGE_PRECISION);
	  
	  
	  UBYTE *mem   = (UBYTE *)malloc(width * 8 * depth * ((prec > 8)?(2):(1)));
	  if (mem) {
	    struct BitmapMemory bmm;
            bmm.bmm_pMemPtr     = mem;
	    bmm.bmm_ulWidth     = width;
	    bmm.bmm_ulHeight    = height;
	    bmm.bmm_usDepth     = depth;
	    bmm.bmm_ucPixelType = (prec > 8)?(CTYP_UWORD):(CTYP_UBYTE);
	    bmm.bmm_pTarget     = fopen(outfile,"wb");
	    bmm.bmm_pSource     = NULL;
	    bmm.bmm_bFloat      = pfm;
	    bmm.bmm_bBigEndian  = true;

	    if (bmm.bmm_pTarget) {
	      ULONG y = 0; // Just as a demo, run a stripe-based reconstruction.
	      ULONG lastline;
	      struct JPG_Hook bmhook(BitmapHook,&bmm);
	      struct JPG_TagItem tags[] = {
		JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook), 
		JPG_ValueTag(JPGTAG_DECODER_MINY,y),
		JPG_ValueTag(JPGTAG_DECODER_MAXY,y+7),
		JPG_EndTag
	      };
	      fprintf(bmm.bmm_pTarget,"P%c\n%d %d\n%d\n",(pfm)?('F'):((depth > 1)?('6'):('5')),
		      width,height,(pfm)?(1):((1 << prec) - 1));

	      //
	      // Reconstruct now the buffered image, line by line. Could also
	      // reconstruct the image as a whole. What we have here is just a demo
	      // that is not necessarily the most efficient way of handling images.
	      do {
		lastline = height;
		if (lastline > y + 8)
		  lastline = y + 8;
		tags[1].ti_Data.ti_lData = y;
		tags[2].ti_Data.ti_lData = lastline - 1;
		ok = jpeg->DisplayRectangle(tags);
		y  = lastline;
	      } while(y < height && ok);

	      fclose(bmm.bmm_pTarget);
	    } else {
	      perror("failed to open the output file");
	    }
	    free(mem);
	  } else {
	    fprintf(stderr,"unable to allocate memory to buffer the image");
	  }
	} else ok = 0;
      } else ok = 0;

      if (!ok) {
	const char *error;
	int code = jpeg->LastError(error);
	fprintf(stderr,"reading a JPEG file failed - error %d - %s\n",code,error);
      }
      JPEG::Destruct(jpeg);
    } else {
      fprintf(stderr,"failed to construct the JPEG object");
    }
    fclose(in);
  } else {
    perror("failed to open the input file");
  }
}
///

/// Parse the subsampling factors off.
void ParseSubsamplingFactors(UBYTE *sx,UBYTE *sy,const char *sub,int cnt)
{
  char *end;

  do {
    int x = strtol(sub,&end,0);
    *sx++ = x;
    if (*end == 'x' || *end == 'X') {
      sub = end + 1;
      int y = strtol(sub,&end,0);
      *sy++ = y;
      if (*end != ',')
	break;
      sub = end + 1;
    } else break;
  } while(--cnt);
}
///



/// BuildToneMapping
// Make a simple attempt to find a reasonable tone mapping from HDR to LDR.
// This is by no means ideal, but seem to work well in most cases.
// 
// The algorithm used here is a simplified version of the exrpptm tone mapper,
// found in the following paper:
// Erik Reinhard and Kate Devlin. Dynamic Range Reduction Inspired by
// Photoreceptor Physiology.  IEEE Transactions on Visualization and
// Computer Graphics (2004).
void BuildToneMapping(FILE *in,int w,int h,int depth,int count,UWORD tonemapping[65536],
		      bool flt,bool bigendian,int hiddenbits)
{
  long pos    = ftell(in);
  int x,y,i;
  int maxin   = 256 << hiddenbits;
  double max  = (1 << depth) - 1;
  double lav  = 0.0;
  double llav = 0.0;
  double minl = HUGE_VAL;
  double maxl =-HUGE_VAL;
  double miny = HUGE_VAL;
  double maxy =-HUGE_VAL;
  double m;
  long cnt = 0;

  for(y = 0;y < h;y++) {
    for(x = 0;x < w;x++) {
      int r,g,b;
      double y;

      if (count == 3) {
	if (flt) { 
	  double rf,gf,bf;
	  rf = readFloat(in,bigendian);
	  gf = readFloat(in,bigendian);
	  bf = readFloat(in,bigendian);
	  y  = (0.2126 * rf + 0.7152 * gf + 0.0722 * bf);
	} else {
	  if (depth <= 8) {
	    r = fgetc(in);
	    g = fgetc(in);
	    b = fgetc(in);
	  } else {
	    r  = fgetc(in) << 8;
	    r |= fgetc(in);
	    g  = fgetc(in) << 8;
	    g |= fgetc(in);
	    b  = fgetc(in) << 8;
	    b |= fgetc(in);
	  }
	  y    = (0.2126 * r + 0.7152 * g + 0.0722 * b) / max;
	}
      } else {
	if (depth <= 8) {
	  g  = fgetc(in);
	} else {
	  g  = fgetc(in) << 8;
	  g |= fgetc(in);
	}
	y = g / max;
      }
      if (y > 0.0) {
	double logy = log(y);
	lav  += y;
	llav += logy;
	if (logy < minl)
	  minl = logy;
	if (logy > maxl)
	  maxl = logy;
	if (y < miny)
	  miny = y;
	if (y > maxy)
	  maxy = y;
	cnt++;
      }
    }
  }

  lav  /= cnt;
  llav /= cnt;
  if (maxl <= minl) {
    m   = 0.3;
  } else {
    double k = (maxl - llav) / (maxl - minl);
    if (k > 0.0) {
      m  = 0.3 + 0.7 * pow(k,1.4);
    } else {
      m  = 0.3;
    }
  }

  fseek(in,pos,SEEK_SET);

  for(i = 0;i < maxin;i++) {
    if (flt) {
      double out = i / double(maxin);
      double in  = pow(pow(lav,m) * out / (1.0 - out),2.2);
      if (in < 0.0) in = 0.0;
      
      tonemapping[i] = DoubleToHalf(in);
    } else {
      double out = i / double(maxin);
      double in  = max * (miny + (maxy - miny) * pow(lav,m) * out / (1.0 - out));
      if (in < 0.0) in = 0.0;
      if (in > max) in = max;
      
      tonemapping[i] = in;
    }
  }
}
///

/// Encode
void Encode(const char *source,const char *target,int quality,int hdrquality,int maxerror,
	    int colortrafo,bool lossless,bool progressive,
	    bool reversible,bool residual,bool optimize,bool accoding,bool dconly,
	    UBYTE levels,bool pyramidal,bool writednl,UWORD restart,double gamma,
	    int lsmode,bool hadamard,bool noiseshaping,int hiddenbits,const char *sub)
{ 
  struct JPG_TagItem dcscan[] = { // scan parameters for the first DCOnly scan
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,0),
    JPG_EndTag
  };
  struct JPG_TagItem pscan1[] = { // standard progressive scan, first scan.
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,0),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,1),
    JPG_EndTag
  };
  struct JPG_TagItem pscan2[] = {
    JPG_ValueTag(JPGTAG_SCAN_COMPONENT0,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,1),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,5),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,2), 
    JPG_EndTag
  };
  struct JPG_TagItem pscan3[] = {
    JPG_ValueTag(JPGTAG_SCAN_COMPONENTS_CHROMA,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,1),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,63),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,1),
    JPG_EndTag
  };
  struct JPG_TagItem pscan4[] = {
    JPG_ValueTag(JPGTAG_SCAN_COMPONENT0,0), 
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,6),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,63),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,2),
    JPG_EndTag
  }; 
  struct JPG_TagItem pscan5[] = {
    JPG_ValueTag(JPGTAG_SCAN_COMPONENT0,0), 
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,1),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,63),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,1),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,2),
    JPG_EndTag
  };  
  struct JPG_TagItem pscan6[] = {
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,0),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,0),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,1),
    JPG_EndTag
  };
  struct JPG_TagItem pscan7[] = {
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,1),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,63),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,0),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,1),
    JPG_EndTag
  };
  UWORD ldrtohdr[65536]; // The tone mapping curve. Currently, only one.
  const UWORD *tonemapping = NULL; // points to the above if used.
  UBYTE subx[4],suby[4];
  memset(subx,0,sizeof(subx));
  memset(suby,0,sizeof(suby));

  if (sub) {
    ParseSubsamplingFactors(subx,suby,sub,4);
  }

  FILE *in = fopen(source,"rb");
  if (in) {
    char id,type;
    int width,height,max,depth,prec;
    bool flt = false;
    bool big = false;
    if (fscanf(in,"%c%c\n",&id,&type) == 2) {
      if (id == 'P' && (type == '5' || type == '6' || type == 'f' || type == 'F')) {
	if (type == '5') {
	  depth = 1;
	} else {
	  depth = 3; // PFM is three-component.
	}
	while((id = fgetc(in)) == '#') {
	  char buffer[256];
	  fgets(buffer,sizeof(buffer),in);
	}
	ungetc(id,in);
	if (fscanf(in,"%d %d %d%*c",&width,&height,&max) == 3) {
	  prec = 0;
	  while((1 << prec) < max)
	    prec++;
	  if (type == 'f' || type == 'F') {
	    prec = 16;
	    flt  = true;
	    if (type == 'F')
	      big = true; // is bigendian
	  }

	  // Create the tone mapping curve if gamma != 1.0
	  if (gamma != 1.0 || hiddenbits) {
	    if (gamma <= 0.0) {
	      BuildToneMapping(in,width,height,prec,depth,ldrtohdr,flt,big,hiddenbits);
	    } else {
	      int i;
	      int outmax  = (flt)?(0x7bff):(256 << hiddenbits); // 0x7c00 is INF in half-float
	      int inmax   = 256 << hiddenbits;
	      double knee = 0.04045;
	      double divs = pow((knee + 0.055) / 1.055,gamma) / knee;
	      for(i = 0;i < inmax;i++) {
		double in = i / double(inmax - 1);
		double out;
		int int_out;
		if (gamma != 1.0) {
		  if (in > knee) {
		    out = pow((in + 0.055) / 1.055,gamma);
		  } else {
		    out = in * divs;
		  }
		  int_out = outmax * out + 0.5;
		} else {
		  int_out = outmax * in + 0.5;
		}
		if (int_out > outmax) int_out = outmax;
		if (int_out < 0     ) int_out = 0;
		ldrtohdr[i] = int_out;
	      }
	    }
	    tonemapping = ldrtohdr;
	  }

	  FILE *out = fopen(target,"wb");
	  if (out) { 
	    int frametype = JPGFLAG_SEQUENTIAL;
	    if (lossless) {
	      frametype = JPGFLAG_LOSSLESS;
	    } else if (progressive) {
	      frametype = JPGFLAG_PROGRESSIVE;
	    } else if (lsmode >=0) {
	      frametype = JPGFLAG_JPEG_LS;
	    }
	    if (reversible) 
	      frametype |= JPGFLAG_REVERSIBLE_DCT;
	    if (residual)
	      frametype |= JPGFLAG_RESIDUAL_CODING;
	    if (optimize)
	      frametype |= JPGFLAG_OPTIMIZE_HUFFMAN;
	    if (accoding)
	      frametype |= JPGFLAG_ARITHMETIC;
	    if (pyramidal)
	      frametype |= JPGFLAG_PYRAMIDAL;
	    
	    {		  
	      int ok = 1;
	      struct BitmapMemory bmm;
	      struct JPG_Hook bmhook(BitmapHook,&bmm);
	      struct JPG_TagItem tags[] = {
		JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook),
		JPG_ValueTag(JPGTAG_DECODER_USE_COLORS,true),
		JPG_ValueTag(JPGTAG_ENCODER_LOOP_ON_INCOMPLETE,true),
		JPG_ValueTag(JPGTAG_IMAGE_WIDTH,width), 
		JPG_ValueTag(JPGTAG_IMAGE_HEIGHT,height), 
		JPG_ValueTag(JPGTAG_IMAGE_DEPTH,depth),      
		JPG_ValueTag(JPGTAG_IMAGE_PRECISION,prec),
		JPG_ValueTag(JPGTAG_IMAGE_FRAMETYPE,frametype),
		JPG_ValueTag((quality >= 0)?JPGTAG_IMAGE_QUALITY:JPGTAG_TAG_IGNORE,quality),
		JPG_ValueTag((hdrquality >= 0)?JPGTAG_IMAGE_HDRQUALITY:JPGTAG_TAG_IGNORE,
			     hdrquality),
		JPG_ValueTag(JPGTAG_IMAGE_ERRORBOUND,maxerror),
		JPG_ValueTag(JPGTAG_IMAGE_RESOLUTIONLEVELS,levels),
		JPG_ValueTag(JPGTAG_IMAGE_COLORTRANSFORMATION,colortrafo),
		JPG_ValueTag(JPGTAG_IMAGE_WRITE_DNL,writednl),
		JPG_ValueTag(JPGTAG_IMAGE_RESTART_INTERVAL,restart),
		JPG_ValueTag(JPGTAG_IMAGE_ENABLE_HADAMARD,hadamard),
		JPG_ValueTag(JPGTAG_IMAGE_ENABLE_NOISESHAPING,noiseshaping),
		JPG_ValueTag(JPGTAG_IMAGE_HIDDEN_DCTBITS,hiddenbits),
		JPG_PointerTag(JPGTAG_IMAGE_SUBX,subx),
		JPG_PointerTag(JPGTAG_IMAGE_SUBY,suby),
		JPG_PointerTag((tonemapping)?(JPGTAG_IMAGE_TONEMAPPING0):JPGTAG_TAG_IGNORE,
			       const_cast<UWORD *>(tonemapping)),
		JPG_PointerTag((tonemapping && depth > 1)?(JPGTAG_IMAGE_TONEMAPPING1):JPGTAG_TAG_IGNORE,
			       const_cast<UWORD *>(tonemapping)),
		JPG_PointerTag((tonemapping && depth > 2)?(JPGTAG_IMAGE_TONEMAPPING2):JPGTAG_TAG_IGNORE,
			       const_cast<UWORD *>(tonemapping)),
		JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(dconly)?(dcscan):(pscan1)),
		JPG_PointerTag((progressive && !dconly)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan2),
		JPG_PointerTag((progressive && !dconly)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan3),
		JPG_PointerTag((progressive && !dconly)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan4),
		JPG_PointerTag((progressive && !dconly)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan5),
		JPG_PointerTag((progressive && !dconly)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan6),
		JPG_PointerTag((progressive && !dconly)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan7),
		JPG_ValueTag((lsmode >= 0)?JPGTAG_SCAN_LS_INTERLEAVING:JPGTAG_TAG_IGNORE,lsmode),
		JPG_EndTag
	      };

	      class JPEG *jpeg = JPEG::Construct(NULL);
	      if (jpeg) {
		UBYTE *mem = (UBYTE *)malloc(width * 8 * depth * ((prec > 8)?(2):(1)));
		if (mem) {
		  bmm.bmm_pMemPtr     = mem;
		  bmm.bmm_ulWidth     = width;
		  bmm.bmm_ulHeight    = height;
		  bmm.bmm_usDepth     = depth;
		  bmm.bmm_ucPixelType = (prec > 8)?(CTYP_UWORD):(CTYP_UBYTE);
		  bmm.bmm_pTarget     = NULL;
		  bmm.bmm_pSource     = in;
		  bmm.bmm_bFloat      = flt;
		  bmm.bmm_bBigEndian  = big;
	      
		  //
		  // Push the image into the frame. We could
		  // get away with writing the image as it is
		  // pushed into the image, but then only a single
		  // scan is allowed. 
		  ok = jpeg->ProvideImage(tags);
		  
		  if (ok) {
		    struct JPG_Hook filehook(FileHook,out);
		    struct JPG_TagItem iotags[] = {
		      JPG_PointerTag(JPGTAG_HOOK_IOHOOK,&filehook),
		      JPG_PointerTag(JPGTAG_HOOK_IOSTREAM,out),
		      JPG_EndTag
		    };

		    //
		    // Write in one go, could interrupt this on each frame,scan,line or MCU.
		    ok = jpeg->Write(iotags);
		  }
		  if (!ok) {
		    const char *error;
		    int code = jpeg->LastError(error);
		    fprintf(stderr,"writing a JPEG file failed - error %d - %s\n",code,error);
		  }
		  free(mem);
		} else {
		  fprintf(stderr,"failed allocating a buffer for the image");
		}
		JPEG::Destruct(jpeg);
	      } else {
		fprintf(stderr,"failed to create a JPEG object");
	      }
	    }
	    fclose(out);
	  } else {
	    perror("unable to open the output file");
	  }
	} else {
	  fprintf(stderr,"unsupported or invalid PNM format\n");
	}
      } else {
	fprintf(stderr,"unsupported or invalid PNM format\n");
      }
    } else {
      fprintf(stderr,"unrecognized input file format, must be PPM or PGM without comments in the header\n");
    }
    fclose(in);
  } else {
    perror("unable to open the input file");
  }
}
///

/// main
int main(int argc,char **argv)
{
  int quality       = -1;
  int hdrquality    = -1;
  int maxerror      = 0;
  int levels        = 0;
  int restart       = 0;
  int lsmode        = -1; // Use JPEGLS
  int hiddenbits    = 0;  // hidden DCT bits
  double gamma      = 1.0;
  bool pyramidal    = false;
  bool reversible   = false;
  bool residuals    = false;
  int  colortrafo   = JPGFLAG_IMAGE_COLORTRANSFORMATION_YCBCR;
  bool forceint     = false;
  bool forcefix     = false;
  bool lossless     = false;
  bool optimize     = false;
  bool accoding     = false;
  bool dconly       = false;
  bool progressive  = false;
  bool writednl     = false;
  bool hadamard     = false;
  bool noiseshaping = false;
  bool pfm          = false;
  const char *sub   = NULL;

  printf("libjpeg,jpeg Copyright (C) 2011-2012 Accusoft, Thomas Richter\n"
	 "This program comes with ABSOLUTELY NO WARRANTY; for details see README.license\n"
	 "This is free software, and you are welcome to redistribute it\n"
	 "under certain conditions; see again README.license for details.\n"
	 "For usage, hints and tips, see README.\n\n");

  while(argc > 3 && argv[1][0] == '-') {
    if (!strcmp(argv[1],"-q")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-q expects a numeric argument\n");
	return 20;
      }
      quality = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-Q")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-Q expects a numeric argument\n");
	return 20;
      }
      hdrquality = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-quality")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-quality expects a numeric argument\n");
	return 20;
      }
      hdrquality = atoi(argv[2]);
      if (hdrquality < 90) {
	quality    = hdrquality;
	hdrquality = 0;
      } else {
	quality    = 90;
	hdrquality = hdrquality - 90;
      }
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-m")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-m expects a numeric argument\n");
	return 20;
      }
      maxerror = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-z")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-z expects a numeric argument\n");
	return 20;
      }
      restart = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-r")) {
      residuals = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-R")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-R expects a numeric argument\n");
	return 20;
      }
      hiddenbits = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-l")) {
      reversible = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-n")) {
      writednl   = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-c")) {
      colortrafo = JPGFLAG_IMAGE_COLORTRANSFORMATION_NONE;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-cls")) {
      colortrafo = JPGFLAG_IMAGE_COLORTRANSFORMATION_LSRCT;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-i")) {
      forceint = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-x")) {
      forcefix = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-s")) {
      sub   = argv[2];
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-p")) {
      lossless = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-h")) {
      optimize = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-a")) {
      accoding = true; 
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-d")) {
      dconly   = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-v")) {
      progressive = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-H")) {
      hadamard  = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-N")) {
      noiseshaping = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-g")) {
      gamma = atof(argv[2]); 
      if (argv[2] == NULL) {
	fprintf(stderr,"-g expects a numeric argument\n");
	return 20;
      }
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-y")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-y expects a numeric argument\n");
	return 20;
      }
      levels = atoi(argv[2]);
      if (levels == 0 || levels == 1) {
	// In this mode, the hierarchical model is used for lossless coding
	levels++;
	pyramidal = false;
      } else {
	pyramidal = true;
      }
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-ls")) {
      if (argv[2] == NULL) {
	fprintf(stderr,"-ls expects a numeric argument\n");
	return 20;
      }
      lsmode = atoi(argv[2]);
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-pfm")) {
      pfm = true;
      argv++;
      argc--;
    } else {
      fprintf(stderr,"unsupported command line switch %s\n",argv[1]);
      return 20;
    }
    // More parameters follow.
  }

  if (argc != 3) {
    fprintf(stderr,"Usage: %s [options] source target\n"
	    "default is to decode the jpeg input and write a ppm output\n"
	    "use -q [1..100] or -p to enforce encoding\n\n"
	    "-q quality : encodes the PPM input and writes JPEG with the given quality\n"
	    "-Q quality : defines the quality for the extension layer\n"
	    "-quality q : defines the base layer quality as max(q,90) and the\n"
	    "             extensions layer quality as max(q-90,0)\n"
	    "-r         : enable lossless coding by encoding residuals, \n"
	    "             also requires -q to define the base quality\n"
	    "-R bits    : enable coding of hidden DCT bits. This works like -r but\n"
	    "             in the DCT domain.\n"
	    "-N         : enable noise shaping of the prediction residual\n"
	    "-l         : enable lossless coding by an int-to-int DCT\n"
	    "             also requires -c and -q 100 for true lossless\n"
	    "-p         : JPEG lossless (predictive) mode\n"
	    "             also requires -c for true lossless\n"
	    "-c         : encode images in RGB rather than YCbCr space\n"
	    "-m maxerr  : defines a maximum pixel errror for JPEG LS coding\n"
	    "-i         : forces to use the integer DCT on decoding\n"
	    "-x         : forces to use the fixput DCT on decoding\n"
	    "-h         : optimize the huffman tables\n"
	    "-a         : use arithmetic coding instead of huffman coding\n"
	    "             available for all coding schemes (-p,-v,-l and default)\n"
	    "-v         : use progressive instead of sequential encoding\n"
	    "             available for all coding schemes (-r,-a,-l and default)\n"
	    "-d         : encode the DC band only (requires -p)\n"
	    "-y levels  : hierarchical JPEG coding with the given number of decomposition\n"
	    "             levels. If levels is zero, then a lossless coding mode for\n"
	    "             hierarchical is used in which the second lossless scan encodes\n"
	    "             the DCT residuals of the first scan. For that, -c is suggested\n"
	    "             for true lossless. If levels is one, then the lossy initial scan\n"
	    "             is downscaled by a power of two.\n"
	    "-g gamma   : define the exponent for the gamma for the LDR domain, or rather, for\n"
	    "             mapping HDR to LDR. A suggested value is 2.4 for mapping scRGB to sRBG.\n"
	    "             setting gamma to zero enables an automatic tone mapping\n"
	    "-z mcus    : define the restart interval size, zero disables it\n"
	    "-n         : indicate the image height by a DNL marker\n"
	    "-H         : enable a Hadamard transformation before encoding the residuals\n"
	    "-s WxH,... : define subsampling factors for all components\n"
	    "             note that these are NOT MCU sizes\n"
	    "             Default is 1x1,1x1,1x1 (444 subsampling)\n"
	    "             1x1,2x2,2x2 is the 420 subsampling often used\n"
	    "-ls mode   : encode in JPEG LS (NOT 10918) mode, where 0 is scan-interleaved,\n"
	    "             1 is line interleaved and 2 is sample interleaved.\n"
	    "             NOTE THAT THIS IS NOT 10918 (JPEG) COMPLIANT, BUT COMPLIANT TO\n"
	    "             14495-1 (JPEG-LS) WHICH IS A DIFFERENT STANDARD.\n"
	    "             Use -c to bypass the YCbCr color transformation for true lossless,\n"
	    "             also use -c for decoding images encoded by the UBC reference software\n"
	    "             as it does not write an indicator marker to disable the\n"
	    "             transformation itself.\n"
	    "             Note that the UBC software will not able to decode streams created by\n"
	    "             this software due to a limitation of the UBC code - the streams are\n"
	    "             nevertheless fully conforming.\n"
	    "             Modes 3 and 4 are also available, but not JPEG LS conforming profiles.\n"
	    "             They rather implement an experimental simple online compression for\n"
	    "             a call from VESA, either using a wavelet or a Hadamard transformation.\n"
	    "             For both modes, the available bandwidth must be provided with the -m\n"
	    "             command line argument. -m 100 specifies 100%% bandwidth, i.e. all \n"
	    "             data is transmitted. -m 25 is a 1:4 compression at 25%% bandwidth.\n"
	    "-cls       : Use a JPEG LS part-2 conforming pseudo-RCT color transformation.\n"
	    "             Note that this transformation is only CONFORMING TO 14495-2\n"
	    "             AND NOT CONFORMING TO 10918-1. Works for near-lossless JPEG LS\n"
	    "             DO NOT USE FOR LOSSY 10918-1, it will also create artifacts.\n"
	    "-pfm       : write 16 bit output as pfm files\n",
	    argv[0]);
    return 5;
  }

  if (quality < 0 && lossless == false && lsmode < 0) {
    Reconstruct(argv[1],argv[2],forceint,forcefix,colortrafo,pfm);
  } else {
    Encode(argv[1],argv[2],quality,hdrquality,maxerror,colortrafo,lossless,progressive,
	   reversible,residuals,optimize,accoding,dconly,levels,pyramidal,writednl,restart,
	   gamma,lsmode,hadamard,noiseshaping,hiddenbits,sub);
  }
  
  return 0;
}
///

