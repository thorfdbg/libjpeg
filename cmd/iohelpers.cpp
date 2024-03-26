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
** Several helper functions that are related to native IO of pixel values
**
** $Id: iohelpers.cpp,v 1.12 2024/03/25 18:42:33 thor Exp $
**
*/

/// Includes
#include "cmd/iohelpers.hpp"
#include "cmd/tmo.hpp"
#include "std/stdio.hpp"
#include "std/math.hpp"
#include "std/stdlib.hpp"
///

/// ReadRGBTriple
// Read an RGB triple from the stream, convert properly.
bool ReadRGBTriple(FILE *in,int &r,int &g,int &b,double &y,int depth,int count,
                   bool flt,bool bigendian,bool xyz)
{ 
  bool warn = false;
  
  // Read the HDR image parameters.
  if (count == 3) {
    if (flt) { 
      double rf,gf,bf;
      if (xyz) {
        double xf,yf,zf;
        // Convert from XYZ to RGB (the same colorspace as the LDR)
        xf = readFloat(in,bigendian);
        yf = readFloat(in,bigendian);
        zf = readFloat(in,bigendian); 
        if (xf < 0.0) xf = 0.0, warn = true;
        if (yf < 0.0) yf = 0.0, warn = true;
        if (zf < 0.0) zf = 0.0, warn = true;
        //
        if (isnan(zf)) {
          fprintf(stderr,"Error reading the source file\n");
          exit(20);
        }
        //
        // Convert from XYZ to RGB (the same colorspace as the LDR)
        rf = xf *  3.2404542 + yf * -1.5371385 + zf * -0.4985314;
        gf = xf * -0.9692660 + yf *  1.8760108 + zf *  0.0415560;
        bf = xf *  0.0556434 + yf * -0.2040259 + zf *  1.0570000;
      } else { 
        rf = readFloat(in,bigendian);
        gf = readFloat(in,bigendian);
        bf = readFloat(in,bigendian);
        //
        if (rf < 0.0) rf = 0.0, warn = true;
        if (gf < 0.0) gf = 0.0, warn = true;
        if (bf < 0.0) bf = 0.0, warn = true;
        if (isnan(bf)) {
          fprintf(stderr,"Error reading the source file\n");
          exit(20);
        }
      }
      y  = (0.2126 * rf + 0.7152 * gf + 0.0722 * bf);
      r  = DoubleToHalf(rf);
      g  = DoubleToHalf(gf);
      b  = DoubleToHalf(bf);
    } else {
      int max = (1l << depth) - 1;
      // Integer samples, three components
      if (depth <= 8) {
        r = getc(in);
        g = getc(in);
        b = getc(in);
      } else {
        r  = getc(in) << 8;
        r |= getc(in);
        g  = getc(in) << 8;
        g |= getc(in);
        b  = getc(in) << 8;
        b |= getc(in);
      }
      if (b < 0) {
        fprintf(stderr,"Error reading the source file\n");
        exit(20);
      }
      y  = (0.2126 * r + 0.7152 * g + 0.0722 * b) / max;
      if (xyz) {
        double rf,gf,bf;
        double xf = r,yf = g,zf = b;
        // Convert from XYZ to RGB (the same colorspace as the LDR)
        rf = xf *  3.2404542 + yf * -1.5371385 + zf * -0.4985314;
        gf = xf * -0.9692660 + yf *  1.8760108 + zf *  0.0415560;
        bf = xf *  0.0556434 + yf * -0.2040259 + zf *  1.0570000;
        r = int(rf),g = int(gf),b = int(bf);
        if (r < 0.0) r = 0, warn = true;
        if (g < 0.0) g = 0, warn = true;
        if (b < 0.0) b = 0, warn = true;
        if (r > max) r = max, warn = true;
        if (g > max) g = max, warn = true;
        if (b > max) b = max, warn = true;
        y  = (0.2126 * rf + 0.7152 * gf + 0.0722 * bf) / max;
      }
    }
  } else {
    if (flt) {
      double gf;
      gf = readFloat(in,bigendian);
      if (gf < 0.0) gf = 0.0, warn = true;
      g  = DoubleToHalf(gf);
      y  = gf;
    } else {
      if (depth <= 8) {
        g  = getc(in);
      } else {
        g  = getc(in) << 8;
        g |= getc(in);
      }
      y = double(g) / ((1L << depth) - 1);
    }
    r = g;
    b = g;
  } 

  return warn;
}
///

/// OpenPNMFile
// Open a PPM/PFM file and return its dimensions and properties.
FILE *OpenPNMFile(const char *file,int &width,int &height,int &depth,int &precision,bool &isfloat,bool &bigendian)
{
  FILE *in = fopen(file,"rb");

  if (in) {
    char id,type;
    int max;
    
    isfloat   = false;
    bigendian = false;
    if (fscanf(in,"%c%c\n",&id,&type) == 2) {
      if (id == 'P' && (type == '5' || type == '6' || type == 'f' || type == 'F')) {
        if (type == '5' || type == 'f') {
          depth = 1;
        } else {
          depth = 3; 
        }
        // Identify pfm one or three component images.
        if (type == 'f' || type == 'F') {
          isfloat   = true;
        }
        while((id = getc(in)) == '#') {
          char buffer[256];
          fgets(buffer,sizeof(buffer),in);
        }
        ungetc(id,in);
        int parms;
        if (isfloat) {
          double scale = 1.0;
          parms = fscanf(in,"%d %d %lg%*c",&width,&height,&scale);
          if (parms == 3) {
            if (scale < 0.0) { 
              // is little-endian
              bigendian = false;
            } else {
              bigendian = true;
            }
            precision = 16;
          }
        } else {
          parms = fscanf(in,"%d %d %d%*c",&width,&height,&max);
          if (parms == 3) {
            precision = 0;
            while((1 << precision) < max)
              precision++;
          }
        }
        if (precision <= 16 && width > 0 && height > 0 && parms == 3) {
          return in;
        }
        fprintf(stderr,"unsupported or invalid PNM format\n");
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
  
  return NULL;
}
///

/// PrepareAlphaForRead
// Prepare the alpha component for reading, return a file in case it was
// opened successfully
FILE *PrepareAlphaForRead(const char *alpha,int width,int height,int &prec,bool &flt,bool &big,
                          bool alpharesidual,int &hiddenbits,
                          UWORD ldrtohdr[65536])
{
  int alphawidth,alphaheight,alphadepth;
  
  FILE *in = OpenPNMFile(alpha,alphawidth,alphaheight,alphadepth,prec,flt,big);
  
  if (in) {
    if (alphawidth != width || alphaheight != height) {
      fprintf(stderr,"The dimensions of the alpha channel in %s alpha do not match the image dimensions.\n",alpha);
      fclose(in);
      return NULL;
    } else if (alphadepth != 1) {
      fprintf(stderr,"The alpha channel in %s must have a depth of one component.\n",alpha);
      fclose(in);
      return NULL;
    }
    if (prec < 8) {
      fprintf(stderr,"The precision of the alpha channel in %s must be at least 8 bits.\n",alpha);
    }
    // Do we need to allocate residual bits or should we go for refinement scans?
    if (prec > 8) {
      if (alpharesidual) {
        // Yes, we have residual scans here.
        // Need to build a lookup table. Make it linear. There is no need for gamma as the
        // image is never being looked at.
        BuildGammaMapping(1.0,1.0,ldrtohdr,flt,(1 << prec) - 1,hiddenbits);
      } else {
        // No residual, use refinement scans. Compute their number. There can be at most four.
        if (hiddenbits != prec - 8) {
          fprintf(stderr,
                  "alpha channel data precision does not match the frame precision.\n"
                  "Please either enable residual coding with -ar or enable refinement\n"
                  "coding with -aR %d. This only works for channel precisions up to 12 bits\n",
                  prec-8);
          fclose(in);
          in = NULL;
        }
        //
        if (hiddenbits > 4) {
          fprintf(stderr,
                  "Alpha channel precision is too large, can have at most four refinement scans, i.e.\n"
                  "the maximum alpha precision is 12. Try to enable residual alpha coding with -ar.\n");
          fclose(in);
          in = NULL;
        }
      }
    } else {
      hiddenbits = 0;
    }
  }

  return in;
}
///

/// ParseQuantizationSteps
// Parse a quantization matrix from a file
bool ParseQuantizationSteps(LONG lumamatrix[64],LONG chromamatrix[64],const char *filename)
{
  FILE *file = fopen(filename,"r");
  bool result = false;

  if (file) {
    int i;
    for(i = 0;i < 64;i++) {
      int val;
      if (fscanf(file,"%d",&val) != 1) {
        fprintf(stderr,"cannot parse an integer value from %s as quantization matrix entry\n",filename);
        break;
      }
      if (val <= 0 || val >= 32768) {
        fprintf(stderr,"quantization matrix entry %d in file %s is out of range, must be > 0 and < 32768\n",val,filename);
        break;
      }
      lumamatrix[i] = chromamatrix[i] = val;
    }
    if (i == 64) {
      result = true;
      for(i = 0;i < 64;i++) {
      int val;
      if (fscanf(file,"%d",&val) != 1) {
        if (i > 0) {
          fprintf(stderr,"cannot parse an integer value from %s as quantization matrix entry\n",filename);
          result = false;
        }
        break;
      }
      if (val <= 0 || val >= 32768) {
        fprintf(stderr,"quantization matrix entry %d in file %s is out of range, must be > 0 and < 32768\n",val,filename);
        result = false;
        break;
      }
      chromamatrix[i] = val;
      }
    }
    fclose(file);
  } else {
    fprintf(stderr,"could not open %s as source for quantization matrix\n",filename);
  }
  return result;
}
///
