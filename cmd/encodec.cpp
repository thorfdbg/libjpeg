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
** Parameter definition and encoding for profile C.
**
** $Id: encodec.cpp,v 1.42 2024/03/26 10:04:47 thor Exp $
**
*/

/// Includes
#include "cmd/encodec.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "interface/types.hpp"
#include "interface/jpeg.hpp"
#include "cmd/tmo.hpp"
#include "cmd/main.hpp"
#include "cmd/defaulttmoc.hpp"
#include "cmd/filehook.hpp"
#include "cmd/bitmaphook.hpp"
#include "cmd/iohelpers.hpp"
#include "tools/numerics.hpp"
#include "tools/traits.hpp"
#include "std/stdio.hpp"
#include "std/stdlib.hpp"
#include "std/string.hpp"
/// 

/// Defines
#define FIX_BITS JPGFLAG_FIXPOINT_PRESHIFT
///

/// EncodeC
void EncodeC(const char *source,const char *ldrsource,const char *target,const char *ltable,
             int quality,int hdrquality,
             int tabletype,int residualtt,int maxerror,
             int colortrafo,bool baseline,bool lossless,bool progressive,
             bool residual,bool optimize,bool accoding,
             bool rsequential,bool rprogressive,bool raccoding,
             bool qscan,UBYTE levels,bool pyramidal,bool writednl,ULONG restart,double gamma,
             int lsmode,bool noiseshaping,bool serms,bool losslessdct,
             bool openloop,bool deadzone,bool lagrangian,bool dering,
             bool xyz,bool cxyz,
             int hiddenbits,int riddenbits,int resprec,bool separate,
             bool median,bool noclamp,int smooth,
             bool dctbypass,
             const char *sub,const char *ressub,
             const char *alpha,int alphamode,int matte_r,int matte_g,int matte_b,
             bool alpharesiduals,int alphaquality,int alphahdrquality,
             int alphatt,int residualalphatt,
             int ahiddenbits,int ariddenbits,int aresprec,
             bool aopenloop,bool adeadzone,bool alagrangian,bool adering,
             bool aserms,bool abypass,
             const char *quantsteps,const char *residualquantsteps,
             const char *alphasteps,const char *residualalphasteps)
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

  struct JPG_TagItem qscan1[] = { // quick progresssive scan, separates only DC from AC
    JPG_ValueTag(JPGTAG_SCAN_COMPONENT0,0), 
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,0),
    JPG_EndTag
  };
  struct JPG_TagItem qscan2[] = { // quick progresssive scan, separates only DC from AC
    JPG_ValueTag(JPGTAG_SCAN_COMPONENTS_CHROMA,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,0),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,0),
    JPG_EndTag
  };
  struct JPG_TagItem qscan3[] = {
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_START,1),
    JPG_ValueTag(JPGTAG_SCAN_SPECTRUM_STOP,63),
    JPG_EndTag
  };

  struct JPG_TagItem rscan1[] = { // residual progressive scan, first scan.
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,6),
    JPG_EndTag
  };
  struct JPG_TagItem rscan2[] = {
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,5),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,6),
    JPG_EndTag
  }; 
  struct JPG_TagItem rscan3[] = {
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,4),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,5),
    JPG_EndTag
  }; 
  struct JPG_TagItem rscan4[] = {
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,3),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,4),
    JPG_EndTag
  }; 
  struct JPG_TagItem rscan5[] = {
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,2),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,3),
    JPG_EndTag
  }; 
  struct JPG_TagItem rscan6[] = {
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,1),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,2),
    JPG_EndTag
  }; 
  struct JPG_TagItem rscan7[] = {
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_LO,0),
    JPG_ValueTag(JPGTAG_SCAN_APPROXIMATION_HI,1),
    JPG_EndTag
  }; 
  //
  //
  //
  UWORD ldrtohdr[65536]; // The tone mapping curve. Currently, only one.
  UWORD red[65536],green[65536],blue[65536];
  UWORD hdrtoldr[65536]; // Its inverse. This table is used to construct the legacy image.
  UWORD alphaldrtohdr[65536]; // the TMO for alpha (if required)
  const UWORD *tonemapping = NULL; // points to the above if used.
  UBYTE subx[4],suby[4];
  UBYTE ressubx[4],ressuby[4];
  LONG qntmatrix[64],qntmatrixchroma[64];
  LONG residualmatrix[64],residualmatrixchroma[64];
  LONG alphamatrix[64],alphamatrixchroma[64];
  LONG residualalphamatrix[64],residualalphamatrixchroma[64];
  memset(subx,1,sizeof(subx));
  memset(suby,1,sizeof(suby));
  memset(ressubx,1,sizeof(ressubx));
  memset(ressuby,1,sizeof(ressuby));

  if (sub) {
    ParseSubsamplingFactors(subx,suby,sub,4);
  }
  if (ressub) {
    ParseSubsamplingFactors(ressubx,ressuby,ressub,4);
  }

  if (quantsteps) {
    if (ParseQuantizationSteps(qntmatrix,qntmatrixchroma,quantsteps)) 
      tabletype       = JPGFLAG_QUANTIZATION_CUSTOM;
  }
  if (residualquantsteps) {
    if (ParseQuantizationSteps(residualmatrix,residualmatrixchroma,residualquantsteps))
      residualtt      = JPGFLAG_QUANTIZATION_CUSTOM;
  }
  if (alphasteps) {
    if (ParseQuantizationSteps(alphamatrix,alphamatrixchroma,alphasteps))
      alphatt         = JPGFLAG_QUANTIZATION_CUSTOM;
  }
  if (residualalphasteps) {
    if (ParseQuantizationSteps(residualalphamatrix,residualalphamatrixchroma,residualalphasteps))
      residualalphatt = JPGFLAG_QUANTIZATION_CUSTOM;
  }

  {
    int width,height,depth,prec;
    int alphaprec   = 0;
    bool flt       = false;
    bool alphaflt  = false;
    bool big       = false;
    bool alphabig  = false;
    bool fullrange = false;
    FILE *ldrin    = NULL;
    FILE *alphain  = NULL;
    FILE *in       = OpenPNMFile(source,width,height,depth,prec,flt,big);
    if (in) {
      if (ldrsource) {
        int ldrdepth  = 0;
        int ldrwidth  = 0;
        int ldrheight = 0;      
        int ldrprec;
        bool ldrflt;
        bool ldrbig;
        ldrin = OpenPNMFile(ldrsource,ldrwidth,ldrheight,ldrdepth,ldrprec,ldrflt,ldrbig);
        //
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
          if (ldrprec != 8) {
            fprintf(stderr,"unsuitable format for LDR images, must be binary PPM with eight bits/component.\n");
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
      }
      //
      // Create the tone mapping curve if gamma != 1.0
      if ((gamma != 1.0 && residual && prec != 8) || hiddenbits || ltable || ldrin) {
        if (ltable != NULL) {
          LoadLTable(ltable,ldrtohdr,flt,(1L << prec) - 1,hiddenbits);
          if (separate)
            printf("Warning: -sp switch ignored, only one TMO will be used");
          separate = false;
        } else {
          if (gamma <= 0.0) {
            if (ldrin != NULL) {
              if (separate) {
                BuildRGBToneMappingFromLDR(in,ldrin,width,height,prec,depth,
                                           red,green,blue,flt,big,xyz || cxyz,hiddenbits,
                                           median,fullrange,smooth);
              } else {
                BuildToneMappingFromLDR(in,ldrin,width,height,prec,depth,
                                        ldrtohdr,flt,big,xyz || cxyz,hiddenbits,
                                        median,fullrange,smooth);
              }
              if (hiddenbits)
                printf("\n"
                       "Warning: If refinement coding is used, the LDR image will only\n"
                       "be used to create a tone mapping function, but the LDR image\n"
                       "itself will not be stored in the legacy codestream.\n\n");
            } else {
              if (separate)
                printf("Warning: -sp switch ignored, only one TMO will be used");
              separate = false; // Still to be done.
              BuildToneMapping_C(in,width,height,prec,depth,ldrtohdr,flt,big,xyz || cxyz,hiddenbits);
            }
          } else {
            BuildGammaMapping(gamma,1.0,ldrtohdr,flt,(1L << prec) - 1,hiddenbits);
            if (separate)
              printf("Warning: -sp switch ignored, only one TMO will be used");
            separate = false; // Still to be done.
          }
        }
        tonemapping = ldrtohdr;
      }
      //
      if (fullrange) {
        if (lossless || hdrquality >= 100 || dctbypass) {
          fullrange = false;
        } else {
          printf("Found overly large differentials, adding additional scaling step.\n");
        }
      }
      //
      // Automatically disable clamping.
      if (lossless || hdrquality >= 100)
        noclamp = true;
      //
      // Check for the alpha channel.
      if (alpha) { 
        alphain = PrepareAlphaForRead(alpha,width,height,alphaprec,alphaflt,alphabig,
                                      alpharesiduals,ahiddenbits,alphaldrtohdr);
      }
      // Construct the forwards tonemapping curve from inverting the
      // inverse. (-: But first check whether there is an inverse table.
      // If not, build one.
      if (residual || hiddenbits || ltable || ldrin) {
        if (tonemapping == NULL) {
          if (hiddenbits)
            fprintf(stderr,
                    "Warning: Suggested to use automatic tone mapping (-g 0)\n"
                    "instead of a gamma=1.0 value\n");
          BuildGammaMapping(1.0,1.0,ldrtohdr,flt,(1L << prec) - 1,hiddenbits);
          if (separate)
            printf("Warning: -sp switch ignored, only one TMO will be used");
          separate = false; 
        }
        // Now build the inverse.
        InvertTable(ldrtohdr,hdrtoldr,8 + hiddenbits,prec);
      }
      //
      FILE *out = fopen(target,"wb");
      if (out) { 
        int frametype = JPGFLAG_SEQUENTIAL;
        int residualtype = JPGFLAG_RESIDUAL;
        int aframetype;
        int arestype;
        if (baseline) {
          frametype = JPGFLAG_BASELINE;
        } else if (lossless) {
          frametype = JPGFLAG_LOSSLESS;
        } else if (progressive) {
          frametype = JPGFLAG_PROGRESSIVE;
        } else if (lsmode >=0) {
          frametype = JPGFLAG_JPEG_LS;
        }
        if (residual)
          frametype |= JPGFLAG_RESIDUAL_CODING;
        if (optimize)
          frametype |= JPGFLAG_OPTIMIZE_HUFFMAN;
        if (accoding)
          frametype |= JPGFLAG_ARITHMETIC;
        if (pyramidal)
          frametype |= JPGFLAG_PYRAMIDAL;
        if (losslessdct)
          residualtype  = JPGFLAG_RESIDUALDCT;
        if (!lossless && !losslessdct && hdrquality < 100)
          residualtype  = JPGFLAG_SEQUENTIAL;
        if (rsequential && !lossless && hdrquality < 100)
          residualtype  = JPGFLAG_SEQUENTIAL;
        if (rprogressive && !lossless && hdrquality < 100)
          residualtype  = JPGFLAG_PROGRESSIVE;
        if (dctbypass)
          residualtype  = JPGFLAG_RESIDUAL;
        if (residualtype == JPGFLAG_RESIDUAL && rprogressive)
          residualtype  = JPGFLAG_RESIDUALPROGRESSIVE;
        if (raccoding)
          residualtype |= JPGFLAG_ARITHMETIC;
        if (depth == 1)
          colortrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE;

        aframetype = frametype & (~JPGFLAG_RESIDUAL_CODING);
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
        if (raccoding)
          arestype |= JPGFLAG_ARITHMETIC;
        
        {                 
          int ok = 1;
          struct BitmapMemory bmm;
          struct JPG_Hook bmhook(BitmapHook,&bmm);
          struct JPG_Hook ldrhook(LDRBitmapHook,&bmm);
          struct JPG_Hook alphahook(AlphaHook,&bmm);
          //
          // Generate the taglist for alpa, even if we don't need it.
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
            JPG_PointerTag((alphatt == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_QUANTIZATION_LUMATABLE:JPGTAG_TAG_IGNORE,alphamatrix),
            JPG_PointerTag((alphatt == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_QUANTIZATION_CHROMATABLE:JPGTAG_TAG_IGNORE,alphamatrixchroma),
            JPG_PointerTag((residualalphatt == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_RESIDUALQUANT_LUMATABLE:JPGTAG_TAG_IGNORE,residualalphamatrix),
            JPG_PointerTag((residualalphatt == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_RESIDUALQUANT_CHROMATABLE:JPGTAG_TAG_IGNORE,residualalphamatrixchroma),
            JPG_ValueTag(JPGTAG_IMAGE_RESOLUTIONLEVELS,levels),
            JPG_ValueTag(JPGTAG_IMAGE_WRITE_DNL,writednl),
            JPG_ValueTag(JPGTAG_IMAGE_RESTART_INTERVAL,restart),
            JPG_ValueTag(JPGTAG_IMAGE_ENABLE_NOISESHAPING,noiseshaping),
            JPG_ValueTag(JPGTAG_IMAGE_HIDDEN_DCTBITS,ahiddenbits),
            JPG_ValueTag(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,ariddenbits),
            JPG_ValueTag(JPGTAG_IMAGE_LOSSLESSDCT,aserms),
            JPG_ValueTag((alphahdrquality >= 100)?JPGTAG_RESIDUAL_DCT:JPGTAG_TAG_IGNORE,losslessdct),
            JPG_ValueTag(JPGTAG_OPENLOOP_ENCODER,aopenloop),
            JPG_ValueTag(JPGTAG_DEADZONE_QUANTIZER,adeadzone),
            JPG_ValueTag(JPGTAG_OPTIMIZE_QUANTIZER,alagrangian),
            JPG_ValueTag(JPGTAG_IMAGE_DERINGING,adering),
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
            JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(qscan)?(qscan1):(pscan1)),
            JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(qscan)?(qscan2):(pscan2)),
            JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(qscan)?(qscan3):(pscan3)),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan4),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan5),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan6),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan7),
            
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan1:pscan1),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan2:pscan2),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan3:pscan3),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan4:pscan4),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan5:pscan5),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan6:pscan6),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan7:pscan7),
            JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,alphaflt),
            JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,alphaflt),
            JPG_EndTag
          };
          //
          struct JPG_TagItem tags[] = {
            JPG_PointerTag(JPGTAG_BIH_HOOK,&bmhook),
            JPG_PointerTag((alpha)?JPGTAG_BIH_ALPHAHOOK:JPGTAG_TAG_IGNORE,&alphahook),
            JPG_PointerTag((residual && hiddenbits == 0 && ldrin)?JPGTAG_BIH_LDRHOOK:JPGTAG_TAG_IGNORE,&ldrhook),
            JPG_ValueTag(JPGTAG_ENCODER_LOOP_ON_INCOMPLETE,true),
            JPG_ValueTag(JPGTAG_IMAGE_WIDTH,width), 
            JPG_ValueTag(JPGTAG_IMAGE_HEIGHT,height), 
            JPG_ValueTag(JPGTAG_IMAGE_DEPTH,depth),      
            JPG_ValueTag(JPGTAG_IMAGE_PRECISION,prec),
            JPG_ValueTag(JPGTAG_IMAGE_FRAMETYPE,frametype),
            JPG_ValueTag(JPGTAG_DECODER_INCLUDE_ALPHA,alpha?true:false),
            JPG_ValueTag(JPGTAG_RESIDUAL_FRAMETYPE,residualtype),
            JPG_ValueTag((quality >= 0)?JPGTAG_IMAGE_QUALITY:JPGTAG_TAG_IGNORE,quality),
            JPG_ValueTag((hdrquality >= 0)?JPGTAG_RESIDUAL_QUALITY:JPGTAG_TAG_IGNORE,
                         hdrquality),
            JPG_ValueTag(JPGTAG_QUANTIZATION_MATRIX,tabletype),
            JPG_ValueTag(JPGTAG_RESIDUALQUANT_MATRIX,residualtt),
            JPG_PointerTag((tabletype == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_QUANTIZATION_LUMATABLE:JPGTAG_TAG_IGNORE,qntmatrix),
            JPG_PointerTag((tabletype == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_QUANTIZATION_CHROMATABLE:JPGTAG_TAG_IGNORE,qntmatrixchroma),
            JPG_PointerTag((residualtt == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_RESIDUALQUANT_LUMATABLE:JPGTAG_TAG_IGNORE,residualmatrix),
            JPG_PointerTag((residualtt == JPGFLAG_QUANTIZATION_CUSTOM)?JPGTAG_RESIDUALQUANT_CHROMATABLE:JPGTAG_TAG_IGNORE,residualmatrixchroma),
            JPG_ValueTag(JPGTAG_IMAGE_ERRORBOUND,maxerror),
            JPG_ValueTag(JPGTAG_IMAGE_RESOLUTIONLEVELS,levels),
            JPG_ValueTag(JPGTAG_MATRIX_LTRAFO,xyz?JPGFLAG_MATRIX_COLORTRANSFORMATION_FREEFORM:colortrafo),
            JPG_ValueTag((xyz && hdrquality < 100)?JPGTAG_MATRIX_RTRAFO:JPGTAG_TAG_IGNORE,
                         JPGFLAG_MATRIX_COLORTRANSFORMATION_FREEFORM),
            JPG_ValueTag(JPGTAG_IMAGE_WRITE_DNL,writednl),
            JPG_ValueTag(JPGTAG_IMAGE_RESTART_INTERVAL,restart),
            JPG_ValueTag(JPGTAG_IMAGE_ENABLE_NOISESHAPING,noiseshaping),
            JPG_ValueTag(JPGTAG_IMAGE_HIDDEN_DCTBITS,hiddenbits),
            JPG_ValueTag(JPGTAG_RESIDUAL_HIDDEN_DCTBITS,riddenbits),
            JPG_ValueTag(JPGTAG_IMAGE_LOSSLESSDCT,serms),
            JPG_ValueTag((hdrquality >= 100)?JPGTAG_RESIDUAL_DCT:JPGTAG_TAG_IGNORE,losslessdct),
            JPG_PointerTag(JPGTAG_IMAGE_SUBX,subx),
            JPG_PointerTag(JPGTAG_IMAGE_SUBY,suby),
            JPG_PointerTag(JPGTAG_RESIDUAL_SUBX,ressubx),
            JPG_PointerTag(JPGTAG_RESIDUAL_SUBY,ressuby),
            JPG_ValueTag(JPGTAG_OPENLOOP_ENCODER,openloop),
            JPG_ValueTag(JPGTAG_DEADZONE_QUANTIZER,deadzone),
            JPG_ValueTag(JPGTAG_OPTIMIZE_QUANTIZER,lagrangian),
            JPG_ValueTag(JPGTAG_IMAGE_DERINGING,dering),
            JPG_ValueTag(JPGTAG_RESIDUAL_PRECISION,resprec),
            // The RGB2XYZ transformation matrix, used as L-transformation if the xyz flag is true.
            // this is the product of the 601->RGB and RGB->XYZ matrix
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(0,0),TO_FIX(0.95047000)),
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(1,0),TO_FIX(0.1966803389)),
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(2,0),TO_FIX(0.3229058048)),
            
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(0,1),TO_FIX(1.00000010)),
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(1,1),TO_FIX(-0.1182157221)),
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(2,1),TO_FIX(-0.2125487302)),
            
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(0,2),TO_FIX(1.08883000)),
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(1,2),TO_FIX(1.642920573)),
            JPG_ValueTag(JPGTAG_MATRIX_LMATRIX(2,2),TO_FIX(-0.05801320439)),    
            //
            // Same for R, potentially ignored.
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(0,0),TO_FIX(0.95047000)),
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(1,0),TO_FIX(0.1966803389)),
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(2,0),TO_FIX(0.3229058048)),
            
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(0,1),TO_FIX(1.00000010)),
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(1,1),TO_FIX(-0.1182157221)),
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(2,1),TO_FIX(-0.2125487302)),
            
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(0,2),TO_FIX(1.08883000)),
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(1,2),TO_FIX(1.642920573)),
            JPG_ValueTag(JPGTAG_MATRIX_RMATRIX(2,2),TO_FIX(-0.05801320439)),
            //
            // If XYZ encoding is done by the C-transformation, enter the transformation matrix from
            // RGB to XYZ here.
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(0,0)):JPGTAG_TAG_IGNORE,TO_FIX(0.4124564)),
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(1,0)):JPGTAG_TAG_IGNORE,TO_FIX(0.3575761)),
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(2,0)):JPGTAG_TAG_IGNORE,TO_FIX(0.1804375)),
            
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(0,1)):JPGTAG_TAG_IGNORE,TO_FIX(0.2126729)),
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(1,1)):JPGTAG_TAG_IGNORE,TO_FIX(0.7151522)),
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(2,1)):JPGTAG_TAG_IGNORE,TO_FIX(0.0721750)),
            
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(0,2)):JPGTAG_TAG_IGNORE,TO_FIX(0.0193339)),
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(1,2)):JPGTAG_TAG_IGNORE,TO_FIX(0.1191920)),
            JPG_ValueTag((cxyz)?(JPGTAG_MATRIX_CMATRIX(2,2)):JPGTAG_TAG_IGNORE,TO_FIX(0.9503041)),
            
            JPG_PointerTag((tonemapping)?(JPGTAG_TONEMAPPING_L_LUT(0)):JPGTAG_TAG_IGNORE,
                           const_cast<UWORD *>(separate?red:tonemapping)),
            JPG_PointerTag((tonemapping && depth > 1)?(JPGTAG_TONEMAPPING_L_LUT(1)):JPGTAG_TAG_IGNORE,
                           const_cast<UWORD *>(separate?green:tonemapping)),
            JPG_PointerTag((tonemapping && depth > 2)?(JPGTAG_TONEMAPPING_L_LUT(2)):JPGTAG_TAG_IGNORE,
                           const_cast<UWORD *>(separate?blue:tonemapping)),
            JPG_ValueTag((tonemapping)?(JPGTAG_TONEMAPPING_L_TYPE(0)):JPGTAG_TAG_IGNORE,
                         JPGFLAG_TONEMAPPING_LUT),
            JPG_ValueTag((tonemapping && depth > 1)?(JPGTAG_TONEMAPPING_L_TYPE(1)):JPGTAG_TAG_IGNORE,
                         JPGFLAG_TONEMAPPING_LUT),
            JPG_ValueTag((tonemapping && depth > 2)?(JPGTAG_TONEMAPPING_L_TYPE(2)):JPGTAG_TAG_IGNORE,
                         JPGFLAG_TONEMAPPING_LUT),
            JPG_ValueTag((fullrange)?(JPGTAG_TONEMAPPING_R2_TYPE(0)):JPGTAG_TAG_IGNORE,
                         JPGFLAG_TONEMAPPING_LINEAR),
            JPG_ValueTag((fullrange)?(JPGTAG_TONEMAPPING_R2_TYPE(1)):JPGTAG_TAG_IGNORE,
                         JPGFLAG_TONEMAPPING_LINEAR),
            JPG_ValueTag((fullrange)?(JPGTAG_TONEMAPPING_R2_TYPE(2)):JPGTAG_TAG_IGNORE,
                         JPGFLAG_TONEMAPPING_LINEAR), 
            // The default settings for R2 in profile C are quite ok.
            JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(qscan)?(qscan1):(pscan1)),
            JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(qscan)?(qscan2):(pscan2)),
            JPG_PointerTag((progressive)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,(qscan)?(qscan3):(pscan3)),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan4),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan5),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan6),
            JPG_PointerTag((progressive && !qscan)?JPGTAG_IMAGE_SCAN:JPGTAG_TAG_IGNORE,pscan7),
            
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan1:pscan1),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan2:pscan2),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan3:pscan3),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan4:pscan4),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan5:pscan5),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan6:pscan6),
            JPG_PointerTag((rprogressive)?JPGTAG_RESIDUAL_SCAN:JPGTAG_TAG_IGNORE,
                           ((residualtype & 7) == JPGFLAG_RESIDUALPROGRESSIVE)?rscan7:pscan7),
            JPG_ValueTag((lsmode >= 0)?JPGTAG_SCAN_LS_INTERLEAVING:JPGTAG_TAG_IGNORE,lsmode),
            JPG_ValueTag(JPGTAG_IMAGE_IS_FLOAT,flt),
            JPG_ValueTag(JPGTAG_IMAGE_OUTPUT_CONVERSION,flt),
            JPG_PointerTag(alphain?JPGTAG_ALPHA_TAGLIST:JPGTAG_TAG_IGNORE,alphatags),
            JPG_EndTag
          };
          
          class JPEG *jpeg = JPEG::Construct(NULL);
          if (jpeg) {
            UBYTE bytesperpixel = sizeof(UBYTE);
            UBYTE pixeltype     = CTYP_UBYTE;
            if (prec > 8) {
              bytesperpixel = sizeof(UWORD);
              pixeltype     = CTYP_UWORD;
            }
            
            UBYTE *mem      = (UBYTE *)malloc(width * 8 * depth * bytesperpixel + width * 8 * depth);
            UBYTE *alphamem = NULL;
            if (mem) {
              bmm.bmm_pMemPtr      = mem + width * 8 * depth;
              bmm.bmm_pLDRMemPtr   = ldrin?mem:NULL;
              bmm.bmm_ulWidth      = width;
              bmm.bmm_ulHeight     = height;
              bmm.bmm_usDepth      = depth;
              bmm.bmm_ucPixelType  = pixeltype;
              bmm.bmm_pTarget      = NULL;
              bmm.bmm_pAlphaPtr    = NULL;
              bmm.bmm_pAlphaTarget = NULL;
              bmm.bmm_pAlphaSource = NULL;
              bmm.bmm_pSource      = in;
              bmm.bmm_pAlphaSource = NULL;
              bmm.bmm_pAlphaTarget = NULL;
              bmm.bmm_pLDRSource   = ldrin;
              bmm.bmm_bFloat       = flt;
              bmm.bmm_bBigEndian   = big;
              bmm.bmm_HDR2LDR      = hdrtoldr;
              bmm.bmm_bNoOutputConversion = false;
              bmm.bmm_bClamp       = !noclamp;
              bmm.bmm_bWritePGX    = false;
              bmm.bmm_bUpsampling  = true;
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
                
                if (ok) {
                  struct JPG_Hook filehook(FileHook,out);
                  struct JPG_TagItem iotags[] = {
                    JPG_PointerTag(JPGTAG_HOOK_IOHOOK,&filehook),
                    JPG_PointerTag(JPGTAG_HOOK_IOSTREAM,out),
#ifdef TEST_MARKER_INJECTION                
                    // Stop after the image header...
                    JPG_ValueTag(JPGTAG_ENCODER_STOP,JPGFLAG_ENCODER_STOP_FRAME),
#endif              
                    JPG_EndTag
                  };
                  
                  //
                  // Write in one go, could interrupt this on each frame,scan,line or MCU.
                  ok = jpeg->Write(iotags);
#ifdef TEST_MARKER_INJECTION              
                  //
                  if (ok) {
                    // Now write a custom marker. This is just a dummy for testing.
                    UBYTE marker[] = {
                      0xff,0xe9, // APP9
                      0x00,0x08, // size of the segment, not including the header
                      'D','u',
                      'm','m',
                      'y',0
                    };
                    ok = (jpeg->WriteMarker(marker,sizeof(marker),NULL) == sizeof(marker));
                    // Continue now with the rest of the data.
                    iotags->SetTagData(JPGTAG_ENCODER_STOP,0);
                  }               
                  //
                  // Continue writing the image.
                  if (ok)
                    ok = jpeg->Write(iotags);
#endif            
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
              fprintf(stderr,"failed allocating a buffer for the image\n");
            }
            JPEG::Destruct(jpeg);
          } else {
            fprintf(stderr,"failed to create a JPEG object\n");
          }
        }
        fclose(out);
      } else {
        perror("unable to open the output file");
      }
      if (alphain)
        fclose(alphain);
      if (ldrin)
        fclose(ldrin);
      fclose(in);
    }
  }
}
///

/// SplitQualityC
// Provide a useful default for splitting the quality between LDR and HDR.
void SplitQualityC(int splitquality,bool residuals,int &quality,int &hdrquality)
{
  if (residuals) {
    hdrquality = splitquality;
    if (hdrquality < 5) {
      quality    = hdrquality;
      hdrquality = 0;
    } else {
      hdrquality = 5 + 0.25 * (splitquality - 5);
      quality    = 0.75 * (splitquality - 5);
      if (quality > 90) {
        quality    = 90;
        hdrquality = splitquality - 90;
      }
    }
    if (hdrquality > 100) {
      quality += hdrquality - 100;
      if (quality > 100)
        quality = 100;
      hdrquality  = 100;
    }
  } else {
    hdrquality = 0;
    quality    = splitquality;
    if (quality > 100)
      quality = 100;
  }
}
///
