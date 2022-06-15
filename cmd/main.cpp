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
** This header provides the main function.
** This main function is only a demo, it is not part of the libjpeg code.
** It is here to serve as an entry point for the command line image
** compressor.
**
** $Id: main.cpp,v 1.220 2022/06/14 06:18:30 thor Exp $
**
*/

/// Includes
#include "cmd/main.hpp"
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
#include "tools/numerics.hpp"
#include "cmd/bitmaphook.hpp"
#include "cmd/filehook.hpp"
#include "cmd/iohelpers.hpp"
#include "cmd/tmo.hpp"
#include "cmd/defaulttmoc.hpp"
#include "cmd/encodec.hpp"
#include "cmd/encodeb.hpp"
#include "cmd/encodea.hpp"
#include "cmd/reconstruct.hpp"
///

bool oznew = false;

/// Defines
#define FIX_BITS 13
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

/// ParseDouble
// Parse off a floating point value, or print an error
double ParseDouble(int &argc,char **&argv)
{
  double v;
  char *endptr;
  
  if (argv[2] == NULL) {
    fprintf(stderr,"%s expected a numeric argument.\n",argv[1]);
    exit(25);
  }

  v = strtod(argv[2],&endptr);

  if (*endptr) {
    fprintf(stderr,"%s expected a numeric argument, not %s.\n",argv[1],argv[2]);
    exit(25);
  }

  argc -= 2;
  argv += 2;

  return v;
}
///

/// ParseInt
// Parse off an integer, return it, or print an error.
int ParseInt(int &argc,char **&argv)
{
  long int v;
  char *endptr;
  
  if (argv[2] == NULL) {
    fprintf(stderr,"%s expected a numeric argument.\n",argv[1]);
    exit(25);
  }

  v = strtol(argv[2],&endptr,0);

  if (*endptr) {
    fprintf(stderr,"%s expected a numeric argument, not %s.\n",argv[1],argv[2]);
    exit(25);
  }

  argc -= 2;
  argv += 2;

  return v;
}
///

/// ParseString
// Parse of a string argument, return it, or print an error
const char *ParseString(int &argc,char **&argv)
{
  const char *v;
  
  if (argv[2] == NULL) {
    fprintf(stderr,"%s expects a string argument.\n",argv[1]);
    exit(25);
  }

  v = argv[2];

  argc -= 2;
  argv += 2;

  return v;
}
///

/// PrintLicense
// Print out the license of the codec.
void PrintLicense(void)
{

  printf(""
         "jpeg Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart\n"
         "and Accusoft\n\n"
         "For license conditions, see README.license for details.\n\n"
         );
}
///

/// PrintUsage
// Print the usage of the program and exit
void PrintUsage(const char *progname)
{    
  printf("Usage: %s [options] source target\n"
          "default is to decode the jpeg input and write a ppm output\n"
          "use -q [1..100] or -p to enforce encoding\n\n"
          "-q quality : selects the encoding mode and defines the quality of the base image\n"
          "-Q quality : defines the quality for the extension layer\n"
          "-quality q : use a profile and part specific weighting between base and extension\n"
          "             layer quality\n"
          "-r         : enable the residual codestream for HDR and lossless\n"
          "             coding, requires -q and -Q to define base and\n"
          "             enhancement layer quality.\n"
          "-r12       : use a 12 bit residual image instead of an 8 bit residual\n"
          "             image.\n"
          "-rl        : enforce a int-to-int lossless DCT in the residual domain\n"
          "             for lossless coding enabled by -Q 100\n"
          "-ro        : disable the DCT in the residual domain, quantize spatially for\n"
          "             near-lossless coding\n"
          "-ldr file  : specifies a separate file containing the base layer\n"
          "             for encoding.\n"
          "-R bits    : specify refinement bits for the base images.\n"
          "             This works like -r but in the DCT domain.\n"
          "-rR bits   : specify refinement bits for the residual image.\n"
          "-N         : enable noise shaping of the prediction residual\n"
          "-U         : disable automatic upsampling\n"
          "-l         : enable lossless coding without a residual image by an\n"
          "             int-to-int DCT, also requires -c and -q 100 for true lossless\n"
#if ACCUSOFT_CODE
          "-p         : JPEG lossless (predictive) mode\n"
          "             also requires -c for true lossless\n"
#endif
          "-c         : disable the RGB to YCbCr decorrelation transformation\n"
          "-xyz       : indicates that the HDR image is in the XYZ colorspace\n"
          "             note that the image is not *converted* to this space, but\n"
          "             is assumed to be encoded in this space.\n"
          "-cxyz      : similar to the above, but uses the dedicated C transformation\n"
          "             to implement a XYZ colorspace conversion.\n"
          "-sp        : use separate LUTs for each component.\n"
          "-md        : use the median instead of the center of mass\n"
          "             for constructing the inverse TMO of ISO/IEC 18477-7 profile C.\n"
          "-ct        : use the center of mass instead of the median\n"
          "             for constructing the inverse TMO of ISO/IEC 18477-7 profile C.\n"
          "-sm iter   : use <iter> iterations to smooth out the histogram for\n"
          "             inverse-TMO based algorithms. Default is not to smooth\n"
          "             the histogram.\n"
          "-ncl       : disable clamping of out-of-gamut colors.\n"
          "             this is automatically enabled for lossless.\n"
#if ACCUSOFT_CODE
          "-m maxerr  : defines a maximum pixel error for JPEG LS coding\n"
#endif
          "-h         : optimize the Huffman tables\n"
#if ACCUSOFT_CODE
          "-a         : use arithmetic coding instead of Huffman coding\n"
          "             available for all coding schemes (-p,-v,-l and default)\n"
#endif
          "-bl        : force encoding in the baseline process, default is extended sequential\n"
          "-v         : use progressive instead of sequential encoding\n"
          "             available for all coding schemes (-r,-a,-l and default)\n"
          "-qv        : use a simplified scan pattern for progressive that only\n"
          "             separates AC from DC bands and may improve the performance\n"
#if ACCUSOFT_CODE
          "-d         : encode the DC band only (requires -p)\n"
#endif
#if ACCUSOFT_CODE
          "-y levels  : hierarchical JPEG coding with the given number of decomposition\n"
          "             levels. If levels is zero, then a lossless coding mode for\n"
          "             hierarchical is used in which the second lossless scan encodes\n"
          "             the DCT residuals of the first scan. For that, -c is suggested\n"
          "             for true lossless. If levels is one, then the lossy initial scan\n"
          "             is downscaled by a power of two.\n"
#endif
          "-g gamma   : define the exponent for the gamma for the LDR domain, or rather, for\n"
          "             mapping HDR to LDR. A suggested value is 2.4 for mapping scRGB to sRBG.\n"
          "             This option controls the base-nonlinearity that generates the\n"
          "             HDR pre-cursor image from the LDR image. It is also used in the\n"
          "             absence of -ldr (i.e. no LDR image) to tonemap the HDR input image.\n"
          "             Use -g 0 to use an approximate inverse TMO as base-nonlinearity, and\n"
          "             for tonemapping with the Reinhard operator if the LDR image is missing.\n"
          "-gf file   : define the inverse one-point L-nonlinearity on decoding from a file\n"
          "             this file contains one (ASCII encoded) digit per line, 256*2^h lines\n"
          "             in total, where h is the number of refinement bits. Each line contains\n"
          "             an (integer) output value the corresponding input is mapped to.\n"
          "-z mcus    : define the restart interval size, zero disables it\n"
#if ACCUSOFT_CODE
          "-n         : indicate the image height by a DNL marker\n"
#endif
          "-s WxH,... : define subsampling factors for all components\n"
          "             note that these are NOT MCU sizes\n"
          "             Default is 1x1,1x1,1x1 (444 subsampling)\n"
          "             1x1,2x2,2x2 is the 420 subsampling often used\n"
          "-sr WxH,...: define subsampling in the residual domain\n"
          "-rs        : encode the residual image in sequential (rather than the modified residual)\n"
          "             coding mode\n"
          "-rv        : encode the residual image in progressive coding mode\n"
          "-ol        : open loop encoding, residuals are based on original, not reconstructed\n"
          "-dz        : improved deadzone quantizer, may help to improve the R/D performance\n"
#if ACCUSOFT_CODE 
          "-oz        : optimize quantizer, may help to improve the R/D performance\n"
          "-dr        : include the optional de-ringing (Gibbs Phenomenon) filter on encoding\n"
#endif   
          "-qt n      : define the quantization table. The following tables are currently defined:\n"
          "             n = 0 the example tables from Rec. ITU-T T.81 | ISO/IEC 10918-1 (default)\n"
          "             n = 1 a completely flat table that should be PSNR-optimal\n"
          "             n = 2 a MS-SSIM optimized table\n"
          "             n = 3 the table suggested by ImageMagick\n"
          "             n = 4 a HSV-PSNR optimized table\n"
          "             n = 5 the table from Klein, Silverstein and Carney:\n"
          "                   Relevance of human vision to JPEG-DCT compression (1992)\n"
          "             n = 6 the table from Watson, Taylor, Borthwick:\n"
          "                   DCTune perceptual optimization of compressed dental X-Rays (1997)\n"
          "             n = 7 the table from Ahumada, Watson, Peterson:\n"
          "                   A visual detection model for DCT coefficient quantization (1993)\n"
          "             n = 8 the table from Peterson, Ahumada and Watson:\n"
          "                   An improved detection model for DCT coefficient quantization (1993)\n"
          "-qtf file  : read the quantization steps from a file, 64*2 integers (luma & chroma)\n"
          "-rqt n     : defines the quantization table for the residual stream in the same way\n"
          "-rqtf file : read the residual quantization steps from a file\n"
          "-al file   : specifies a one-component pgm/pfm file that contains an alpha component\n"
          "             or the code will write the alpha component to.\n"
          "             This demo code DOES NOT implement compositing of alpha and background\n"
          "-am mode   : specifes the mode of the alpha: 1 (regular) 2 (premultiplied) 3 (matte-removal)\n"
          "-ab r,g,b  : specifies the matte (background) color for mode 3 as RGB triple\n"
          "-ar        : enable residual coding for the alpha channel, required if the\n"
          "             alpha channel is larger than 8bpp\n"
          "-ar12      : use a 12 bit residual for the alpha channel\n"
          "-aR bits   : set refinement bits in the alpha base codestream\n"
          "-arR bits  : set refinement bits in the residual alpha codestream\n"
          "-aol       : enable open loop coding for the alpha channel\n"
          "-adz       : enable the deadzone quantizer for the alpha channel\n"
#if ACCUSOFT_CODE        
          "-aoz       : enable the quantization optimization for the alpha channel\n"
          "-adr       : include the de-ringing filter for the alpha channel\n"
#endif   
          "-all       : enable lossless DCT for alpha coding\n"
          "-alo       : disable the DCT in the residual alpha channel, quantize spatially.\n"
          "-aq qu     : specify a quality for the alpha base channel (usually the only one)\n"
          "-aQ qu     : specify a quality for the alpha extension layer\n"
          "-aqt n     : specify the quantization table for the alpha channel\n"
          "-aqtf file : read the alpha quantization tables from a file\n"
          "-arqt n    : specify the quantization table for residual alpha\n"
          "-arqtf file: read the residual alpha quantization tables from a file\n"
          "-aquality q: specify a combined quality for both\n"
#if ACCUSOFT_CODE
          "-ra        : enable arithmetic coding for residual image (*NOT SPECIFIED*)\n"
          "-ls mode   : encode in JPEG LS mode, where 0 is scan-interleaved,\n"
          "             1 is line interleaved and 2 is sample interleaved.\n"
          "             NOTE THAT THIS IS NOT CONFORMING TO REC. ITU-T T.81 | ISO/IEC 10918 BUT\n"
          "             COMPLIANT TO REC. ITU-T T.87 | ISO/IEC 14495-1 (JPEG-LS) WHICH IS A\n"
          "             DIFFERENT STANDARD.\n"
          "             Use -c to bypass the YCbCr color transformation for true lossless,\n"
          "             also use -c for decoding images encoded by the UBC reference software\n"
          "             as it does not write an indicator marker to disable the\n"
          "             transformation itself.\n"
          "             Note that the UBC implementation will not able to decode streams created by\n"
          "             this software due to a limitation of the UBC code - the streams are\n"
          "             nevertheless fully conforming.\n"
          "-cls       : Use a JPEG LS part-2 conforming pseudo-RCT color transformation.\n"
          "             Note that this transformation is only CONFORMING TO\n"
          "             REC. ITU-T T.870 | ISO/IEC 14495-2 AND NOT CONFORMING TO\n"
          "             REC. ITU-T T.81 | ISO/IEC 10918-1. Works for near-lossless JPEG LS\n"
          "             DO NOT USE FOR LOSSY JPEG, it will also create artifacts.\n"
#endif
          ,progname);
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
  int riddenbits    = 0;  // hidden bits in the residual domain
  int ahiddenbits   = 0;  // hidden DCT bits in the base alpha codestream
  int ariddenbits   = 0;  // hidden DCT bits in the residual alpha codestream.
  int resprec       = 8;  // precision in the residual domain
  int aresprec      = 8;  // precision of the residual alpha
  double gamma      = 0.0;
  bool pyramidal    = false;
  bool residuals    = false;
  int  colortrafo   = JPGFLAG_MATRIX_COLORTRANSFORMATION_YCBCR;
  bool baseline     = false;
  bool lossless     = false;
  bool optimize     = false;
  bool accoding     = false;
  bool qscan        = false;
  bool progressive  = false;
  bool writednl     = false;
  bool noiseshaping = false;
  bool rprogressive = false;
  bool rsequential  = false;
  bool raccoding    = false;
  bool serms        = false;
  bool aserms       = false;
  bool abypass      = false;
  bool losslessdct  = false;
  bool dctbypass    = false;
  bool openloop     = false;
  bool deadzone     = false;
  bool lagrangian   = false;
  bool dering       = false;
  bool aopenloop    = false;
  bool adeadzone    = false;
  bool alagrangian  = false;
  bool adering      = false;
  bool xyz          = false;
  bool cxyz         = false;
  bool separate     = false;
  bool noclamp      = false;
  bool setprofile   = false;
  bool upsample     = true;
  bool median       = true;
  int splitquality  = -1;
  int profile       = 2;    // profile C.
  const char *sub       = NULL;
  const char *ressub    = NULL;
  const char *ldrsource = NULL;
  const char *lsource   = NULL;
  const char *alpha     = NULL; // source or target of the alpha plane 
  bool alpharesiduals   = false;
  int alphamode         = JPGFLAG_ALPHA_REGULAR; // alpha mode
  int matte_r = 0,matte_g = 0,matte_b = 0; // matte color for alpha.
  int alphaquality      = 70;
  int alphahdrquality   = 0;
  int alphasplitquality = -1;
  int tabletype         = 0; // quantization table types
  int residualtt        = 0;
  int alphatt           = 0;
  int residualalphatt   = 0;
  int smooth            = 0; // histogram smoothing
  const char *quantsteps         = NULL;
  const char *residualquantsteps = NULL;
  const char *alphasteps         = NULL;
  const char *residualalphasteps = NULL;
  PrintLicense();
  fflush(stdout);

  while(argc > 3 && argv[1][0] == '-') {
    if (!strcmp(argv[1],"-q")) {
      quality = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-Q")) {
      hdrquality = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-quality")) {
      splitquality = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-profile")) {
      const char *s = ParseString(argc,argv);
      setprofile    = true;
      if (!strcmp(s,"a") || !strcmp(s,"A")) {
        profile = 0;
      } else if (!strcmp(s,"b") || !strcmp(s,"B")) {
        profile = 1;
      } else if (!strcmp(s,"c") || !strcmp(s,"C")) {
        profile = 2;
      } else if (!strcmp(s,"d") || !strcmp(s,"D")) {
        profile = 4;
      } else {
        fprintf(stderr,"unknown profile definition %s, only profiles a,b,c and d exist",
                s);
        return 20;
      }
    } else if (!strcmp(argv[1],"-m")) {
      maxerror = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-md")) {
      median = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-ct")) {
      median = false;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-sm")) {
      smooth = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-z")) {
      restart = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-r")) {
      residuals = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-R")) {
      hiddenbits = ParseInt(argc,argv);
      if (hiddenbits < 0 || hiddenbits > 4) {
        fprintf(stderr,"JPEG XT allows only between 0 and 4 refinement bits.\n");
        return 20;
      }
    } else if (!strcmp(argv[1],"-rR")) {
      riddenbits = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-n")) {
      writednl   = true;
      argv++;
      argc--;
    } 
    else if (!strcmp(argv[1],"-c")) {
      colortrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-cls")) {
      colortrafo = JPGFLAG_MATRIX_COLORTRANSFORMATION_LSRCT;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-sp")) {
      separate = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-s")) {
      sub    = ParseString(argc,argv);
    } else if (!strcmp(argv[1],"-sr")) {
      ressub = ParseString(argc,argv);
    } else if (!strcmp(argv[1],"-ncl")) {
      noclamp = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-al")) {
      alpha   = ParseString(argc,argv);
    } else if (!strcmp(argv[1],"-am")) {
      alphamode = ParseInt(argc,argv);
      if (alphamode < 0 || alphamode > 3) {
        fprintf(stderr,"the alpha mode specified with -am must be between 0 and 3\n");
        return 20;
      }
    } else if (!strcmp(argv[1],"-ab")) {
      const char *matte = ParseString(argc,argv);
      if (sscanf(matte,"%d,%d,%d",&matte_r,&matte_g,&matte_b) != 3) {
        fprintf(stderr,"-ab expects three numeric arguments separated comma, i.e. r,g,b\n");
        return 20;
      }
    } else if (!strcmp(argv[1],"-all")) {
      aserms = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-alo")) {
      abypass = true;
      argv++;
      argc--;
    }
#if ACCUSOFT_CODE
    else if (!strcmp(argv[1],"-p")) {
      lossless = true;
      argv++;
      argc--;
    } 
#endif
    else if (!strcmp(argv[1],"-h")) {
      optimize = true;
      argv++;
      argc--;
    } 
#if ACCUSOFT_CODE
    else if (!strcmp(argv[1],"-a")) {
      accoding = true; 
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-ra")) {
      raccoding = true;
      argv++;
      argc--;
    }
#endif
    else if (!strcmp(argv[1],"-qv")) {
      qscan       = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-bl")) {
      baseline    = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-v")) {
      progressive = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-rv")) {
      rprogressive = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-rs")) {
      rsequential = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-r12")) {
      resprec   = 12;
      residuals = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-rl")) {
      losslessdct = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-ro")) {
      dctbypass = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-xyz")) {
      xyz = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-cxyz")) {
      cxyz = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-N")) {
      noiseshaping = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-ol")) {
      openloop = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-U")) {
      upsample = false;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-dz")) {
      deadzone = true;
      argv++;
      argc--;
#if ACCUSOFT_CODE
    } else if (!strcmp(argv[1],"-oz")) {
      lagrangian = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-ozn")) {
      oznew = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-dr")) {
      dering = true;
      argv++;
      argc--;
#endif      
    } else if (!strcmp(argv[1],"-qt")) {
      tabletype = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-qtf")) {
      quantsteps = argv[2];
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-rqt")) {
      residualtt = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-rqtf")) {
      residualquantsteps = argv[2];
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-aqt")) {
      alphatt = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-aqtf")) {
      alphasteps = argv[2];
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-arqt")) {
      residualalphatt = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-arqtf")) {
      residualalphasteps = argv[2];
      argv += 2;
      argc -= 2;
    } else if (!strcmp(argv[1],"-aol")) {
      aopenloop = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-adz")) {
      adeadzone = true;
      argv++;
      argc--;
#if ACCUSOFT_CODE
    } else if (!strcmp(argv[1],"-aoz")) {
      alagrangian = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-adr")) {
      adering = true;
      argv++;
      argc--;
#endif      
    } else if (!strcmp(argv[1],"-ldr")) {
      ldrsource = ParseString(argc,argv);
    } else if (!strcmp(argv[1],"-l")) {
      serms = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-g")) {
      gamma = ParseDouble(argc,argv);
    } else if (!strcmp(argv[1],"-gf")) {
      lsource = ParseString(argc,argv);
    } else if (!strcmp(argv[1],"-aq")) {
      alphaquality = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-aQ")) {
      alphahdrquality = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-aquality")) {
      alphasplitquality = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-ar")) {
      alpharesiduals = true;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-ar12")) {
      alpharesiduals = true;
      aresprec = 12;
      argv++;
      argc--;
    } else if (!strcmp(argv[1],"-aR")) {
      ahiddenbits = ParseInt(argc,argv);
    } else if (!strcmp(argv[1],"-arR")) {
      ariddenbits = ParseInt(argc,argv);
    }
#if ACCUSOFT_CODE
    else if (!strcmp(argv[1],"-y")) {
      levels = ParseInt(argc,argv);
      if (levels == 0 || levels == 1) {
        // In this mode, the hierarchical model is used for lossless coding
        levels++;
        pyramidal = false;
      } else {
        pyramidal = true;
      }
    } 
#endif
    else if (!strcmp(argv[1],"-ls")) {
      lsmode = ParseInt(argc,argv);
    } else {
      fprintf(stderr,"unsupported command line switch %s\n",argv[1]);
      return 20;
    }
  }

  //
  // Use a very simplistic quality split.
  if (splitquality >= 0) {
    switch(profile) {
    case 0:
      break;
    case 1:
      break;
    case 2:
    case 4:
      SplitQualityC(splitquality,residuals,quality,hdrquality);
      break;
    }
  }

  //
  // The alpha channel is encoded with something that works like part 6.
  if (alphasplitquality > 0) {
    SplitQualityC(alphasplitquality,alpharesiduals,alphaquality,alphahdrquality);
  }

  if (argc != 3) {
    if (argc > 3) {
      fprintf(stderr,"Error in argument parsing, argument %s not understood or parsed correctly.\n"
              "Run without arguments for a list of command line options.\n\n",
              argv[1]);
      exit(20);
    }

    PrintUsage(argv[0]);
    
    return 5;
  }

  if (quality < 0 && lossless == false && lsmode < 0) {
    Reconstruct(argv[1],argv[2],colortrafo,alpha,upsample);
  } else {
    switch(profile) {
    case 0:
      fprintf(stderr,"**** Profile A encoding not supported due to patented IPRs.\n");
      break;
    case 1:
      fprintf(stderr,"**** Profile B encoding not supported due to patented IPRs.\n");
      break;
    case 2:
    case 4:
      if (setprofile && ((residuals == false && hiddenbits == false && profile != 4) || profile == 2))
        residuals = true;
      EncodeC(argv[1],ldrsource,argv[2],lsource,quality,hdrquality,
              tabletype,residualtt,maxerror,
              colortrafo,baseline,lossless,progressive,
              residuals,optimize,accoding,
              rsequential,rprogressive,raccoding,
              qscan,levels,pyramidal,writednl,restart,
              gamma,
              lsmode,noiseshaping,serms,losslessdct,
              openloop,deadzone,lagrangian,dering,
              xyz,cxyz,
              hiddenbits,riddenbits,resprec,separate,
              median,noclamp,smooth,dctbypass,
              sub,ressub,
              alpha,alphamode,matte_r,matte_g,matte_b,
              alpharesiduals,alphaquality,alphahdrquality,
              alphatt,residualalphatt,
              ahiddenbits,ariddenbits,aresprec,
              aopenloop,adeadzone,alagrangian,adering,
              aserms,abypass,
              quantsteps,residualquantsteps,
              alphasteps,residualalphasteps);
      break;
    }
  }
  
  return 0;
}
///

