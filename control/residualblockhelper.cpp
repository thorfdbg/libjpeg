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
**
** This class computes, prepares or includes residual data for block
** based processing. It abstracts parts of the residual coding
** process.
**
** $Id: residualblockhelper.cpp,v 1.68 2022/06/14 06:18:30 thor Exp $
**
*/


/// Includes
#include "control/residualblockhelper.hpp"
#include "control/blockbitmaprequester.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "marker/quantizationtable.hpp"
#include "boxes/dctbox.hpp"
#include "boxes/mergingspecbox.hpp"
#include "interface/imagebitmap.hpp"
#include "tools/rectangle.hpp"
#include "tools/numerics.hpp"
#include "colortrafo/colortrafo.hpp"
#include "codestream/tables.hpp"
#include "dct/dct.hpp"
#include "std/string.hpp"
///

/// Defines
//#define SAVE_RESIDUAL
#ifdef SAVE_RESIDUAL
//#define SAVE_16BIT
#define TO_RGB
#define SCALE 128.0
UWORD *residual = NULL;
ULONG res_width,res_height;
#endif
//#define SAVE_SPECTRUM
#ifdef SAVE_SPECTRUM
double spectrum[64];
#endif
///

/// ResidualBlockHelper::ResidualBlockHelper
ResidualBlockHelper::ResidualBlockHelper(class Frame *frame,class Frame *residual)
  : JKeeper(frame->EnvironOf()), m_pFrame(frame), m_pResidualFrame(residual)
{ 
  UBYTE i;

  assert(frame != residual);
  m_ucCount         = m_pFrame->DepthOf();
  m_ucMaxError      = frame->TablesOf()->MaxErrorOf();
  m_bHaveQuantizers = false;

  for(i = 0;i < 4;i++) {
    m_pDCT[i]       = NULL;  
    m_pBuffer[i]    = m_ColorBuffer[i];
  }
#ifdef SAVE_SPECTRUM
  for(i = 0;i < 64;i++) {
    spectrum[i] = 0.0;
  }
#endif
#ifdef SAVE_RESIDUAL
  {
    res_width  = (frame->WidthOf()  + 7) & -8;
    res_height = (frame->HeightOf() + 7) & -8;
    ::residual = (UWORD *)malloc(sizeof(UWORD) * (res_width * res_height * 3));
  }
#endif
}
///

/// ResidualBlockHelper::~ResidualBlockHelper
ResidualBlockHelper::~ResidualBlockHelper(void)
{ 
  UBYTE i;
  
  for(i = 0;i < 4;i++) {
    delete m_pDCT[i];
  }

#ifdef SAVE_SPECTRUM
  {
    FILE *spectrumfile = fopen("spectrum.plot","w");
    if (spectrumfile) {
      for(i = 0;i <64;i++) {
        fprintf(spectrumfile,"%d\t%g\n",i,spectrum[DCT::ScanOrder[i]]);
      }
      fclose(spectrumfile);
    }
  }
#endif
#ifdef SAVE_RESIDUAL
  if (residual) {
    FILE *resfile = fopen("residual.ppm","wb");
    if (resfile) {
#ifdef SAVE_16BIT
      fprintf(resfile,"P6\n%u\t%u\t65535\n",res_width,res_height);
#else
      fprintf(resfile,"P6\n%u\t%u\t255\n",res_width,res_height);
#endif
      for(ULONG y = 0;y < res_height;y++) {
        for(ULONG x = 0;x < res_width;x++) {
          UWORD r = residual[3 * (x + y * res_width) + 0];
          UWORD g = residual[3 * (x + y * res_width) + 1];
          UWORD b = residual[3 * (x + y * res_width) + 2];
#ifdef SAVE_16BIT
          fputc(r >> 8,resfile);
          fputc(r >> 0,resfile);
          fputc(g >> 8,resfile);
          fputc(g >> 0,resfile);
          fputc(b >> 8,resfile);
          fputc(b >> 0,resfile);
#else
#ifdef TO_RGB
          double rf = (r - 0x8000 + (b - 0x8000) * 1.40200);
          double gf = (r - 0x8000 - (b - 0x8000) * 0.7141362859 - (g - 0x8000) * 0.3441362861);
          double bf = (r - 0x8000 + (g - 0x8000) * 1.772);
          WORD deltar = rf / SCALE;
          WORD deltag = gf / SCALE;
          WORD deltab = bf / SCALE;
#else
          WORD deltar = r - 0x8000;
          WORD deltag = g - 0x8000;
          WORD deltab = b - 0x8000;
#endif
          if (deltar < -128) deltar = -128;
          if (deltar >  127) deltar = 127;
          if (deltag < -128) deltag = -128;
          if (deltag >  127) deltag = 127;
          if (deltab < -128) deltab = -128;
          if (deltab >  127) deltab = 127;
          fputc(deltar + 128,resfile);
          fputc(deltag + 128,resfile);
          fputc(deltab + 128,resfile);
#endif
        }
      }
      fclose(resfile);
    }
    free(residual);
  }
#endif
}
///

/// ResidualBlockHelper::DequantizeResidual
// Dequantize the already decoded residual (possibly taking the decoded
// image as predictor) and return it, ready for the color transformation.
void ResidualBlockHelper::DequantizeResidual(const LONG *,LONG *target,const LONG *residual,UBYTE i)
{
  LONG dcshift = (1L << m_pResidualFrame->HiddenPrecisionOf()) >> 1;

  assert(residual);
  //
  AllocateBuffers();
  //
  // Depending on the transformation type, either
  // run the DCT or just dequantize.
  if (m_pDCT[i]) {
    m_pDCT[i]->InverseTransformBlock(target,residual,dcshift);
  } else {
    UWORD quant     = m_usQuantization[i];
    const LONG *res = residual; // the residual buffer.
    LONG *dst       = target;
    //
    int x,y,dx,dy;
    for(y = 0;y < 64;y += 16) {
      for(x = 0;x < 8;x += 2) {
        LONG avg = 0;
        if (m_bNoiseShaping[i]) {
          for(dy = 0;dy < 16;dy += 8) {
            for(dx = 0;dx < 2;dx++) {
              avg   += res[x + dx + y + dy] * quant;
            }
          }
        }
        avg = (avg + 2) >> 2;
        for(dy = 0;dy < 16;dy += 8) {
          for(dx = 0;dx < 2;dx++) {
            LONG v = res[x + dx + y + dy] * quant;
            if (m_bNoiseShaping[i] && v > avg - quant && v < avg + quant) {
              v = avg;
            }
            dst[x + dx + y + dy] = v + dcshift;
          }
        }
      }
    }
  }
}
///

/// Variance4x4
// Compute the variance of the 4x4 subblock starting at the indicated position
#ifdef EXPERIMENTAL_MASKING
static QUAD Variance4x4(const LONG *org)
{
  QUAD a = org[ 0] + org[ 1] + org[ 2] + org[ 3] 
    +      org[ 8] + org[ 9] + org[10] + org[11]
    +      org[16] + org[17] + org[18] + org[19]
    +      org[24] + org[25] + org[26] + org[27];

  a = (a + 8) >> 4;

  QUAD v = (org[ 0]-a)*(org[ 0]-a)+(org[ 1]-a)*(org[ 1]-a)+(org[ 2]-a)*(org[ 2]-a)+(org[ 3]-a)*(org[ 3]-a)
    +      (org[ 8]-a)*(org[ 8]-a)+(org[ 9]-a)*(org[ 9]-a)+(org[10]-a)*(org[10]-a)+(org[11]-a)*(org[11]-a)
    +      (org[16]-a)*(org[16]-a)+(org[17]-a)*(org[17]-a)+(org[18]-a)*(org[26]-a)+(org[19]-a)*(org[27]-a)
    +      (org[24]-a)*(org[24]-a)+(org[25]-a)*(org[25]-a)+(org[18]-a)*(org[26]-a)+(org[19]-a)*(org[27]-a);

  v = (v + 8) >> 4;

  return v;
}
#endif
///

/// Variance2x2
// Compute the variance of the 2x2 subblock starting at the indicated position
static LONG Variance2x2(const LONG *org)
{
  LONG a = (org[0] + org[1] + org[8] + org[9] + 2) >> 2;
  LONG v = ((org[0] - a) * (org[0] - a) + (org[1] - a) * (org[1] - a) + 
            (org[8] - a) * (org[8] - a) + (org[9] - a) * (org[9] - a) + 2) >> 2;
  return v;
}
///

/// ClearSubblock4x4
// Clear the 4x4 subblock at the indicated position
// ClearSubblock
#ifdef EXPERIMENTAL_MASKING
static void ClearSubblock4x4(LONG *res,LONG dcshift)
{
  res[ 0] = res[ 1] = res[ 2] = res[ 3] = dcshift;
  res[ 8] = res[ 9] = res[10] = res[11] = dcshift;
  res[16] = res[17] = res[18] = res[19] = dcshift;
  res[24] = res[25] = res[26] = res[27] = dcshift;
}
#endif
///

/// ClearSubblock2x2
// Clear the 2x2 subblock at the indicated position
// ClearSubblock
static void ClearSubblock2x2(LONG *res,LONG dcshift)
{
  res[ 0] = res[ 1] = dcshift;
  res[ 8] = res[ 9] = dcshift;
}
///

/// ClearSubblock8x8
// Clear the 8x8 subblock at the indicated position
// ClearSubblock
static void ClearSubblock8x8(LONG *res,LONG dcshift)
{
  for(int k = 0;k < 64;k++) {
    *res++ = dcshift;
  }
}
///

/// ResidualBlockHelper::FindQuantizationFor
// Find the quantization table for residual component i (index, not label).
// Throws if this table is not available.
class QuantizationTable *ResidualBlockHelper::FindQuantizationFor(UBYTE i) const
{ 
  class QuantizationTable *table    = NULL;
  class Component *comp = m_pResidualFrame->ComponentOf(i);
  if (comp)
    table = m_pResidualFrame->TablesOf()->FindQuantizationTable(comp->QuantizerOf());
  if (table == NULL)
    JPG_THROW(MALFORMED_STREAM,"ResidualBlockHelper::FindQuantizationFor",
              "Unable to find the specified residual quantization matrix in "
              "the legacy codestream.");
  //
  return table;
}
///

/// ResidualBlockHelper::FindDCTFor
// Find the DCT transformation for component i, if enabled.
class DCT *ResidualBlockHelper::FindDCTFor(UBYTE i) const
{
  return m_pResidualFrame->TablesOf()->BuildDCT(m_pResidualFrame->ComponentOf(i),m_ucCount,
                                                m_pResidualFrame->HiddenPrecisionOf());
}
///

/// ResidualBlockHelper::AllocateBuffers
// Allocate the temporary buffers to hold the residuals and their bitmaps.
// Only required during encoding.
void ResidualBlockHelper::AllocateBuffers(void)
{
  if (!m_bHaveQuantizers) {
    class MergingSpecBox *res = m_pFrame->TablesOf()->ResidualSpecsOf(); 
    UBYTE rbits   = m_pResidualFrame->TablesOf()->FractionalColorBitsOf(m_ucCount,
                                                                        m_pResidualFrame->isDCTBased());
    UBYTE i,depth = m_pFrame->DepthOf();
    
    m_ucCount     = depth;
    //
    // There hopefully is a merging spec box.
    if (res) {
      //
      // Find noise shaping and quantization and DCT parameters.
      for(i = 0;i < depth;i++) {
        assert(m_pDCT[i] == NULL);
        
        switch(res->RDCTProcessOf()) {
        case DCTBox::Bypass:
          m_bNoiseShaping[i] = res->isNoiseShapingEnabled();
          //
          // Only the highest frequency entry is used.
          m_usQuantization[i] = FindQuantizationFor(i)->DeltasOf()[63];
          // If this is a color signal with pre-shifted bits, include
          // the subtraction of pre-shifted color bits so we get integer
          // bits already. For RCT, we could either say that there is one
          // fractional bit and quantization deltas are halfed, or we say
          // that the bit-range is one bit larger.
          if (rbits > 1)
            m_usQuantization[i] <<= rbits;
          // Actually, was NULL before.
          m_pDCT[i]           = NULL;
          break;
        case DCTBox::FDCT:
        case DCTBox::IDCT:
          // Both handled by the same process.
          m_bNoiseShaping[i]  = false;
          m_pDCT[i]           = FindDCTFor(i);
          m_usQuantization[i] = 0;
        }
      }
    } else {
      // Code should actually not go in here. How weird.
      for(i = 0;i < depth;i++) {
        m_bNoiseShaping[i]  = false;
        m_pDCT[i]           = NULL;
        m_usQuantization[i] = 1;
      }
    }
    //
    //
    m_bHaveQuantizers = true;
  }
}
///

/// ResidualBlockHelper::QuantizeResidual
// Compute the residuals of a block given the DCT data
void ResidualBlockHelper::QuantizeResidual(const LONG *legacy,LONG *residual,UBYTE i,LONG bx,LONG by)
{ 
  ULONG rmaxval  = (1UL << m_pResidualFrame->HiddenPrecisionOf()) - 1;
  ULONG rdcshift = (rmaxval + 1) >> 1;

  //
  AllocateBuffers();
  //
#ifdef SAVE_RESIDUAL
  if (::residual) {
    int x,y;
    UBYTE rbits  = 0;
    // The output of the YCbCr transformation is preshifted by four bits by convention in
    // this implementation (not part of the specs!). Get the shift to fixup the DC
    // offset.
    if (m_pResidualFrame->TablesOf()->RTrafoTypeOf(m_ucCount) == MergingSpecBox::YCbCr) {
      rbits = m_pResidualFrame->TablesOf()->FractionalColorBitsOf(m_ucCount);
    }
    UWORD *rgb = ::residual + 3 * ((bx << 3) + (by << 3) * res_width);
    for(y = 0;y < 8;y++) {
      for(x = 0;x < 8;x++) {
        rgb[(x + y * res_width) * 3 + i] = 0x8000 + ((residual[x + y * 8] - (rdcshift << rbits)));
      }
    }
  }
#endif
  // A test: Measure the variance in each 4x4 subblock in the original image, if all of them are 
  // above a threshold, drop the residual due to masking.
  if (legacy && m_ucMaxError > 0) {
    LONG thres      = (m_ucMaxError + 1) * (m_ucMaxError + 1);
    const LONG *org = legacy;    // the original data.
    LONG *res       = residual;  // the residual buffer.
    int count       = 0;
    for(int yoff = 0; yoff < 8;yoff += 2) {
      for (int xoff = 0;xoff < 8;xoff += 2) {
        if (Variance2x2(org + xoff + yoff * 8) > thres) {
          ClearSubblock2x2(res + xoff + yoff * 8,rdcshift);
          count++;
        }
      }
    }
    if (count > 11) {
      ClearSubblock8x8(res,rdcshift);
    }
  }
  //
  // Finally, DCT transform and quantize, or quantize directly.
  if (m_pDCT[i]) {
    m_pDCT[i]->TransformBlock(residual,residual,rdcshift);
    if (m_pResidualFrame->TablesOf()->Optimization()) {
      m_pResidualFrame->OptimizeDCTBlock(bx,by,i,m_pDCT[i],residual);
    }
#ifdef SAVE_SPECTRUM
    // Add up the signal energy of the residual in all bands.
    {
      for(int j = 0;j < 64;j++) {
        spectrum[j] += residual[j] * residual[j];
      }
    }
#endif
  } else {
    // Quantize and noise-shape in the spatial domain
    UWORD quant = m_usQuantization[i];
    LONG *res   = residual; // the residual buffer.
    LONG  error = 0;
    int   p     = 0;
    LONG  v;
    int qnt;
    int x,y,dx,dy;
    for(y = 0;y < 64;y += 16) {
      for(x = 0;x < 8;x += 2) {
        for(dy = 0;dy < 16;dy += 8) {
          for(dx = 0;dx < 2;dx++) {
            p      = x + dx + y + dy;
            v      = res[p] - rdcshift;
            if (m_bNoiseShaping[i])
              v     += error;
            // This is currently a deadzone-quantizer with a deadzone T = 2\Delta.
            qnt    = v / quant;
            error += res[p] - rdcshift - quant * qnt;
            if (qnt > int(rdcshift) || qnt < -int(rdcshift) || qnt > MAX_WORD || qnt < MIN_WORD)
              JPG_THROW(OVERFLOW_PARAMETER,"ResidualBlockHelper::QuantizeResidual",
                        "Error residual is too large, try to increase the base layer quality");
            res[p] = qnt;
          }
        }
      }
    }
  }
}
///
