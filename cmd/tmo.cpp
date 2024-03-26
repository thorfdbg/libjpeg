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
** A couple of generic TMO related functions: Estimate TMO from LDR and HDR
** image pair, build a gamma mapping.
**
** $Id: tmo.cpp,v 1.20 2024/03/25 18:42:33 thor Exp $
**
*/

/// Includes
#include "cmd/tmo.hpp"
#include "cmd/iohelpers.hpp"
#include "std/stdlib.hpp"
#include "std/string.hpp"
#include "std/math.hpp"
#include "std/assert.hpp"
///

/// Defines
//#define SAVE_ITMO
///

/// InvertTable
// Invert a tabulated tone mapping table. To be on the fair side,
// this uses the same algorithm the library uses to build the forwards
// table from the backwards table provided. Otherwise, I could also
// invert numerically the (parametric) table.
void InvertTable(UWORD input[65536],UWORD output[65536],UBYTE inbits,UBYTE outbits)
{ 
  LONG j,lastj,lastanchor;
  LONG last,current,mid;
  LONG outmax = (1L << outbits) - 1;
  LONG inmax  = (1L << inbits)  - 1;
  bool lastfilled;

  assert(outbits <= 16);
  assert(inbits  <= 16);
  
  // Not guaranteed that the mapping is surjective onto the output
  // range. There is nothing that says how to handle this case. We just define
  // "undefined" outputs to zero, and try our best to continue the missing parts
  // continuously along the output range. 
  memset(output,0,(1 << outbits) * sizeof(UWORD));
  //
  // Loop over positive coefficients.
  lastj      = inmax;
  lastanchor = inmax;
  lastfilled = false;
  j          = inmax;
  last       = input[j];
  //
  // Try some guesswork whether we should extend this to fullrange.
  // This avoids trouble in case the backwards mapping is not
  // onto, but we find the corresponding out-of-range pixels in the
  // input image.
  if (last < (((outmax + 1) * 3) >> 2)) {
    last = outmax;
  }
  //
  //
  // Go from max to zero. This direction is intentional.
  do {
    // Get the next possible output for the given input.
    // Swap back to get the proper endianness.
    current = input[j];
    // If the function jumps, fill in half the values with the old
    // the other half with the new values. The output is never
    // swapped here, otherwise the table would grow out of range
    // too easily.
    if (current == last) {
      // Found a "flat" area, i.e. had the same external value for 
      // similar internal values. If so, fill in the midpoint 
      // into the table. If lastanchor + j overflows, then our
      // tables are far too huge in first place.
      output[last] = (lastanchor + j) >> 1;
      lastfilled               = true;
    } else {
      // Found a "steep" part of the output curve. 
      // If the function jumps, fill in half the values with the old
      // the other half with the new values. The output is never
      // swapped here, otherwise the table would grow out of range
      // too easily.
      if (last > current) {
        mid = ((current + last + 1) >> 1) - 1;
      } else {
        mid = ((current + last - 1) >> 1) - 1;
      }
      while(last != mid) {
        if (lastfilled == false) // Do not overwrite the flat area from the last time.
          output[last] = lastj;
        if (last > mid)    last--;
        else               last++;
        lastfilled = false;
      }
      while(last != current) {
        if (lastfilled == false) // Do not overwrite the flat area from the last time.
          output[last]  = j;
        if (last > current) last--;
        else                last++;
        lastfilled = false;
      }
      lastanchor = j;
      }
    lastj = j;
    last  = current;
  } while(j--); // This includes the case j = 0 in the inner loop
  // Now we could have the situation that "lastfilled" is still false,
  // thus lut[last] is not yet filled. j is now -1 and thus not
  // usable, lastj == 0, and there is no further point to extrapolate
  // to. Thus, set to the exact end-point.
  if (lastfilled == false)
    output[last] = lastj;
  //
  // Fixup the ends of the table. If the start or the end of the LUT have a very low slope,
  // we will find jumps in the table that are likely undesired. Fix them up here to avoid
  // artefacts in the image.
  if (outmax > 4) {
    LONG i1,i2,i3;
    LONG d1,d2;
    //
    i1 = output[0];
    i2 = output[1];
    i3 = output[2];
    //
    d1 = (i1 > i2)?(i1 - i2):(i2 - i1);
    d2 = (i3 > i2)?(i3 - i2):(i2 - i3);
    //
    // If the first jump is too large, clip it.
    if (d1 > 2 * d2)
      output[0] = 2 * i2 - i3;
    //
    // Now at the other end. Note that max is inclusive.
    i1 = output[outmax];
    i2 = output[outmax - 1];
    i3 = output[outmax - 2];
    //
    d1 = (i1 > i2)?(i1 - i2):(i2 - i1);
    d2 = (i3 > i2)?(i3 - i2):(i2 - i3);
    //
    if (d1 > 2 * d2)
      output[outmax] = 2 * i2 - i3;
  }
}
///

/// SaveHistogram
#ifdef SAVE_ITMO
static void save_histogram(const char *filename,double hist[256])
{
  FILE *out = fopen(filename,"w");
  if (out) {
    for(int i = 0;i < 256;i++) {
      if (hist[i] >= 0.0) {
        fprintf(out,"%d\t%g\n",i,hist[i]);
      }
    }
    fclose(out);
  }
}
#endif
///

/// BuildIntermediateTable
// Build an intermediate table from a histogram.
void BuildIntermediateTable(int **hists,int offs,int hdrcnt,
                            UWORD ldrtohdr[65536],int hiddenbits,
                            bool median,bool &fullrange,bool flt,
                            int smooth)
{     
  int i,j,k;
  double intermed[256];
  LONG absmin = hdrcnt;
  LONG absmax = 0;
  //
  // For each LDR value (first index), find a suitable
  // HDR value to map to. 
  // Several methods could be used here.
  // As a starter, use the average.
  for(i = 0;i < 256;i++) {
    int count  = 0;
    int min    = -1;
    int max    = -1;
    for(j = 0;j < hdrcnt;j++) {
      count += hists[i+offs][j];
      if (hists[i+offs][j] > 0) {
        if (min == -1) min = j;
        max = j;
      }
    }
    if (count > 0) {
      if (max > absmax)
        absmax = max;
      if (min < absmin)
        absmin = min;
      if (max - min > (hdrcnt >> 1)) {
        fullrange   = true;
        intermed[i] = (max - min) >> 1;
      } else {
        if (median && count > 1) {
          int median = 0;
          for(j = 0;j < hdrcnt;j++) {
            median += hists[i+offs][j];
            if (median >= (count >> 1))
              break;
          }
          intermed[i] = j;
        } else {
          double total = 0.0;
          double sum   = 0.0;
          for(j = 0;j < hdrcnt;j++) {
            sum   += double(hists[i+offs][j]) * j;
            total += double(hists[i+offs][j]);
          }
          intermed[i] = sum / total;
        }
      }
    } else {
      intermed[i] = -1;
    }
  }
  //
#ifdef SAVE_ITMO
  save_histogram("histogram.plot",intermed);
#endif
  //
  // Fill in "holes" in the intermediate map.
  if (absmin == hdrcnt)
    absmin = 0;
  if (absmax == 0)
    absmax = hdrcnt;
  double cur = absmin;
  double nex = 0.0;
  int v = 0;
  for(i = 0;i < 256;i++) {
    if (intermed[i] < 0.0) {
      // Find the next filled slot.
      for(j = i;j < 256;j++) {
        if (intermed[j] >= 0.0) {
          nex = intermed[j];
          break;
        }
      }
      if (j == 256)
        nex = absmax;
      // Use a linear interpolation to fill the gaps.
      for(k = i;k < j;k++) {
        intermed[i] = double(k - v)/double(j - v) * (nex - cur) + cur;
      }
    } else {
      cur = intermed[i];
      v   = i;
    }
  }
  //
#ifdef SAVE_ITMO
  save_histogram("histogram-filled.plot",intermed);
#endif
  //
  // Make the map monotonical. First find the minimum.
  double min = hdrcnt;
  double max = 0;
  for (i = 0;i < 256;i++) {
    if (intermed[i] < min)
      min = intermed[i];
    if (intermed[i] > max)
      max = intermed[i];
  }
  // Second, map everything below the minimum to the minimum value. This will be extended later.
  if (intermed[0] > min) {
    for (i = 0;i < 256;i++) {
      if (intermed[i] >= min + i) {
        intermed[i] = min + i;
      } else break;
    }
  }
  //
  int now = intermed[0] - 1;
  for(i = 0;i < 256;i++) {
    if (intermed[i] <= now) {
      if (now + 1 < max) {
        intermed[i] = now + 1;
      } else {
        intermed[i] = max;
      }
    }
    now = intermed[i];
  }
  //
  if (max > intermed[255]) {
    now = intermed[255] + 1;
    for(i = 255;i >= 0;i--) {
      if (intermed[i] >= now) {
        if (now - 1 > min) {
          intermed[i] = now - 1;
        } else {
          intermed[i] = min + i;
        }
      }
      now = intermed[i];
    }
  }
  //
#ifdef SAVE_ITMO
  save_histogram("histogram-monotonic.plot",intermed);
#endif
  //
  // Now smoothen the values, outwards in.
  for(i = 0;i < smooth;i++) {
    for(j = 1;j < 255;j+=2) {
      intermed[j] = 0.25 * intermed[j-1] + 0.5 * intermed[j] + 0.25 * intermed[j+1];
    }
    for(j = 2;j < 255;j+=2) {
      intermed[j] = 0.25 * intermed[j-1] + 0.5 * intermed[j] + 0.25 * intermed[j+1];
    }
  }
  //
#ifdef SAVE_ITMO
  save_histogram("histogram-smooth.plot",intermed);
#endif

  // Use a very simple interpolation to fill in the final
  // output map. Note that this might expand the output by "hiddenbits".
  for(i = 0;i < (256 << hiddenbits);i++) {
    j = i >> hiddenbits;
    k = j + 1;
    if (k >= 256)
      k = 255;
    if (k > j) {
      ldrtohdr[i] = double(i - (j << hiddenbits)) / 
        double((k << hiddenbits) - double(j << hiddenbits)) * 
        (intermed[k] - intermed[j]) + intermed[j];
    } else {
      ldrtohdr[i] = intermed[j];
    }
    // If this is floating point, invert negative values
    // to create a continuous map.
    if (flt) {
      if (ldrtohdr[i] & 0x8000)
        ldrtohdr[i] ^= 0x7fff;
    }
  }
  //
}
///

/// BuildToneMappingFromLDR (for FLOAT)
// Build an inverse tone mapping from a hdr/ldr image pair, though generate it as
// a floating point table. This requires floating point input.
void BuildToneMappingFromLDR(FILE *in,FILE *ldrin,int w,int h,int count,
                             FLOAT ldrtohdr[256],
                             bool bigendian,bool median,bool fullrange,
                             int smooth)
{
  int i;
  UWORD tmp[65536];
  DOUBLE scale;
  //
  // Call the generic function. This returns half-float values we still have to cast to float.
  BuildToneMappingFromLDR(in,ldrin,w,h,16,count,tmp,true,bigendian,false,0,median,fullrange,smooth);
  //
  // Potentially scale the map so we avoid clamping. This is necessary because the output
  // of this map goes into the 2nd base trafo, which implies input clamping. Profile A can
  // compensate for this by the mu-map. Profile B has the output transformation exposure
  // value and hence can compensate for it, too. The 65535.0 comes from the output
  // transformation.
  scale = 65535.0 / HalfToDouble(tmp[255]);
  //
  // Now convert the sample values to float.
  for(i = 0;i < 256;i++) {
    ldrtohdr[i] = HalfToDouble(tmp[i]) * scale; // This is the output transformation.
  }
}
///

/// BuildToneMappingFromLDR
// Build an inverse tone mapping from a hdr/ldr image pair
void BuildToneMappingFromLDR(FILE *in,FILE *ldrin,int w,int h,int depth,int count,
                             UWORD ldrtohdr[65536],bool flt,
                             bool bigendian,bool xyz,int hiddenbits,bool median,bool &fullrange,
                             int smooth)
{
  long hpos  = ftell(in);
  long lpos  = ftell(ldrin);
  int **hists = NULL; // Histograms for each LDR pixel value.
  int i;
  int hdrcnt = (flt)?(65536):(1 << depth);
  int x,y;
  bool warn = false;

  hists     = (int **)malloc(sizeof(int *) * 256);
  fullrange = false;
  if (hists) {
    memset(hists,0,sizeof(int *) * 256);
    for(i = 0;i < 256;i++) {
      hists[i] = (int *)malloc(sizeof(int) * hdrcnt);
      if (hists[i] == NULL)
        break;
      memset(hists[i],0,sizeof(int) * hdrcnt);
    }
    if (i == 256) {
      for(y = 0;y < h;y++) {
        for(x = 0;x < w;x++) {
          // Read the HDR image parameters.
          int r,g,b;
          int rl,gl,bl;
          double y;
          //
          warn |= ReadRGBTriple(in,r,g,b,y,depth,count,flt,bigendian,xyz);
          /*
          r     = y * (hdrcnt - 1) + 0.5;
          if (r < 0)       r = 0;
          if (r >= hdrcnt) r = hdrcnt - 1;
          */
          //
          // Read the LDR parameters.
          ReadRGBTriple(ldrin,rl,gl,bl,y,8,count,false,false,false);
          /*
          rl    = y * 255 + 0.5;
          if (rl < 0)   rl = 0;
          if (rl > 255) rl = 255;
          */
          // Update the histogram.
          // Actually, here it might make sense to collect
          // three histograms, not one. The coding core
          // would actually even support this, though this
          // frontend is currently limited.
          hists[rl][r]++;
          hists[gl][g]++;
          hists[bl][b]++;
        }
      }
      //
      // Build tables for each component.
      BuildIntermediateTable(hists,0,hdrcnt,ldrtohdr,hiddenbits,median,fullrange,flt,smooth);
      //
      // Release the temporary storage for the histogram.
      for(i = 0;i < 256;i++) {
        free(hists[i]);
      }
    }
    free(hists);
  }

  fseek(in   ,hpos,SEEK_SET);
  fseek(ldrin,lpos,SEEK_SET);

  if (warn)
    fprintf(stderr,"Warning: Input image contains out of gamut values, clamping it.\n");
}
///

/// BuildRGBToneMappingFromLDR
// Build an inverse tone mapping from a hdr/ldr image pair
void BuildRGBToneMappingFromLDR(FILE *in,FILE *ldrin,int w,int h,int depth,int count,
                                UWORD red[65536],UWORD green[65536],UWORD blue[65536],
                                bool flt,bool bigendian,bool xyz,int hiddenbits,
                                bool median,bool &fullrange,int smooth)
{
  long hpos  = ftell(in);
  long lpos  = ftell(ldrin);
  int **hists = NULL; // Histograms for each LDR pixel value.
  int i;
  int hdrcnt = (flt)?(65536):(1 << depth);
  int x,y;
  bool warn = false;

  fullrange = false;
  hists     = (int **)malloc(sizeof(int *) * 256 * 3);
  if (hists) {
    memset(hists,0,sizeof(int *) * 256 * 3);
    for(i = 0;i < 256 * 3;i++) {
      hists[i] = (int *)malloc(sizeof(int) * hdrcnt);
      if (hists[i] == NULL)
        break;
      memset(hists[i],0,sizeof(int) * hdrcnt);
    }
    if (i == 256 * 3) {
      for(y = 0;y < h;y++) {
        for(x = 0;x < w;x++) {
          // Read the HDR image parameters.
          int r,g,b;
          int rl,gl,bl;
          double y;
          //
          warn |= ReadRGBTriple(in,r,g,b,y,depth,count,flt,bigendian,xyz);
          //
          // Read the LDR parameters.
          ReadRGBTriple(ldrin,rl,gl,bl,y,8,count,false,false,false);
          // Update the histogram.
          // Actually, here it might make sense to collect
          // three histograms, not one. The coding core
          // would actually even support this, though this
          // frontend is currently limited.
          hists[rl + (0<<0)][r]++;
          hists[gl + (1<<8)][g]++;
          hists[bl + (2<<8)][b]++;
        }
      }
      //
      // Build tables for each component.
      BuildIntermediateTable(hists,0 << 8,hdrcnt,red  ,hiddenbits,median,fullrange,flt,smooth);
      BuildIntermediateTable(hists,1 << 8,hdrcnt,green,hiddenbits,median,fullrange,flt,smooth);
      BuildIntermediateTable(hists,2 << 8,hdrcnt,blue ,hiddenbits,median,fullrange,flt,smooth);
      //
      // Release the temporary storage for the histogram.
      for(i = 0;i < 256;i++) {
        free(hists[i]);
      }
    }
    free(hists);
  }

  fseek(in   ,hpos,SEEK_SET);
  fseek(ldrin,lpos,SEEK_SET);

  if (warn)
    fprintf(stderr,"Warning: Input image contains out of gamut values, clamping it.\n");
}
///

/// BuildGammaMapping
// Build a static gamma mapping to map the HDR to the LDR domain.
void BuildGammaMapping(double gamma,double exposure,UWORD ldrtohdr[65536],
                       bool flt,int max,int hiddenbits)
{
  int i;
  int outmax   = (flt)?(0x7bff):(max); // 0x7c00 is INF in half-float
  int inmax    = 256 << hiddenbits;
  double knee  = 0.04045;
  double divs  = pow((knee + 0.055) / 1.055,gamma) / knee;
  double shift = 1.0 / (1 << (12 + hiddenbits));
  
  for(i = 0;i < inmax;i++) {
    double in  = i / double(inmax - 1);
    double out = 0.0;
    int int_out;
    if (gamma != 1.0) {
      if (in > knee) {
        out = pow((in + 0.055) / 1.055,gamma) / exposure;
      } else {
        out = in * divs / exposure;
      }
      if (flt) {
        int_out = DoubleToHalf(out + shift);
      } else {
        int_out = outmax * (out + shift) + 0.5;
      }
    } else {
      if (flt) {
        int_out = DoubleToHalf(out + shift);
      } else {
        int_out = outmax * (in  + shift) + 0.5;
      }
    }
    if (int_out > outmax) int_out = outmax;
    if (int_out < 0     ) int_out = 0;
    ldrtohdr[i] = int_out;
  }
}
///


/// LoadLTable
// Load an inverse tone mapping table from a file.
void LoadLTable(const char *ltable,UWORD ldrtohdr[65536],bool flt,int max,int hiddenbits)
{
  FILE *in = fopen(ltable,"r");

  if (in) {
    char buffer[256];
    int outmax   = (flt)?(0x7bff):(max); // 0x7c00 is INF in half-float
    int inmax    = 256 << hiddenbits;
    int line     = 0;
    int i        = 0;
    while(!feof(in) && !ferror(in)) {
      char *end;
      long value;
      fgets(buffer,sizeof(buffer),in);line++;
      if (buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\0')
        continue;
      value = strtol(buffer,&end,0);
      if (end <= buffer) {
        fprintf(stderr,"junk in LUT table definition file %s at line %d, ignoring this line.\n",ltable,line);
        continue;
      }
      if (*end != '\n') {
        fprintf(stderr,"junk in LUT table definition file %s behind line %d, ignoring the junk.\n",ltable,line);
      }
      if (i > inmax) {
        fprintf(stderr,"too many lines in file %s, line %d is superfluos. Expected only %d inputs.\n",ltable,line,inmax);
      } else {
        if (value > long(outmax)) {
          fprintf(stderr,"input value %ld at line %d in file %s is too large, maximum value is %d, clipping it.\n",
                  value,line,ltable,max);
          value = outmax;
        } else if (value < 0) {
          fprintf(stderr,"input value %ld at line %d in file %s is too small, minimum value is %d, clipping it.\n",
                  value,line,ltable,0);
          value = 0;
        }
        ldrtohdr[i++] = value;
      }
    }
    if (i < inmax) {
      fprintf(stderr,"file %s only defined %d out of %d values, extending the table by adding the maximum.\n",
              ltable,i,inmax);
      while(i < inmax) {
        ldrtohdr[i++] = outmax;
      }
    }
    fclose(in);
  }
}
///
