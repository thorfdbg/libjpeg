/*
** This file includes the decompressor front-end of the
** command line interface. It doesn't do much except
** calling libjpeg.
**
** $Id: reconstruct.cpp,v 1.2 2015/03/16 08:55:27 thor Exp $
**
*/

/// Includes
#include "std/stdio.hpp"
#include "cmd/reconstruct.hpp"
#include "cmd/bitmaphook.hpp"
#include "cmd/filehook.hpp"
#include "tools/environment.hpp"
#include "tools/traits.hpp"
#include "interface/types.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "interface/jpeg.hpp"
///

/// Reconstruct
// This reconstructs an image from the given input file
// and writes the output ppm.
void Reconstruct(const char *infile,const char *outfile,
		 int colortrafo,const char *alpha)
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
	JPG_ValueTag(JPGTAG_MATRIX_LTRAFO,colortrafo),
	JPG_EndTag
      };

      if (jpeg->Read(tags)) {
	struct JPG_TagItem atags[] = {
	  JPG_ValueTag(JPGTAG_IMAGE_PRECISION,0),
	  JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,false),
	  JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,true),
	  JPG_EndTag
	};
	struct JPG_TagItem itags[] = {
	  JPG_ValueTag(JPGTAG_IMAGE_WIDTH,0),
	  JPG_ValueTag(JPGTAG_IMAGE_HEIGHT,0),
	  JPG_ValueTag(JPGTAG_IMAGE_DEPTH,0),
	  JPG_ValueTag(JPGTAG_IMAGE_PRECISION,0),
	  JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,false),
	  JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,true),
	  JPG_ValueTag(JPGTAG_ALPHA_MODE,JPGFLAG_ALPHA_OPAQUE),
	  JPG_PointerTag(JPGTAG_ALPHA_TAGLIST,atags),
	  JPG_EndTag
	};
	if (jpeg->GetInformation(itags)) {
	  ULONG width  = itags->GetTagData(JPGTAG_IMAGE_WIDTH);
	  ULONG height = itags->GetTagData(JPGTAG_IMAGE_HEIGHT);
	  UBYTE depth  = itags->GetTagData(JPGTAG_IMAGE_DEPTH);
	  UBYTE prec   = itags->GetTagData(JPGTAG_IMAGE_PRECISION);
	  UBYTE aprec  = 0;
	  bool  pfm    = itags->GetTagData(JPGTAG_IMAGE_IS_FLOAT)?true:false;
	  bool convert = itags->GetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION)?true:false;
	  bool doalpha = itags->GetTagData(JPGTAG_ALPHA_MODE,JPGFLAG_ALPHA_OPAQUE)?true:false;
	  bool apfm    = false;
	  bool aconvert= false;
	  
	  if (alpha && doalpha) {
	    aprec    = atags->GetTagData(JPGTAG_IMAGE_PRECISION);
	    apfm     = atags->GetTagData(JPGTAG_IMAGE_IS_FLOAT)?true:false;
	    aconvert = atags->GetTagData(JPGTAG_IMAGE_OUTPUT_CONVERSION)?true:false;
	  } else {
	    doalpha  = false; // alpha channel is ignored.
	  }
	  
	  UBYTE bytesperpixel = sizeof(UBYTE);
	  UBYTE pixeltype     = CTYP_UBYTE;
	  if (prec > 8) {
	    bytesperpixel = sizeof(UWORD);
	    pixeltype     = CTYP_UWORD;
	  }
	  if (pfm && convert == false) {
	    bytesperpixel = sizeof(FLOAT);
	    pixeltype     = CTYP_FLOAT;
	  }

	  UBYTE alphabytesperpixel = sizeof(UBYTE);
	  UBYTE alphapixeltype     = CTYP_UBYTE; 
	  if (aprec > 8) {
	    alphabytesperpixel = sizeof(UWORD);
	    alphapixeltype     = CTYP_UWORD;
	  }
	  if (apfm && aconvert == false) {
	    alphabytesperpixel = sizeof(FLOAT);
	    alphapixeltype     = CTYP_FLOAT;
	  }

	  UBYTE *amem  = NULL;
	  UBYTE *mem   = (UBYTE *)malloc(width * 8 * depth * bytesperpixel);
	  if (doalpha)
	    amem = (UBYTE *)malloc(width * 8 * alphabytesperpixel); // only one component!

	  if (mem) {
	    struct BitmapMemory bmm;
            bmm.bmm_pMemPtr      = mem;
	    bmm.bmm_pAlphaPtr    = amem;
	    bmm.bmm_ulWidth      = width;
	    bmm.bmm_ulHeight     = height;
	    bmm.bmm_usDepth      = depth;
	    bmm.bmm_ucPixelType  = pixeltype;
	    bmm.bmm_ucAlphaType  = alphapixeltype;
	    bmm.bmm_pTarget      = fopen(outfile,"wb");
	    bmm.bmm_pAlphaTarget = (doalpha)?(fopen(alpha,"wb")):NULL;
	    bmm.bmm_pSource      = NULL;
	    bmm.bmm_pAlphaSource = NULL;
	    bmm.bmm_pLDRSource   = NULL;
	    bmm.bmm_bFloat       = pfm;
	    bmm.bmm_bAlphaFloat  = apfm;
	    bmm.bmm_bBigEndian   = true;
	    bmm.bmm_bAlphaBigEndian          = true;
	    bmm.bmm_bNoOutputConversion      = !convert;
	    bmm.bmm_bNoAlphaOutputConversion = !aconvert;

	    if (bmm.bmm_pTarget) {
	      ULONG y = 0; // Just as a demo, run a stripe-based reconstruction.
	      ULONG lastline;
	      struct JPG_Hook bmhook(BitmapHook,&bmm);
	      struct JPG_Hook alphahook(AlphaHook,&bmm);
	      struct JPG_TagItem tags[] = {
		JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook),
		JPG_PointerTag(JPGTAG_BIH_ALPHAHOOK,&alphahook),
		JPG_ValueTag(JPGTAG_DECODER_MINY,y),
		JPG_ValueTag(JPGTAG_DECODER_MAXY,y+7),
		JPG_EndTag
	      };
	      fprintf(bmm.bmm_pTarget,"P%c\n%d %d\n%d\n",
		      (pfm)?((depth > 1)?'F':'f'):((depth > 1)?('6'):('5')),
		      width,height,(pfm)?(1):((1 << prec) - 1));

	      if (bmm.bmm_pAlphaTarget)
		fprintf(bmm.bmm_pAlphaTarget,"P%c\n%d %d\n%d\n",
			(apfm)?('f'):('5'),
			width,height,(apfm)?(1):((1 << aprec) - 1));

	      //
	      // Reconstruct now the buffered image, line by line. Could also
	      // reconstruct the image as a whole. What we have here is just a demo
	      // that is not necessarily the most efficient way of handling images.
	      do {
		lastline = height;
		if (lastline > y + 8)
		  lastline = y + 8;
		tags[2].ti_Data.ti_lData = y;
		tags[3].ti_Data.ti_lData = lastline - 1;
		ok = jpeg->DisplayRectangle(tags);
		y  = lastline;
	      } while(y < height && ok);

	      fclose(bmm.bmm_pTarget);
	    } else {
	      perror("failed to open the output file");
	    }

	    if (bmm.bmm_pAlphaTarget)
	      fclose(bmm.bmm_pAlphaTarget);
	    
	    if (amem)
	      free(amem);

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
