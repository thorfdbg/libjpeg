/*
** Parameter definition and encoding for profile A.
**
** $Id: encodea.cpp,v 1.26 2016/01/26 13:25:18 thor Exp $
**
*/

/// Includes
#include "cmd/encodea.hpp"
#include "cmd/iohelpers.hpp"
#include "cmd/main.hpp"
#include "cmd/defaulttmoc.hpp"
#include "cmd/bitmaphook.hpp"
#include "cmd/filehook.hpp"
#include "cmd/tmo.hpp"
#include "interface/parameters.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/jpeg.hpp"
#include "tools/traits.hpp"
#include "std/math.hpp"
#include "std/assert.hpp"
#include "std/string.hpp"
#include "std/stdlib.hpp"
#if ISO_CODE
///

/// Defines
#define COLOR_BITS 4
///

/// FindEncodingParametersA
// Profile A of the JPEG Extensions requires a couple of parameters
// for defining the R,L and diagonal (pre/post) lookup tables. This function
// goes over the source image (again - not very efficient) to compute these
// parameters. The arguments osp1...ocr2 are the output parameters for
// the configuration of the R and S lut's as they are used by the proposal.
static void FindEncodingParametersA(FILE *hdrin,FILE *ldrin,
				    const UWORD *hdrtoldrmap,
				    int w,int h,int count,
				    bool bigendian,int riddenbits,
				    double &osp1,double &osp2,   // configuration of the s-LUT (post-scaling map)
				    double &ocb1,double &ocb2,   // configuration of the first r-Lut for  cb
				    double &ocr1,double &ocr2,   // configuration of the second r-lut for cr
				    const float gamma_lut[256])  // LUT from LDR to HDR
{
  long hdrpos     = ftell(hdrin); 
  long ldrpos     = (ldrin)?(ftell(ldrin)):(0);
  int x,y;
  double scalemin =  HUGE_VAL;
  double scalemax = -HUGE_VAL;
  double cbmin    = HUGE_VAL;
  double cbmax    = -HUGE_VAL;
  double crmin    = HUGE_VAL;
  double crmax    = -HUGE_VAL;
  double min_lum  = exp(-10.0 * log(10.0));
  double nf        = 0.1f;// noise floor. Whatever... this is the default in the dolby impl.
  double sp1,sp2;   // encoding parameters for the S-LUT
  double cbp1,cbp2; // encoding parameters for the R-LUT of cb
  double crp1,crp2; // encoding parameters for the R-LUT of cr
  double outscale = 1.0 / 65535.0; // un-does the output transformation
  int imax        = (1 <<                8) - 1; // again, assume 8 bit LDR data for dolby.
  long rmax       = (1 << (8 + riddenbits)) - 1;
  double rdmax    = rmax;
  
  
  assert(ldrin != NULL || hdrtoldrmap != NULL);
  
  for(y = 0;y < h;y++) {
    for(x = 0;x < w;x++) {
      double rf,gf,bf;    // HDR versions
      double rv,gv,bv;    // tonemapped LDR versions
      int rldr,gldr,bldr; // LDR versions
      double y,yldr;
      double scale;

      if (count == 3) {
	rf = readFloat(hdrin,bigendian);
	gf = readFloat(hdrin,bigendian);
	bf = readFloat(hdrin,bigendian);
	// Compute the LDR versions or read them.
	if (ldrin) {
	  rldr = getc(ldrin);
	  gldr = getc(ldrin);
	  bldr = getc(ldrin);
	} else {
	  rldr = hdrtoldrmap[DoubleToHalf(rf)];
	  gldr = hdrtoldrmap[DoubleToHalf(gf)];
	  bldr = hdrtoldrmap[DoubleToHalf(bf)];
	}
      } else {
	gf = readFloat(hdrin,bigendian);
	if (ldrin) {
	  gldr = getc(ldrin);
	} else {
	  gldr = hdrtoldrmap[DoubleToHalf(gf)];
	}
	// Set to neutral grey.
	rf   = gf;
	bf   = gf;
	rldr = gldr;
	bldr = gldr;
      }
      // 
      if (bldr < 0 || isnan(bf)) {
	fprintf(stderr,"Error reading the image");
	exit(20);
      }
	
      // Get the output mapping of the LDR map, i.e. after application
      // of the L-Lut.
      rv = gamma_lut[rldr] * outscale;
      gv = gamma_lut[gldr] * outscale;
      bv = gamma_lut[bldr] * outscale;
      //
      // Step one: Compute the luminance using 601 colors for both
      // the ldr and hdr image. Note that the std uses 601-luma.
      y    = rf * 0.29900 + gf * 0.58700 + bf * 0.11400;
      yldr = rv * 0.29900 + gv * 0.58700 + bv * 0.11400;
      //
      // The scale ratio (which will go into the luma channel of the residual)
      // is the quotient of y / yldr. If both are zero, the result does not
      // really matter. 
      if (y < min_lum || yldr < min_lum) { // was: yldr > 0.0, but change that to what dolby does.
	scale = 1.0; // Pretty bad if y is not zero here.
      } else {
	scale = y / yldr;
      }
      if (scale < scalemin)
	scalemin = scale;
      if (scale > scalemax)
	scalemax = scale;
    }
  }

  sp1 = log(scalemin);
  sp2 = log(scalemax);

  //
  // Second pass - not very efficient.
  fseek(hdrin,hdrpos,SEEK_SET);
  if (ldrin) {
    fseek(ldrin,ldrpos,SEEK_SET);
  }
  
  //
  // Next go, compute the chrominance residuals.
  for(y = 0;y < h;y++) {
    for(x = 0;x < w;x++) {
      double rf,gf,bf;    // HDR versions
      int rldr,gldr,bldr; // LDR versions
      double rv,gv,bv;    // tonemapped LDR versions
      double y,yldr;
      double scale;
      double cb;
      double cr;

      if (count == 3) {
	rf = readFloat(hdrin,bigendian);
	gf = readFloat(hdrin,bigendian);
	bf = readFloat(hdrin,bigendian);
	// Compute the LDR versions or read them from file.
	if (ldrin) {
	  rldr = getc(ldrin);
	  gldr = getc(ldrin);
	  bldr = getc(ldrin);
	} else {
	  rldr = hdrtoldrmap[DoubleToHalf(rf)];
	  gldr = hdrtoldrmap[DoubleToHalf(gf)];
	  bldr = hdrtoldrmap[DoubleToHalf(bf)];
	}
      } else {
	gf   = readFloat(hdrin,bigendian);
	if (ldrin) {
	  gldr = getc(ldrin);
	} else {
	  gldr = hdrtoldrmap[DoubleToHalf(gf)];
	}
	//
	// Set to neutral grey.
	rf   = gf;
	bf   = gf;
	bldr = gldr;
	rldr = gldr;
      }  
      //
      if (bldr < 0 || isnan(bf)) {
	fprintf(stderr,"Error reading the image");
	exit(20);
      }
      //
      // If the ldr values got clipped, ignore.
      if (rldr == 0    || gldr == 0    || bldr == 0)    continue;
      if (rldr == imax || gldr == imax || bldr == imax) continue;
      //
      // Get the output mapping of the LDR map, i.e. after application
      // of the L-Lut.
      rv = gamma_lut[rldr] * outscale;
      gv = gamma_lut[gldr] * outscale;
      bv = gamma_lut[bldr] * outscale;
      //
      // Step one: Compute the luminance using 601 colors for both
      // the ldr and hdr image. Note that the std uses 601-luma.
      y    = rf * 0.29900 + gf * 0.58700 + bf * 0.11400;
      yldr = rv * 0.29900 + gv * 0.58700 + bv * 0.11400;
      //
      // The scale ratio (which will go into the luma channel of the residual)
      // is the quotient of y / yldr. If both are zero, the result does not
      // really matter.
      if (y < min_lum || yldr < min_lum) { // was: yldr > 0.0, but change that to what dolby does.
	scale = 1.0; // Pretty bad if y is not zero here.
      } else {
	scale = y / yldr;
      } 
      //
      // Ok, now compute the quantized value of the luminance ratio.
      LONG r1 = (log(scale) - sp1) * (rdmax / (sp2 - sp1)) + 0.5f;
      if (r1 < 0)
	r1 = 0;
      if (r1 > rmax)
	r1 = rmax;
      //
      // Mimimic the decoder and reconstruct the value of the scale
      // from the function.
      scale = exp(r1 / rdmax * (sp2 - sp1) + sp1);
      //
      // This is what the decoder would see. Now compute the chrominance
      // residual.
      rf = (rf / scale - rv) / (yldr + nf);
      gf = (gf / scale - gv) / (yldr + nf);
      bf = (bf / scale - bv) / (yldr + nf);
      //
      // Dolby has an additional factor of 0.5 in here. Unclear
      // what this is good for....?????
      //
      // Convert to YCbCr, ignoring the Y output.
#if CHECK_LEVEL > 0      
      y  = rf *  0.29900      + gf *  0.58700      + bf * 0.11400; // not used, only for checking.
      assert(y == y);
#endif      
      cb = rf * -0.1687358916 + gf * -0.3312641084 + bf * 0.5;
      cr = rf *  0.5          + gf * -0.4186875892 + bf *-0.08131241085;
      //
      if (cb < cbmin)
	cbmin = cb;
      if (cb > cbmax)
	cbmax = cb;
      if (cr < crmin)
	crmin = cr;
      if (cr > crmax)
	crmax = cr;
    }
  }
  //
  // Rewind again.
  fseek(hdrin,hdrpos,SEEK_SET);
  if (ldrin)
    fseek(ldrin,ldrpos,SEEK_SET);
  //
  cbp1 = cbmin;
  cbp2 = cbmax;
  crp1 = crmin;
  crp2 = crmax;

  osp1 = sp1;
  osp2 = sp2;
  ocb1 = cbp1;
  ocb2 = cbp2;
  ocr1 = crp1;
  ocr2 = crp2;
}
///

/// EncodeA
// Encode an image in profile A, filling in all the parameters the codec needs.
void EncodeA(const char *source,const char *ldrsource,const char *target,
	     int quality,int hdrquality,
	     int tabletype,int residualtt,int colortrafo,
	     bool progressive,bool rprogressive,
	     int hiddenbits,int residualhiddenbits,bool optimize,
	     bool openloop,bool deadzone,bool noclamp,const char *sub,const char *resub,
	     double gamma,bool median,int smooth,
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
  UWORD hdrtoldr[65536];
  UWORD alphaldrtohdr[65536];  // the TMO for alpha (if required)
  FLOAT tonemapping[256];      // in case we do not simply use the gamma map to generate the inverse TMO
  FLOAT *invtmo = NULL;        // the parameter to be pushed into the core code. NULL for traditional gamma.

  if (sub) {
    ParseSubsamplingFactors(subx,suby,sub,4);
  }
  if (resub) {
    ParseSubsamplingFactors(ressubx,ressuby,resub,4);
  }

  if (hiddenbits) {
    fprintf(stderr,
	    "*** hidden bits in the LDR domain are currently not supported\n"
	    "*** by this encoder with profile A configuration.\n");
    exit(20);
  }
  //
  // Fill the TMO with the default value. This is the sRGB 2.4 gamma
  // with toe value.
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
    FILE *ldrin     = NULL;
    FILE *alphain   = NULL;
    int width,height,depth,prec;
    bool big,flt;
    bool alphaflt   = false;
    bool alphabig   = false;
    int alphaprec   = 0;
    //
    FILE *in    = OpenPNMFile(source,width,height,depth,prec,flt,big);
    if (in) {
      if (!flt) {
	fprintf(stderr,"Profile A only handles floating point images, cannot encode integer images.\n");
	fclose(in);
	exit(20);
      }
      // Now one of the following can happen: Either we have an LDR image and use that,
      // or we compute an automatic TMO similar to what profile C does.
      // The TMO used here is much less sophisticated than what the Dolby software does,
      // but this is sufficient.
      if (ldrsource) {
	// Ok, we do have an LDR image.
	int ldrwidth,ldrheight,ldrdepth = 0,ldrprec;
	bool ldrbig,ldrflt;
	ldrin = OpenPNMFile(ldrsource,ldrwidth,ldrheight,ldrdepth,ldrprec,ldrflt,ldrbig);
	if (ldrin) {
	  if (ldrflt) {
	    fprintf(stderr,"%s is a floating point image, but the LDR image must be 8 bits/sample\n",
		    ldrsource);
	    ldrdepth = 0;
	  }
	  if (ldrdepth != depth) {
	    fprintf(stderr,"The number of components of %s and %s do not match\n",
		    source,ldrsource);
	    ldrdepth = 0;
	  }
	  if (ldrwidth != width || ldrheight != height) {
	    fprintf(stderr,"The image dimensions of %s and %s do not match\n",
		    source,ldrsource);
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
	// Ok, here the header of the LDR image is parsed off if we have one.
      }
      //
      // Check whether we can create a Reinhard TMO if we have no LDR source.
      if (ldrin == NULL) {
	UWORD ldrtohdr[65536];
	BuildToneMapping_C(in,width,height,prec,depth,ldrtohdr,flt,big,false,0);
	InvertTable(ldrtohdr,hdrtoldr,8,16);
      } else if (gamma == 0.0) {
	// Here the user requested to build a custom TMO instead of the
	// default LDR to HDR gamma map.
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
      // Get now the scaling parameters for the Q tables and the S-table.
      {
	double osp1,osp2,ocb1,ocb2,ocr1,ocr2;
	FILE *out;
	FindEncodingParametersA(in,ldrin,hdrtoldr,width,height,depth,big,residualhiddenbits,
				osp1,osp2,ocb1,ocb2,ocr1,ocr2,tonemapping);
	//
	// Convert to the proper range. Use the simplified formulas from
	// Annex F.
	double cbmin = ocb1 + 0.5;
	double cbmax = ocb2 + 0.5;
	double crmin = ocr1 + 0.5;
	double crmax = ocr2 + 0.5;
	//
	double spmin = osp1;
	double spmax = osp2;
	//
	out = fopen(target,"wb");
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
	    JPG_PointerTag(JPGTAG_BIH_LDRHOOK,&ldrhook),
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
	    //
	    // L-Tonemapping. This is always an sRGB-gamma. The core code already picks
	    // this as default and we don't need to do much about it.
	    JPG_ValueTag(JPGTAG_TONEMAPPING_L_TYPE(0),invtmo?JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_GAMMA),
	    JPG_ValueTag(JPGTAG_TONEMAPPING_L_TYPE(1),invtmo?JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_GAMMA),
	    JPG_ValueTag(JPGTAG_TONEMAPPING_L_TYPE(2),invtmo?JPGFLAG_TONEMAPPING_LUT:JPGFLAG_TONEMAPPING_GAMMA),
	    JPG_PointerTag(invtmo?JPGTAG_TONEMAPPING_L_FLUT(0):JPGTAG_TAG_IGNORE,invtmo),
	    JPG_PointerTag(invtmo?JPGTAG_TONEMAPPING_L_FLUT(1):JPGTAG_TAG_IGNORE,invtmo),
	    JPG_PointerTag(invtmo?JPGTAG_TONEMAPPING_L_FLUT(2):JPGTAG_TAG_IGNORE,invtmo),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(0,0):JPGTAG_TAG_IGNORE,0.0),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(0,1):JPGTAG_TAG_IGNORE,gamma),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(0,2):JPGTAG_TAG_IGNORE,0.0),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(1,0):JPGTAG_TAG_IGNORE,0.0),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(1,1):JPGTAG_TAG_IGNORE,gamma),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(1,2):JPGTAG_TAG_IGNORE,0.0),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(2,0):JPGTAG_TAG_IGNORE,0.0),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(2,1):JPGTAG_TAG_IGNORE,gamma),
	    JPG_FloatTag((gamma > 0.0)?JPGTAG_TONEMAPPING_L_P(2,2):JPGTAG_TAG_IGNORE,0.0),
	    // Currently no C trafo. Probably do that later.
	    // No L2-Trafo. Not needed.
	    // Q-trafo: Picks the transformation according to the parameters, just
	    // some scaling is required to get the output right.
	    // For luma, the default values are already correct. For chroma, we need
	    // to fill in image-characteristic values.
	    JPG_ValueTag(JPGTAG_TONEMAPPING_Q_TYPE(0),JPGFLAG_TONEMAPPING_LINEAR),
	    JPG_ValueTag(JPGTAG_TONEMAPPING_Q_TYPE(1),JPGFLAG_TONEMAPPING_LINEAR),
	    JPG_FloatTag(JPGTAG_TONEMAPPING_Q_P(1,0),cbmin),
	    JPG_FloatTag(JPGTAG_TONEMAPPING_Q_P(1,1),cbmax),
	    JPG_ValueTag(JPGTAG_TONEMAPPING_Q_TYPE(2),JPGFLAG_TONEMAPPING_LINEAR),
	    JPG_FloatTag(JPGTAG_TONEMAPPING_Q_P(2,0),crmin),
	    JPG_FloatTag(JPGTAG_TONEMAPPING_Q_P(2,1),crmax),
	    //
	    // Intermediate trafo: not present. Second Base, Second Residual: Not present.
	    // Output transformation. This is a downscale, always linear. Defaults are good.
	    JPG_ValueTag(JPGTAG_TONEMAPPING_O_TYPE(0),JPGFLAG_TONEMAPPING_LINEAR),
	    JPG_ValueTag(JPGTAG_TONEMAPPING_O_TYPE(1),JPGFLAG_TONEMAPPING_LINEAR),
	    JPG_ValueTag(JPGTAG_TONEMAPPING_O_TYPE(2),JPGFLAG_TONEMAPPING_LINEAR),
	    //
	    // Prescaling: again, this is good with the defaults, no need to do anything about it.
	    JPG_ValueTag(JPGTAG_TONEMAPPING_P_TYPE,JPGFLAG_TONEMAPPING_LINEAR),
	    //
	    // Postscaling: This is here an exponential. Profile C would also allow something
	    // more complicated like a table, though we stick with that.
	    JPG_ValueTag(JPGTAG_TONEMAPPING_S_TYPE,JPGFLAG_TONEMAPPING_EXPONENTIAL),
	    JPG_FloatTag(JPGTAG_TONEMAPPING_S_P(0),spmin),
	    JPG_FloatTag(JPGTAG_TONEMAPPING_S_P(1),spmax),
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
	      bmm.bmm_pLDRMemPtr   = mem;
	      bmm.bmm_ulWidth      = width;
	      bmm.bmm_ulHeight     = height;
	      bmm.bmm_usDepth      = depth;
	      bmm.bmm_ucPixelType  = pixeltype;
	      bmm.bmm_pTarget      = NULL;
	      bmm.bmm_pSource      = in;
	      bmm.bmm_pLDRSource   = ldrin;
	      bmm.bmm_bFloat       = true;
	      bmm.bmm_bBigEndian   = big;
	      bmm.bmm_HDR2LDR      = hdrtoldr;
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
	fclose(in);
	if (ldrin)
	  fclose(ldrin);
      }
    }
  }
}	  
///

/// SplitQualityA
// Provide a useful default for splitting the quality between LDR and HDR.
void SplitQualityA(int splitquality,int &quality,int &hdrquality)
{
  // Let's make a 30-70 split.
  hdrquality = 0.3 * splitquality;
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
