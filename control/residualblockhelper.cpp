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
**
** This class computes, prepares or includes residual data for block
** based processing. It abstracts parts of the residual coding
** process.
**
** $Id: residualblockhelper.cpp,v 1.20 2012-09-23 21:11:51 thor Exp $
**
*/


/// Includes
#include "control/residualblockhelper.hpp"
#include "control/blockbitmaprequester.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "marker/residualmarker.hpp"
#include "marker/residualspecsmarker.hpp"
#include "interface/imagebitmap.hpp"
#include "tools/rectangle.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/trivialtrafo.hpp"
#include "codestream/tables.hpp"
#include "dct/hadamard.hpp"
#include "dct/sermsdct.hpp"
#include "dct/dct.hpp"
#include "std/string.hpp"
///
    
/// Defines
#define NOISE_SHAPING
//#define TEST_RESIDUALS 1
///

/// ResidualBlockHelper::ResidualBlockHelper
ResidualBlockHelper::ResidualBlockHelper(class Frame *frame)
  : JKeeper(frame->EnvironOf()), m_pFrame(frame), m_pOriginalData(NULL),
    m_ppTransformed(NULL), m_ppReconstructed(NULL), m_pusIdentity(NULL),
    m_ppTrafo(NULL)
{ 
  m_ucCount      = m_pFrame->DepthOf();
  m_ucPointShift = m_pFrame->PointPreShiftOf();
}
///

/// ResidualBlockHelper::~ResidualBlockHelper
ResidualBlockHelper::~ResidualBlockHelper(void)
{ 
  UBYTE i;
  
  delete m_pOriginalData;
  
  if (m_ppTransformed) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppTransformed[i];
    }
    m_pEnviron->FreeMem(m_ppTransformed,sizeof(struct ImageBitMap *) * m_ucCount);
  }

  if (m_ppReconstructed) {
    for(i = 0;i < m_ucCount;i++) {
      m_pEnviron->FreeMem(m_ppReconstructed[i],64 * sizeof(LONG));
    }
    m_pEnviron->FreeMem(m_ppReconstructed,sizeof(LONG *) * m_ucCount);
  }

  if (m_pusIdentity)
    m_pEnviron->FreeVec(m_pusIdentity);

  if (m_ppTrafo) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppTrafo[i];
    }
    m_pEnviron->FreeMem(m_ppTrafo,sizeof(class DCT **) * m_ucCount);
  }
}
///

/// AddResidual
// Add a residual to an external component
template<typename external>
static void AddResidualHelper(const LONG *residual,const RectAngle<LONG> &r,
			      const struct ImageBitMap *target,ULONG max)
{ 
  LONG x,y;
  LONG xmin   = r.ra_MinX & 7;
  LONG ymin   = r.ra_MinY & 7;
  LONG xmax   = r.ra_MaxX & 7;
  LONG ymax   = r.ra_MaxY & 7;
  external *eptr = (external *)(target->ibm_pData);
  
  for(y = ymin;y <= ymax;y++) {
    const LONG *rsrc = residual + xmin + (y << 3);
    external      *e = eptr;
    for(x = xmin;x <= xmax;x++) {
      LONG v = *e + *rsrc;
      if (v < 0)         v = 0;
      if (v > LONG(max)) v = max;
      *e = v;
      rsrc++;
      e  = (external *)((UBYTE *)e + target->ibm_cBytesPerPixel);
    }
    eptr  = (external *)((UBYTE *)eptr + target->ibm_lBytesPerRow);
  }
}
///

/// ResidualBlockHelper::AddResidual
// Add residual coding errors from a side-channel if there is such a thing.
void ResidualBlockHelper::AddResidual(const RectAngle<LONG> &rect,
				      const struct ImageBitMap *const *target,
				      LONG * const *residual,ULONG max)
{
  if (residual) {
    bool  noiseshaping = m_pFrame->TablesOf()->ResidualSpecsOf()->isNoiseShapingEnabled();
    UBYTE i,k;
    //
    max = ((max + 1) << m_ucPointShift) - 1;
    //
    // Step 1
    AllocateBuffers();
    //
    if (BuildTransformations()) {
      // Run a Hadamard transform on the data, backwards now.
      for(i = 0;i < m_ucCount;i++) {
	m_ppTrafo[i]->InverseTransformBlock(m_ppReconstructed[i],residual[i],0);
      }
    } else {
      //
#ifdef TEST_RESIDUALS
      {
	LONG x = rect.ra_MinX;
	LONG y = rect.ra_MinY;
	LONG xmax = rect.ra_MaxX & 7;
	LONG ymax = rect.ra_MaxY & 7;
	UBYTE i;
	LONG xp,yp;
	for(i = 0;i < m_ucCount;i++) {
	  char resname[60];
	  char orgname[60];
	  sprintf(resname,"/tmp/residual_%d_%d_%d",x,y,i);
	  sprintf(orgname,"/tmp/original_%d_%d_%d",x,y,i);
	  FILE *res = fopen(resname,"rb");
	  FILE *org = fopen(orgname,"rb");
	  assert(res != NULL);
	  assert(org != NULL);
	  for(yp = 0;yp <= ymax;yp++) {
	    for(xp = 0;xp <= xmax;xp++) {
	      LONG resv,orgv;
	      LONG t    = 0x0badf00d;
	      UBYTE *dt = (UBYTE *)(target[i]->ibm_pData);
	      dt       += xp * target[i]->ibm_cBytesPerPixel;
	      dt       += yp * target[i]->ibm_lBytesPerRow;
	      switch(target[i]->ibm_ucPixelType) {
	      case CTYP_UBYTE:
		t = *dt;
		break;
	      case CTYP_UWORD:
		t = *(UWORD *)dt;
		break;
	      }
	      fscanf(res,"%d ",&resv);
	      fscanf(org,"%d ",&orgv);
	      assert(resv == residual[i][xp + (yp << 3)]);
	      assert(orgv == t);
	    }
	  }
	  fclose(org);
	  fclose(res);
	}
      }
#endif
      //
      // No transformation.
      for(i = 0;i < m_ucCount;i++) {
	UWORD quant = (i > 0 && i < 3)?(m_usChromaQuant):(m_usLumaQuant); 
	LONG *res   = residual[i]; // the residual buffer.
	LONG *rec   = m_ppReconstructed[i]; 
	int x,y,dx,dy;
	for(y = 0;y < 64;y += 16) {
	  for(x = 0;x < 8;x += 2) {
	    LONG avg = 0;
	    if (noiseshaping) {
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
		if (noiseshaping && v > avg - quant && v < avg + quant) {
		  v = avg;
		}
		rec[x + dx + y + dy] = v;
	      }
	    }
	  }
	}
      }
    }
    //
    //
    if (m_ucCount >= 3) {
      LONG *y  = m_ppReconstructed[0];
      LONG *cb = m_ppReconstructed[1];
      LONG *cr = m_ppReconstructed[2];
      for(k = 0; k < 64;k++) {
	LONG g = y[k] - ((cr[k] + cb[k]) >> 2);
	LONG r = cr[k] + g;
	LONG b = cb[k] + g;
	y[k]   = r;
	cb[k]  = g;
	cr[k]  = b;
      }
    }
    for(i = 0;i < m_ucCount;i++) {
      LONG *res = m_ppReconstructed[i];
      switch(target[i]->ibm_ucPixelType) {
      case CTYP_UBYTE:
	AddResidualHelper<UBYTE>(res,rect,target[i],max);
	break;
      case CTYP_UWORD:
	AddResidualHelper<UWORD>(res,rect,target[i],max);
	break;
      default:
	JPG_THROW(INVALID_PARAMETER,"ResidualCoder::AddResidual",
		  "sample type unknown or unsupported");
      }
    }
  }
}
///

/// Variance4x4
// Compute the variance of the 4x4 subblock starting at the indicated position
#if 0
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
static void ClearSubblock4x4(LONG *res)
{
  res[ 0] = res[ 1] = res[ 2] = res[ 3] = 0;
  res[ 8] = res[ 9] = res[10] = res[11] = 0;
  res[16] = res[17] = res[18] = res[19] = 0;
  res[24] = res[25] = res[26] = res[27] = 0;
}
///

/// ClearSubblock2x2
// Clear the 2x2 subblock at the indicated position
// ClearSubblock
static void ClearSubblock2x2(LONG *res)
{
  res[ 0] = res[ 1] = 0;
  res[ 8] = res[ 9] = 0;
}
///

/// ClearSubblock8x8
// Clear the 8x8 subblock at the indicated position
// ClearSubblock
static void ClearSubblock8x8(LONG *res)
{
  for(int k = 0;k < 64;k++) {
    *res++ = 0;
  }
}
#endif
///

/// ResidualBlockHelper::CreateTrivialTrafo
// Create a trivial color transformation that pulls the data from the source
class ColorTrafo *ResidualBlockHelper::CreateTrivialTrafo(class ColorTrafo *ctrafo,UBYTE bpp)
{
  // Get the color transformer that just collects the
  // original data for computing the residuals.
  if (m_pOriginalData == NULL) {
    switch(m_ucCount) {
    case 1:
      switch(ctrafo->PixelTypeOf()) {
      case CTYP_UBYTE:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UBYTE,1>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UWORD,1>(m_pEnviron);
	break;
      }
      break;
     case 2:
      switch(ctrafo->PixelTypeOf()) {
      case CTYP_UBYTE:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UBYTE,2>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UWORD,2>(m_pEnviron);
	break;
      }
      break;
    case 3:
      switch(ctrafo->PixelTypeOf()) {
      case CTYP_UBYTE:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UBYTE,3>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UWORD,3>(m_pEnviron);
	break;
      }
      break;
     case 4:
      switch(ctrafo->PixelTypeOf()) {
      case CTYP_UBYTE:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UBYTE,4>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pOriginalData = new(m_pEnviron) class TrivialTrafo<UWORD,4>(m_pEnviron);
	break;
      }
      break;
    }
    if (m_pOriginalData == NULL) {
      JPG_THROW(INVALID_PARAMETER,"ResidualBlockHelper::CreateTrivialTrafo",
		"sample type unknown or unsupported");
    }

    // Define a trivial tone mapping algorithm:
    // Simply the identity.
    if (m_pusIdentity == NULL) {
      int i,size = 1 << bpp;
      m_pusIdentity = (UWORD *)m_pEnviron->AllocVec(size * sizeof(UWORD));
      for(i = 0;i < size;i++) {
	m_pusIdentity[i] = i;
      }
    }
    {
      const UWORD *tables[4] = {m_pusIdentity,m_pusIdentity,m_pusIdentity,m_pusIdentity};
      m_pOriginalData->DefineEncodingTables(tables);
      m_pOriginalData->DefineDecodingTables(tables);
    }
  }

  return m_pOriginalData;
}
///

/// ResidualBlockHelper::AllocateBuffers
// Allocate the temporary buffers to hold the residuals and their bitmaps.
// Only required during encoding.
void ResidualBlockHelper::AllocateBuffers(void)
{
  UBYTE i;

  if (m_ppTransformed == NULL) {
    m_ppTransformed = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap *) * m_ucCount);
    memset(m_ppTransformed,0,sizeof(struct ImageBitMap *) * m_ucCount);
    for(i = 0;i < m_ucCount;i++) {
      m_ppTransformed[i] = new(m_pEnviron) struct ImageBitMap();
    }
  }

  if (m_ppReconstructed == NULL) {
    m_ppReconstructed = (LONG **)m_pEnviron->AllocMem(sizeof(LONG *) * m_ucCount);
    memset(m_ppReconstructed,0,sizeof(LONG *) * m_ucCount);
    for(i = 0;i < m_ucCount;i++) {
      m_ppReconstructed[i] = (LONG *)m_pEnviron->AllocMem(sizeof(LONG) * 64);
    }
  }
}
///

/// ResidualBlockHelper::BuildTransformations
// Allocate the Hadamard transformations
// and initialize their quantization factors.
bool ResidualBlockHelper::BuildTransformations(void)
{
  class ResidualSpecsMarker *residual = m_pFrame->TablesOf()->ResidualSpecsOf();

  if (m_ppTrafo == NULL) {
    const UWORD *quant;
    const UWORD *hdrluma   = NULL;
    const UWORD *hdrchroma = NULL;
    UWORD table[64];
    UBYTE i,k;

    if (residual) {
      UBYTE lumatable   = residual->LumaQuantizationMatrix();
      UBYTE chromatable = residual->ChromaQuantizationMatrix();
      
      if (lumatable < MAX_UBYTE)
	hdrluma = m_pFrame->TablesOf()->FindQuantizationTable(lumatable);

      if (chromatable < MAX_UBYTE)
	hdrchroma = m_pFrame->TablesOf()->FindQuantizationTable(chromatable);
    }
    
    if (residual->isHadamardEnabled()) {
      m_ppTrafo = (class DCT **)m_pEnviron->AllocMem(sizeof(class DCT *) * m_ucCount);
      memset(m_ppTrafo,0,sizeof(class DCT *) * m_ucCount);
      
      for(i = 0;i < m_ucCount;i++) {
	m_ppTrafo[i] = new(m_pEnviron) class Hadamard(m_pEnviron);
	//m_ppTrafo[i] = new(m_pEnviron) class SERMSDCT(m_pEnviron);
	if (i == 0 && hdrluma) {
	  m_ppTrafo[i]->DefineQuant(hdrluma);
	} else if (i > 0 && i < 3 && hdrchroma) {
	  m_ppTrafo[i]->DefineQuant(hdrchroma);
	} else {
	  quant = m_pFrame->TablesOf()->FindQuantizationTable(m_pFrame->ComponentOf(i)->QuantizerOf());
	  for(k = 0;k < 64;k++) {
	    table[k] = quant[k]; // >> ((i > 0)?(4):(5));
	    if (table[k] < 32) {
	      table[k] = 1;
	    } else if (m_ucCount >= 3 && (i == 1 || i == 2)) {
	      table[k] <<= 1;
	    }
	  }
	  m_ppTrafo[i]->DefineQuant(table);
	}
      }
      return true;
    } else {
      if (hdrluma)
	m_usLumaQuant = hdrluma[63];
      else
	m_usLumaQuant = 1;

      if (hdrchroma)
	m_usChromaQuant = hdrchroma[63];
      else
	m_usChromaQuant = 1;

      return false;
    }
  }

  return true;
}
///

/// ResidualBlockHelper::ComputeResiduals
// Compute the residuals of a block given the DCT data
void ResidualBlockHelper::ComputeResiduals(const RectAngle<LONG> &r,
					   class BlockBitmapRequester *req,
					   const struct ImageBitMap *const *source,
					   LONG *const *dct,LONG **residual,ULONG max)
{ 
  class ColorTrafo *ctrafo = req->ColorTrafoOf(false);
  class DCT *const *dtrafo = req->DCTsOf();
  ULONG pmax               = max;
  bool  noiseshaping       = m_pFrame->TablesOf()->ResidualSpecsOf()->isNoiseShapingEnabled();
  UBYTE i,k;
  //
  // Adjust the maximum and the level shift to that of the original input.
  pmax = ((max + 1) << m_ucPointShift) - 1;
  //
  // Pull the data once to collect the original. No shifting here.
  CreateTrivialTrafo(ctrafo,8 + m_ucPointShift)->RGB2YCbCr(r,source,(pmax + 1) >> 1,pmax);
  //
  AllocateBuffers();
  //
  // Reconstruct into the transformed YCbCr buffer of the color transformer.
  for(i = 0;i < m_ucCount;i++) {
    dtrafo[i]->InverseTransformBlock(ctrafo->BufferOf()[i],dct[i],(max + 1) >> 1);
  }

  for(i = 0;i < m_ucCount;i++) {
    LONG *dest                             = m_ppReconstructed[i];
    m_ppTransformed[i]->ibm_pData          = dest;
    m_ppTransformed[i]->ibm_cBytesPerPixel =     sizeof(LONG);
    m_ppTransformed[i]->ibm_lBytesPerRow   = 8 * sizeof(LONG);
    m_ppTransformed[i]->ibm_ulWidth        = 8;
    m_ppTransformed[i]->ibm_ulHeight       = 8;
    m_ppTransformed[i]->ibm_ucPixelType    = CTYP_LONG;
    memset(dest,0,64 * sizeof(LONG));
  }
  //
  // Inverse transform into this buffer, i.e. into its own buffer, overwriting the
  // content there.
  ctrafo->YCbCr2RGB(r,m_ppTransformed,(max + 1) >> 1,max);
  //
  // Compute now the residual
  for(i = 0;i < m_ucCount;i++) { 
    LONG *rec = m_ppReconstructed[i];
    LONG *org = m_pOriginalData->BufferOf()[i]; // the original data.
    LONG *res = residual[i]; // the residual buffer.
    for(k = 0;k < 64;k++) {
      res[k] = org[k] - rec[k];
    }
  }
  //
  // If there are three or more components, run an RCT on the first three
  // of them.
  if (m_ucCount >= 3) {
    LONG *r = residual[0]; // the residual buffer.
    LONG *g = residual[1];
    LONG *b = residual[2];
    for(k = 0;k < 64;k++) {
       LONG y  = (r[k] + (g[k] << 1) + b[k]) >> 2;
       LONG cb = b[k] - g[k];
       LONG cr = r[k] - g[k];
       r[k]    = y;
       g[k]    = cb;
       b[k]    = cr;
    }
  }
  //
#if 0
  //
  // A test: Measure the variance in each 4x4 subblock in the original image, if all of them are 
  // above a threshold, drop the residual due to masking.
  if (m_ucMaxError > 0) {
    for(i = 0;i < m_ucCount;i++) { 
      LONG thres = (m_ucMaxError + 1) * (m_ucMaxError + 1);
      LONG *org  = m_pOriginalData->BufferOf()[i]; // the original data.
      LONG *res  = residual[i]; // the residual buffer.
      int count  = 0;
      for(int yoff = 0; yoff < 8;yoff += 2) {
	for (int xoff = 0;xoff < 8;xoff += 2) {
	  if (Variance2x2(org + xoff + yoff * 8) > thres) {
	    ClearSubblock2x2(res + xoff + yoff * 8);
	    count++;
	  }
	}
      }
      if (count < 3 || count > 11) {
	ClearSubblock8x8(res);
      }
    }
  }
#endif
  //
  if (BuildTransformations()) {
    //
    // Finally, run a Hadamard transformation on the data.
    for(i = 0;i < m_ucCount;i++) {
      m_ppTrafo[i]->TransformBlock(residual[i],residual[i],0);
    }
  } else {
    // No transformation.
    for(i = 0;i < m_ucCount;i++) {
      UWORD quant = (i > 0 && i < 3)?(m_usChromaQuant):(m_usLumaQuant); 
      LONG *res   = residual[i]; // the residual buffer.
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
	      v      = res[p];
	      if (noiseshaping)
		v     += error;
	      qnt    = v / quant;
	      error += res[p] - quant * qnt;
	      res[p] = qnt;
	    }
	  }
	}
      }
    }
  }  
  //
#ifdef TEST_RESIDUALS
  {
    LONG x = r.ra_MinX;
    LONG y = r.ra_MinY;
    LONG xmax = r.ra_MaxX & 7;
    LONG ymax = r.ra_MaxY & 7;
    UBYTE i;
    LONG xp,yp;
    for(i = 0;i < m_ucCount;i++) {
      char resname[60];
      char orgname[60];
      sprintf(resname,"/tmp/residual_%d_%d_%d",x,y,i);
      sprintf(orgname,"/tmp/original_%d_%d_%d",x,y,i);
      FILE *res = fopen(resname,"wb");
      FILE *org = fopen(orgname,"wb");
      for(yp = 0;yp <= ymax;yp++) {
	for(xp = 0;xp <= xmax;xp++) {
	  fprintf(res,"%d ",residual[i][xp + (yp << 3)]);
	  fprintf(org,"%d ",m_ppReconstructed[i][xp + (yp << 3)]);
	}
	fprintf(res,"\n");
	fprintf(org,"\n");
      }
      fclose(org);
      fclose(res);
    }
  }
#endif
  //
}
///
