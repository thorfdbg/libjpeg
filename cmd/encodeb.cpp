/*
** Parameter definition and encoding for profile B.
**
** $Id: encodeb.cpp,v 1.38 2016/01/26 13:25:18 thor Exp $
**
*/

/// Includes
#include "cmd/encodeb.hpp"
#include "cmd/iohelpers.hpp"
#include "cmd/main.hpp"
#include "cmd/bitmaphook.hpp"
#include "cmd/filehook.hpp"
#include "cmd/tmo.hpp"
#include "interface/parameters.hpp"
#include "interface/tagitem.hpp"
#include "interface/hooks.hpp"
#include "interface/jpeg.hpp"
#include "tools/traits.hpp"
#include "std/math.hpp"
#include "std/stdlib.hpp"
#include "std/string.hpp"
#include "std/assert.hpp"
#if ISO_CODE
///

/// FindEncodingParametersB
// This call here finds the encoding parameters for the mult2 aka Trellis XDepth proposal
// It computes a suitable HDR gamma from the input file and the exposure value.
static void FindEncodingParametersB(FILE *in,FILE *ldrin,int w,int h,int count,
				    bool flt,bool bigendian,double &exposure,double efac,
				    double &gamma_hdr,double epsnum,double epsdenum,
				    double &minr,double &maxr,
				    double &ming,double &maxg,
				    double &minb,double &maxb,
				    const float tmo[256],bool linear)
{
  long pos    = ftell(in); 
  int x,y,i;
  double av   = 0.0;
  double factor;
  double min_val = 1.0;
  double gamma_out;
  const double scale   = 1.0 / 65535.0;
  const double ptfive  = 32768.0 / 65535.0; // the offset the code adds. Almost 0.5.
  
  if (!flt) {
    fprintf(stderr,"the mult2 code works only for floating point input samples\n"); 
    exit(20);
  }

  if (exposure <= 0.0) {
    //
    // Step 1: Implement the autobalance function of XDepth.
    for(y = 0;y < h;y++) {
      for(x = 0;x < w;x++) {
	for(i = 0;i < count;i++) {
	  av += readFloat(in,bigendian);
	}
      }
    }

    if (isnan(av)) {
      fprintf(stderr,"Error reading the source image.\n");
      exit(20);
    }
    
    av      /= double(count) * w * h;
    factor   = efac / av; // auto-exposure changed to 0.6
    exposure = 1.0 / factor;
  } else {
    factor   = 1.0 / exposure;
  }

  // If we have an LDR image, ensure that the denominator cannot be larger than 1.0
  // or we are running into problems!
  if (ldrin) {
    long ldr_pos = ftell(ldrin);
    double max   = 1.0;
    double clip  = exposure * (tmo[0] * scale) / (1.0 + epsdenum);
    if (clip <= 0.0)
      clip = 0.0;
    
    fseek(in,pos,SEEK_SET);
    for(y = 0;y < h;y++) {
      for(x = 0;x < w;x++) {
	for(i = 0;i < count;i++) {
	  double hdrv = readFloat(in,bigendian) * factor;
	  double ldrv = tmo[fgetc(ldrin)] * scale;
	  if (hdrv > clip) {
	    double quot = ldrv / hdrv;
	    if (quot > max)
	      max = quot;
	  }
	}
      }
    }
    fseek(ldrin,ldr_pos,SEEK_SET);
    if (max > 1.0) {
      fprintf(stderr,
	      " Denominator may become larger than 1.0.\n"
	      "adjusting the exposure.\n");
      factor   *= max;
      exposure /= max;
    }
  }
  //
  // XDepth would now scale the entire HDR image by the factor (i.e. multiply each sample vector)
  // Since the HDR image is given here, we cannot do much about it.
  //
  if (linear) {
    // In this case, no gamma is here. Just set it to 1.0
    gamma_out = 1.0;
  } else {
    // Step 2: Implement pass 1 of the XDepth code. This would already see the scaled image.
    // This computes the minimum pixel intensity.
    fseek(in,pos,SEEK_SET);
    for(y = 0;y < h;y++) {
      for(x = 0;x < w;x++) {
	for(i = 0;i < count;i++) {
	  float px = readFloat(in,bigendian) * factor;
	  if (isnan(px)) {
	    fprintf(stderr,"Error reading the source image.\n");
	    exit(20);
	  }
	  if (px > 0.00001) {
	    double curval = 1.0 / px;
	    if (curval > 0.0 && curval < min_val)
	      min_val = curval;
	  }
	}
      }
    }
    // The XDepth code calls the above minimum value "prev_gamma". I keep min_val here.
    // Now start with:
    // Step 3: Implement pass 2 of the XDepth code. Pass 1 did not change the image.
    // Unclear what the 0.5 does here. Should I scale this by "factor"? I probably should
    // since the auto-exposure does that. Ok, hope the best... 
    if (min_val < 0.5) {
      gamma_out = log(min_val) / log(0.5);
      if (gamma_out > 8.0)
	gamma_out = 8.0;
    } else {
      gamma_out = 1.0;
    }
  }
  fseek(in,pos,SEEK_SET);
  //
  // Use the same settings as XDepth
  gamma_hdr = gamma_out;
  //
  // If there is an LDR image, compute the residual and its size and make sure that it scales to 0..1
  if (ldrin) {
    double r0 =  HUGE_VAL;
    double r1 = -HUGE_VAL;
    double g0 =  HUGE_VAL;
    double g1 = -HUGE_VAL;
    double b0 =  HUGE_VAL;
    double b1 = -HUGE_VAL;
    long ldr_pos = ftell(ldrin);
    double y;
    int yi;
    double r,g,b;
    int ri,gi,bi;
    //
    fseek(in,pos,SEEK_SET);
    //
    for(y = 0;y < h;y++) {
      for(x = 0;x < w;x++) {
	if (count == 1) {
	  y  = readFloat(in,bigendian) * factor;
	  yi = getc(ldrin);
	  if (linear) {
	    if (y >= epsnum) {
	      y = -log((tmo[yi] * scale + epsnum) / y) + ptfive;
	      if (y < r0) r0 = y;
	      if (y > r1) r1 = y;
	    }
	  } else {
	    if (y >= epsnum) {
	      y = (tmo[yi] * scale + epsnum) / y;
	      if (y < r0) r0 = y;
	      if (y > r1) r1 = y;
	    }
	  }
	} else if (count == 3) {
	  r  = readFloat(in,bigendian) * factor;
	  g  = readFloat(in,bigendian) * factor;
	  b  = readFloat(in,bigendian) * factor;
	  ri = getc(ldrin);
	  gi = getc(ldrin);
	  bi = getc(ldrin);
	  if (linear) {
	    if (r >= epsnum && b >= epsnum && g >= epsnum) {
	      r = -log((tmo[ri] * scale + epsnum) / r) + ptfive;
	      g = -log((tmo[gi] * scale + epsnum) / g) + ptfive;
	      b = -log((tmo[bi] * scale + epsnum) / b) + ptfive;
	      if (r < r0) r0 = r;
	      if (r > r1) r1 = r;
	      if (g < g0) g0 = g;
	      if (g > g1) g1 = g;
	      if (b < b0) b0 = b;
	      if (b > b1) b1 = b;
	    }
	  } else {
	    if (r >= epsnum && b >= epsnum && g >= epsnum) {
	      r  = (tmo[ri] * scale + epsnum) / r;
	      g  = (tmo[gi] * scale + epsnum) / g;
	      b  = (tmo[bi] * scale + epsnum) / b;
	      if (r < r0) r0 = r;
	      if (r > r1) r1 = r;
	      if (g < g0) g0 = g;
	      if (g > g1) g1 = g;
	      if (b < b0) b0 = b;
	      if (b > b1) b1 = b;
	    }
	  }
	}
      }
    }
    // Test for errors.
    if (ferror(ldrin) || ferror(in)) {
      perror("unable to read source images");
    } else if (feof(ldrin) || feof(in)) {
      fprintf(stderr,"Unexpected end of file when reading source images\n");
    } else {
      if (r1 > r0) {
	minr = r0 - epsdenum;
	maxr = r1 - epsdenum;
      }
      if (g1 > g0) {
	ming = g0 - epsdenum;
	maxg = g1 - epsdenum;
      }
      if (b1 > b0) {
	minb = b0 - epsdenum;
	maxb = b1 - epsdenum;
      }
    }
    fseek(ldrin,ldr_pos,SEEK_SET);
  }
  //
  // Rewind to the file where we started.
  fseek(in,pos,SEEK_SET);
}
///

/// EncodeB
// Encode an image in profile A, filling in all the parameters the codec needs.
void EncodeB(const char *source,const char *ldr,const char *target,
	     double exposure,double autoexposure,double gamma,
	     double epsnum,double epsdenum,bool median,int smooth,bool linearres,
	     int quality,int hdrquality,int tabletype,int residualtt,
	     int colortrafo,bool progressive,bool rprogressive,
	     int hiddenbits,int residualhiddenbits,bool optimize,
	     bool openloop,bool deadzone,bool noclamp,const char *sub,const char *resub,
	     const char *alpha,int alphamode,int matte_r,int matte_g,int matte_b,
	     bool alpharesiduals,int alphaquality,int alphahdrquality,
	     int alphatt,int residualalphatt,
	     int ahiddenbits,int ariddenbits,int aresprec,
	     bool aopenloop,bool adeadzone,bool aserms,bool abypass)
{  
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
  UBYTE subx[4],suby[4];
  UBYTE ressubx[4],ressuby[4];
  memset(subx,1,sizeof(subx));
  memset(suby,1,sizeof(suby));
  memset(ressubx,1,sizeof(ressubx));
  memset(ressuby,1,sizeof(ressuby));
  double gamma_hdr;
  UWORD alphaldrtohdr[65536]; // the TMO for alpha (if required)
  FLOAT tonemapping[256];
  FLOAT *invtmo = NULL;

  if (sub) {
    ParseSubsamplingFactors(subx,suby,sub,4);
  }
  if (resub) {
    ParseSubsamplingFactors(ressubx,ressuby,resub,4);
  }
  
  if (hiddenbits) {
    fprintf(stderr,
	    "*** hidden bits in the LDR domain are currently not supported\n"
	    "*** by this encoder with profile B configuration.\n");
    exit(20);
  }
  //
  // Fill the TMO with the default value. This is 2.4 gamma with
  // toe value, or any other gamma value that was passed in.
  // The 65535.0 is the natural scale of the domain.
  for(int i = 0;i < 256;i++) {
    DOUBLE x = i / 255.0;
    if (gamma <= 0.0) {
      if (x < 0.04045) {
	tonemapping[i] = 65535.0 * (x / 12.92);
      } else {
	tonemapping[i] = 65535.0 * (pow((x + 0.055)/(1.055),2.4));
      }
    } else {
      tonemapping[i] = 63335.0 * pow(x,gamma);
    }
  }
  
  {
    int width,height,depth,prec;
    bool big,flt;
    bool alphaflt   = false;
    bool alphabig   = false;
    FILE *ldrin     = NULL;
    FILE *alphain   = NULL; 
    int alphaprec   = 0;
    // Minimum and maximum values to scale the residual to full range.
    double minr = 0.0;
    double maxr = 1.0;
    double ming = 0.0;
    double maxg = 1.0;
    double minb = 0.0;
    double maxb = 1.0;
    //
    // Get the source file.
    FILE *in    = OpenPNMFile(source,width,height,depth,prec,flt,big);
    if (in) {
      if (!flt) {
	fprintf(stderr,"Profile B only handles floating point images, cannot encode integer images.\n");
	fclose(in);
	exit(20);
      }
      //
      // Profile B supports now LDR images as input. If none is available, it falls back to the old
      // gamma plus clipping algorithm.
      if (ldr) {
	int ldrwidth,ldrheight,ldrdepth = 0,ldrprec;
	bool ldrbig,ldrflt;
	ldrin = OpenPNMFile(ldr,ldrwidth,ldrheight,ldrdepth,ldrprec,ldrflt,ldrbig);
	if (ldrin) {
	  if (ldrflt) {
	    fprintf(stderr,"%s is a floating point image, but the LDR image must be 8 bits/sample\n",
		    ldr);
	    ldrdepth = 0;
	  }
	  if (ldrdepth != depth) {
	    fprintf(stderr,"The number of components of %s and %s do not match\n",
		    source,ldr);
	    ldrdepth = 0;
	  }
	  if (ldrwidth != width || ldrheight != height) {
	    fprintf(stderr,"The image dimensions of %s and %s do not match\n",
		    source,ldr);
	    ldrdepth = 0;
	  }
	  if (ldrdepth == 0) {
	    fprintf(stderr,"LDR image unsuitable, will not be used.\n");
	    if (ldrin) 
	      fclose(ldrin);
	    ldrin = NULL;
	  }
	}
	//
	// Ok, here the header of the LDR file is parsed off and the LDR file is sane.
      }
      //
      if (ldrin && gamma < 0.0) {
	printf("**** A gamma value for the LDR image is missing  ***\n"
	       "**** using an sRGB nonlinearity, hoping the best ***\n");
      } else if (ldrin && gamma == 0.0) {
	// This is a new mode that is outside of profile B. It uses a profile-C
	// type of inverse TMO instead of gamma.
	BuildToneMappingFromLDR(in,ldrin,width,height,depth,tonemapping,big,median,noclamp,smooth);
	invtmo = tonemapping;
      }
      //
      // Check for the alpha channel.
      if (alpha) {
	alphain = PrepareAlphaForRead(alpha,width,height,alphaprec,alphaflt,alphabig,
				      alpharesiduals,ahiddenbits,alphaldrtohdr);
      }
      //
      // Only if we know the width, height depth and scale of the image.
      // Note that profile B does not (yet?) handle the case of an LDR image.
      FindEncodingParametersB(in,ldrin,width,height,depth,true,big,exposure,autoexposure,gamma_hdr,
			      epsnum,epsdenum,
			      minr,maxr,ming,maxg,minb,maxb,tonemapping,linearres);
      //
      // Create the output file.
      FILE *out = fopen(target,"wb");
      if (out) {
	int frametype    = JPGFLAG_SEQUENTIAL;
	int aframetype;
	int residualtype = JPGFLAG_SEQUENTIAL;
	int arestype;
	
	if (progressive) {
	  frametype = JPGFLAG_PROGRESSIVE;
	}
	if (rprogressive) {
	  residualtype   = JPGFLAG_PROGRESSIVE;
	}
	if (optimize) {
	  frametype    |= JPGFLAG_OPTIMIZE_HUFFMAN;
	  residualtype |= JPGFLAG_OPTIMIZE_HUFFMAN;
	}
	if (depth == 1)
	  colortrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE;
 

	aframetype = frametype;
	if (alpharesiduals)
	  aframetype |= JPGFLAG_RESIDUAL_CODING;

	arestype = residualtype;
	if (alphahdrquality >= 100) {
	  if (rprogressive) {
	    arestype = JPGFLAG_RESIDUALPROGRESSIVE;
	  } else {
	    arestype = JPGFLAG_RESIDUAL;
	  }
	} else if (abypass) {
	  arestype = JPGFLAG_RESIDUAL;
	}
	
	int ok = 1;
	struct BitmapMemory bmm;
	struct JPG_Hook bmhook(BitmapHook,&bmm);
	struct JPG_Hook ldrhook(LDRBitmapHook,&bmm);
	struct JPG_Hook alphahook(AlphaHook,&bmm);
	
	// Generate the taglist for alpha, even if we don't need it.
	struct JPG_TagItem alphatags[] = {
	  JPG_ValueTag(JPGTAG_IMAGE_PRECISION,alphaprec),
	  // currently, make the frame types the same as the image
	  // can be changed later.
	  JPG_ValueTag(JPGTAG_IMAGE_FRAMETYPE,aframetype), 
	  JPG_ValueTag(JPGTAG_RESIDUAL_FRAMETYPE,arestype),
	  JPG_ValueTag((alphaquality >= 0)?JPGTAG_IMAGE_QUALITY:JPGTAG_TAG_IGNORE,alphaquality),
	  JPG_ValueTag((alphahdrquality >= 0)?JPGTAG_RESIDUAL_QUALITY:JPGTAG_TAG_IGNORE,
		       alphahdrquality),
	  JPG_ValueTag(JPGTAG_QUANTIZATION_MATRIX,alphatt),
	  JPG_ValueTag(JPGTAG_RESIDUALQUANT_MATRIX,residualalphatt),
	  JPG_ValueTag(JPGTAG_IMAGE_HIDDEN_DCTBITS,ahiddenbits),
	  JPG_ValueTag(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,ariddenbits),
	  JPG_ValueTag(JPGTAG_OPENLOOP_ENCODER,aopenloop),	  
	  JPG_ValueTag(JPGTAG_DEADZONE_QUANTIZER,adeadzone),
	  JPG_ValueTag(JPGTAG_IMAGE_LOSSLESSDCT,aserms),
	  JPG_ValueTag(JPGTAG_ALPHA_MODE,alphamode),
	  JPG_ValueTag(JPGTAG_ALPHA_MATTE(0),matte_r),
	  JPG_ValueTag(JPGTAG_ALPHA_MATTE(1),matte_g),
	  JPG_ValueTag(JPGTAG_ALPHA_MATTE(2),matte_b),
	  JPG_ValueTag(JPGTAG_RESIDUAL_PRECISION,aresprec),
	  // Use a LUT in case we must be lossless.
	  JPG_PointerTag((alpharesiduals && (residualtype == JPGFLAG_RESIDUALDCT ||
					     residualtype == JPGFLAG_RESIDUAL    ||
					     residualtype == JPGFLAG_RESIDUALPROGRESSIVE))?
			 JPGTAG_TONEMAPPING_L_LUT(0):JPGTAG_TAG_IGNORE,alphaldrtohdr),
	  // Ditto. Use linear scaling otherwise.
	  JPG_ValueTag(alpharesiduals?JPGTAG_TONEMAPPING_L_TYPE(0):JPGTAG_TAG_IGNORE,
		       (residualtype == JPGFLAG_RESIDUALDCT ||
			residualtype == JPGFLAG_RESIDUAL    ||
			  residualtype == JPGFLAG_RESIDUALPROGRESSIVE)?
		       JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_IDENTITY),
	  // Define the scan parameters for progressive.
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan1),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan2),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan3),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan4),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan5),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan6),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan7),
	  
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan1),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan2),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan3),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan4),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan5),
	    JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan6),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan7),
	  JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,alphaflt),
	  JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,alphaflt),
	  JPG_EndTag
	};
	
	struct JPG_TagItem tags[] = {
	  JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook),
	  JPG_PointerTag(ldrin?JPGTAG_BIH_LDRHOOK:JPGTAG_TAG_IGNORE,&ldrhook),
	  JPG_PointerTag((alpha)?JPGTAG_BIH_ALPHAHOOK:JPGTAG_TAG_IGNORE,&alphahook),
	  JPG_ValueTag(JPGTAG_ENCODER_LOOP_ON_INCOMPLETE,true),
	  JPG_ValueTag(JPGTAG_IMAGE_WIDTH,width), 
	  JPG_ValueTag(JPGTAG_IMAGE_HEIGHT,height), 
	  JPG_ValueTag(JPGTAG_IMAGE_DEPTH,depth),      
	  JPG_ValueTag(JPGTAG_IMAGE_PRECISION,prec),
	  JPG_ValueTag(JPGTAG_IMAGE_FRAMETYPE,frametype | JPGFLAG_RESIDUAL_CODING),
	  JPG_ValueTag(JPGTAG_RESIDUAL_FRAMETYPE,residualtype),
	  JPG_ValueTag(JPGTAG_IMAGE_QUALITY,quality),
	  JPG_ValueTag(JPGTAG_RESIDUAL_QUALITY,hdrquality),
	  JPG_ValueTag(JPGTAG_QUANTIZATION_MATRIX,tabletype),
	  JPG_ValueTag(JPGTAG_RESIDUALQUANT_MATRIX,residualtt),
	  JPG_PointerTag(JPGTAG_IMAGE_SUBX,subx),
	  JPG_PointerTag(JPGTAG_IMAGE_SUBY,suby),
	  JPG_PointerTag(JPGTAG_RESIDUAL_SUBX,ressubx),
	  JPG_PointerTag(JPGTAG_RESIDUAL_SUBY,ressuby),
	  JPG_ValueTag(JPGTAG_IMAGE_HIDDEN_DCTBITS,hiddenbits),
	  JPG_ValueTag(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,residualhiddenbits),
	  JPG_ValueTag(JPGTAG_OPENLOOP_ENCODER,openloop),
	  JPG_ValueTag(JPGTAG_DEADZONE_QUANTIZER,deadzone),
	  JPG_ValueTag(JPGTAG_MATRIX_LTRAFO,colortrafo),
	  JPG_ValueTag(JPGTAG_MATRIX_RTRAFO,(depth > 1)?JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR:
		       JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan1),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan2),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan3),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan4),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan5),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan6),
	  JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan7),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan1),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan2),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan3),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan4),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan5),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan6),
	  JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,pscan7),
	  JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,true),
	  JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,false),
	  // L-tonemapping. Even though profile B also allows LUTs here, I don't use
	  // them because it is unclear how to set them up. Instead, stick with sRGB.
	  // It is enough to provide the curve type, the codec is smart enough to
	  // pick sRGB as default once "gamma" has been given as a hint.
	  // Components 1 and 2 are ignored on a one-plane (grey-scale) image.
	  JPG_ValueTag(JPGTAG_TONEMAPPING_L_TYPE(0),invtmo?JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_GAMMA),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_L_TYPE(1),invtmo?JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_GAMMA),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_L_TYPE(2),invtmo?JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_GAMMA),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(0,0):JPGTAG_TAG_IGNORE,0.0),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(0,1):JPGTAG_TAG_IGNORE,gamma),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(0,2):JPGTAG_TAG_IGNORE,0.0),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(1,0):JPGTAG_TAG_IGNORE,0.0),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(1,1):JPGTAG_TAG_IGNORE,gamma),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(1,2):JPGTAG_TAG_IGNORE,0.0),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(2,0):JPGTAG_TAG_IGNORE,0.0),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(2,1):JPGTAG_TAG_IGNORE,gamma),
	  JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(2,2):JPGTAG_TAG_IGNORE,0.0),
	  JPG_PointerTag(invtmo?JPGTAG_TONEMAPPING_L_FLUT(0):JPGTAG_TAG_IGNORE,invtmo),
	  JPG_PointerTag(invtmo?JPGTAG_TONEMAPPING_L_FLUT(1):JPGTAG_TAG_IGNORE,invtmo),
	  JPG_PointerTag(invtmo?JPGTAG_TONEMAPPING_L_FLUT(2):JPGTAG_TAG_IGNORE,invtmo),
	  // Currently no C trafo. Probably do that later.
	  // The L2-type must be logarithmic. Again, the codec is smart enough
	  // to pick the right defaults, there is no point in providing anything
	  // else.
	  JPG_ValueTag(JPGTAG_TONEMAPPING_L2_TYPE(0),JPGFLAG_TONEMAPPING_LOGARITHMIC),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_L2_P(0,2),epsnum),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_L2_TYPE(1),JPGFLAG_TONEMAPPING_LOGARITHMIC),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_L2_P(1,2),epsnum),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_L2_TYPE(2),JPGFLAG_TONEMAPPING_LOGARITHMIC),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_L2_P(2,2),epsnum),
	  // The intermediate residual transformation must be a gamma curve with
	  // the hdr gamma computed from above. This parameter cannot be reasonably
	  // guessed by the core codec, thus provide it here.
	  JPG_ValueTag(JPGTAG_TONEMAPPING_R_TYPE(0),linearres?JPGFLAG_TONEMAPPING_IDENTITY:
		       JPGFLAG_TONEMAPPING_POWER),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_R_P(0,2),gamma_hdr), // This is ignored if linearres is on
	  JPG_FloatTag(ldrin?JPGTAG_TONEMAPPING_R_P(0,0):JPGTAG_TAG_IGNORE,minr),
	  JPG_FloatTag(ldrin?JPGTAG_TONEMAPPING_R_P(0,1):JPGTAG_TAG_IGNORE,maxr),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_R_TYPE(1),linearres?JPGFLAG_TONEMAPPING_IDENTITY:
		       JPGFLAG_TONEMAPPING_POWER),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_R_P(1,2),gamma_hdr), // This is ignored if linearres is on   	    
	  JPG_FloatTag(ldrin?JPGTAG_TONEMAPPING_R_P(1,0):JPGTAG_TAG_IGNORE,ming),
	  JPG_FloatTag(ldrin?JPGTAG_TONEMAPPING_R_P(1,1):JPGTAG_TAG_IGNORE,maxg),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_R_TYPE(2),linearres?JPGFLAG_TONEMAPPING_IDENTITY:
		       JPGFLAG_TONEMAPPING_POWER),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_R_P(2,2),gamma_hdr), // This is ignored if linearres is on
	  JPG_FloatTag(ldrin?JPGTAG_TONEMAPPING_R_P(2,0):JPGTAG_TAG_IGNORE,minb),
	  JPG_FloatTag(ldrin?JPGTAG_TONEMAPPING_R_P(2,1):JPGTAG_TAG_IGNORE,maxb),
	  // The R2-transformation. This must be a logarithm with an offset. 
	  // All parameters have reasonable defaults except for epsilon which we
	  // fill in.
	  JPG_ValueTag(JPGTAG_TONEMAPPING_R2_TYPE(0),linearres?JPGFLAG_TONEMAPPING_LINEAR:
		       JPGFLAG_TONEMAPPING_LOGARITHMIC),
	  JPG_FloatTag((ldrin && linearres)?JPGTAG_TONEMAPPING_R2_P(0,0):JPGTAG_TAG_IGNORE,minr),
	  JPG_FloatTag((ldrin && linearres)?JPGTAG_TONEMAPPING_R2_P(0,1):JPGTAG_TAG_IGNORE,maxr),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_R2_P(0,2),epsdenum),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_R2_TYPE(1),linearres?JPGFLAG_TONEMAPPING_LINEAR:
		       JPGFLAG_TONEMAPPING_LOGARITHMIC),
	  JPG_FloatTag((ldrin && linearres)?JPGTAG_TONEMAPPING_R2_P(1,0):JPGTAG_TAG_IGNORE,ming),
	  JPG_FloatTag((ldrin && linearres)?JPGTAG_TONEMAPPING_R2_P(1,1):JPGTAG_TAG_IGNORE,maxg),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_R2_P(1,2),epsdenum),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_R2_TYPE(2),linearres?JPGFLAG_TONEMAPPING_LINEAR:
		       JPGFLAG_TONEMAPPING_LOGARITHMIC),
	  JPG_FloatTag((ldrin && linearres)?JPGTAG_TONEMAPPING_R2_P(2,0):JPGTAG_TAG_IGNORE,minb),
	  JPG_FloatTag((ldrin && linearres)?JPGTAG_TONEMAPPING_R2_P(2,1):JPGTAG_TAG_IGNORE,maxb),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_R2_P(2,2),epsdenum),
	  // The output transformation. This is an exponential. The only unknown
	  // factor is here the exposure value which needs to be provided.
	  // Everything else has defaults.
	  JPG_ValueTag(JPGTAG_TONEMAPPING_O_TYPE(0),JPGFLAG_TONEMAPPING_EXPONENTIAL),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_O_P(0,2),exposure),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_O_TYPE(1),JPGFLAG_TONEMAPPING_EXPONENTIAL),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_O_P(1,2),exposure),
	  JPG_ValueTag(JPGTAG_TONEMAPPING_O_TYPE(2),JPGFLAG_TONEMAPPING_EXPONENTIAL),
	  JPG_FloatTag(JPGTAG_TONEMAPPING_O_P(2,2),exposure),
	  // Information on the alpha channel.
	  JPG_PointerTag(alphain?JPGTAG_ALPHA_TAGLIST:JPGTAG_TAG_IGNORE,alphatags),
	  // That's all folks!
	  JPG_EndTag
	};
	//
	class JPEG *jpeg = JPEG::Construct(NULL);
	if (jpeg) {
	  UBYTE bytesperpixel = sizeof(FLOAT);
	  UBYTE pixeltype     = CTYP_FLOAT;
	  //
	  // Fill in the administration of the bitmap hook.
	  UBYTE *mem          = (UBYTE *)malloc(width * 8 * depth * bytesperpixel + width * 8 * depth);
	  UBYTE *alphamem     = NULL;
	  if (mem) {
	    bmm.bmm_pMemPtr      = mem + width * 8 * depth;
	    bmm.bmm_pAlphaPtr    = NULL;
	    bmm.bmm_pAlphaSource = NULL;
	    bmm.bmm_pLDRMemPtr   = (ldrin)?(mem):(NULL);
	    bmm.bmm_ulWidth      = width;
	    bmm.bmm_ulHeight     = height;
	    bmm.bmm_usDepth      = depth;
	    bmm.bmm_ucPixelType  = pixeltype;
	    bmm.bmm_pTarget      = NULL;
	    bmm.bmm_pSource      = in;
	    bmm.bmm_pLDRSource   = ldrin;
	    bmm.bmm_bFloat       = true;
	    bmm.bmm_bBigEndian   = big;
	    bmm.bmm_HDR2LDR      = NULL;
	    bmm.bmm_bNoOutputConversion = true;
	    bmm.bmm_bClamp       = !noclamp;
	    //  
	    // Create the buffer for the alpha channel.
	    if (alphain) {
	      UBYTE alphabytesperpixel = sizeof(UBYTE);
	      UBYTE alphapixeltype     = CTYP_UBYTE;
	      if (alphaprec > 8) {
		alphabytesperpixel = sizeof(UWORD);
		alphapixeltype     = CTYP_UWORD;
	      }
	      alphamem = (UBYTE *)malloc(width * 8 * alphabytesperpixel + width * 8);
	      if (alphamem) {
		bmm.bmm_pAlphaPtr    = alphamem + width * 8;
		bmm.bmm_ucAlphaType  = alphapixeltype;
		bmm.bmm_pAlphaSource = alphain;
		bmm.bmm_bAlphaFloat  = alphaflt;
		bmm.bmm_bAlphaBigEndian          = alphabig;
		bmm.bmm_bNoAlphaOutputConversion = false;
		bmm.bmm_bAlphaClamp      = !noclamp;
	      }
	    }
	    //
	    // Ready to go?
	    if (alphain == NULL || alphamem) {
	      // Push the image into the frame. We could
	      // get away with writing the image as it is
	      // pushed into the image, but then only a single
	      // scan is allowed. 
	      ok = jpeg->ProvideImage(tags);
	      //
	      // Start writing the image.
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
	      if (alphamem)
		free(alphamem);
	    } else {
	      fprintf(stderr,"failed allocating a buffer for the alpha channel\n");
	    }
	    free(mem);
	  } else {
	    fprintf(stderr,"failed allocating a buffer for the image");
	  }
	  JPEG::Destruct(jpeg);
	} else {
	  fprintf(stderr,"failed to create a JPEG object");
	}
	fclose(out);
      } else {
	  perror("unable to open the output file");
      }
      if (ldrin)
	fclose(ldrin);
      fclose(in);
    }
  }
}
///

/// SplitQualityB
// Provide a useful default for splitting the quality between LDR and HDR.
void SplitQualityB(int splitquality,int &quality,int &hdrquality)
{
  // Let's make a 50-50 split.
  hdrquality = 0.5 * splitquality;
  quality    = splitquality - hdrquality;
  if (hdrquality > 100) {
    quality += hdrquality - 100;
    if (quality > 100)
      quality = 100;
    hdrquality  = 100;
  }
}
///

///
#endif
