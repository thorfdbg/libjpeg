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
** This class pulls blocks from the frame and reconstructs from those
** quantized block lines or encodes from them.
**
** $Id: blockbitmaprequester.cpp,v 1.23 2012-09-15 21:45:51 thor Exp $
**
*/

/// Includes
#include "control/bitmapctrl.hpp"
#include "control/blockbitmaprequester.hpp"
#include "control/residualblockhelper.hpp"
#include "interface/imagebitmap.hpp"
#include "upsampling/upsamplerbase.hpp"
#include "upsampling/downsamplerbase.hpp"
#include "coding/quantizedrow.hpp"
#include "codestream/tables.hpp"
#include "codestream/rectanglerequest.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "dct/dct.hpp"
#include "colortrafo/colortrafo.hpp"
#include "std/string.hpp"
///

/// BlockBitmapRequester::BlockBitmapRequester
BlockBitmapRequester::BlockBitmapRequester(class Frame *frame)
  : BlockBuffer(frame), BitmapCtrl(frame), m_pEnviron(frame->EnvironOf()), m_pFrame(frame),
    m_pulReadyLines(NULL),
    m_ppDownsampler(NULL), m_ppUpsampler(NULL),
    m_ppTempIBM(NULL), m_ppQTemp(NULL), m_ppRTemp(NULL),
    m_pppQImage(NULL), m_pppRImage(NULL),
    m_pResidualHelper(NULL), m_bSubsampling(false)
{  
  m_ucCount       = frame->DepthOf(); 
  m_ulPixelWidth  = frame->WidthOf();
  m_ulPixelHeight = frame->HeightOf();
}
///

/// BlockBitmapRequester::~BlockBitmapRequester
BlockBitmapRequester::~BlockBitmapRequester(void)
{
  UBYTE i;

  if (m_ppDownsampler) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppDownsampler[i];
    }
    m_pEnviron->FreeMem(m_ppDownsampler,m_ucCount * sizeof(class DownsamplerBase *));
  }

  if (m_ppUpsampler) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppUpsampler[i];
    }
    m_pEnviron->FreeMem(m_ppUpsampler,m_ucCount * sizeof(class UpsamplerBase *));
  }

  if (m_ppTempIBM) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppTempIBM[i];
    }
    m_pEnviron->FreeMem(m_ppTempIBM,m_ucCount * sizeof(struct ImageBitMap *));
  }
  
  if (m_pulReadyLines)
    m_pEnviron->FreeMem(m_pulReadyLines,m_ucCount * sizeof(ULONG));

  if (m_pppQImage)
    m_pEnviron->FreeMem(m_pppQImage,m_ucCount * sizeof(class QuantizedRow **));

  if (m_pppRImage)
    m_pEnviron->FreeMem(m_pppRImage,m_ucCount * sizeof(class QuantizedRow **));

  if (m_ppQTemp)
    m_pEnviron->FreeMem(m_ppQTemp,m_ucCount * sizeof(class QuantizedRow **));

  if (m_ppRTemp)
    m_pEnviron->FreeMem(m_ppRTemp,m_ucCount * sizeof(class QuantizedRow **));

}
///

/// BlockBitmapRequester::BuildCommon
// Build common structures for encoding and decoding
void BlockBitmapRequester::BuildCommon(void)
{
  UBYTE i;

  BitmapCtrl::BuildCommon();
  BlockBuffer::BuildCommon();

  if (m_ppDCT == NULL) {
    m_ppDCT = (class DCT **)m_pEnviron->AllocMem(sizeof(class DCT *) * m_ucCount);
    memset(m_ppDCT,0,sizeof(class DCT *) * m_ucCount);
  }

  if (m_ppTempIBM == NULL) {
    m_ppTempIBM = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap **) * m_ucCount);
    memset(m_ppTempIBM,0,sizeof(struct ImageBitMap *) * m_ucCount);
  }

  if (m_pulY == NULL) {
    m_pulY        = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    memset(m_pulY,0,sizeof(ULONG) * m_ucCount);
  }
  
  if (m_pulCurrentY == NULL) {
    m_pulCurrentY = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    memset(m_pulCurrentY,0,sizeof(ULONG) * m_ucCount);
  }

  if (m_pulReadyLines == NULL) {
    m_pulReadyLines = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    memset(m_pulReadyLines,0,sizeof(ULONG) * m_ucCount);
  }
  
  if (m_pppQImage == NULL) 
    m_pppQImage   = (class QuantizedRow ***)m_pEnviron->AllocMem(sizeof(class QuantizedRow **) * 
								 m_ucCount);

  if (m_pppRImage == NULL)
    m_pppRImage   = (class QuantizedRow ***)m_pEnviron->AllocMem(sizeof(class QuantizedRow **) * 
								 m_ucCount);

  if (m_ppQTemp == NULL)
    m_ppQTemp     = (LONG **)m_pEnviron->AllocMem(sizeof(LONG *) * m_ucCount);

  if (m_ppRTemp == NULL)
    m_ppRTemp     = (LONG **)m_pEnviron->AllocMem(sizeof(LONG *) * m_ucCount);
  
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_ppTempIBM[i] == NULL)
      m_ppTempIBM[i]      = new(m_pEnviron) struct ImageBitMap();

    m_pppQImage[i]        = m_ppQTop + i;
    m_pppRImage[i]        = m_ppRTop + i;
  }
}
///

/// BlockBitmapRequester::PrepareForEncoding
// First time usage: Collect all the information
void BlockBitmapRequester::PrepareForEncoding(void)
{  
  UBYTE i;
  
  BuildCommon();
  
  // Build the DCT transformers.
  ResetToStartOfScan(NULL);

  if (m_ppDownsampler == NULL) {
    m_ppDownsampler = (class DownsamplerBase **)m_pEnviron->AllocMem(sizeof(class DownsamplerBase *) * m_ucCount);
    memset(m_ppDownsampler,0,sizeof(class DownsamplerBase *) * m_ucCount);
    
    for(i = 0;i < m_ucCount;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE sx = comp->SubXOf();
      UBYTE sy = comp->SubYOf();
      
      if (sx > 1 || sy > 1) {
	m_ppDownsampler[i] = DownsamplerBase::CreateDownsampler(m_pEnviron,sx,sy,
								m_ulPixelWidth,m_ulPixelHeight);
	m_bSubsampling     = true;
      }
    }
  }
}
///

/// BlockBitmapRequester::PrepareForDecoding
// First time usage: Collect all the information
void BlockBitmapRequester::PrepareForDecoding(void)
{  
  UBYTE i;
  
  BuildCommon();

  if (m_ppUpsampler == NULL) {
    m_ppUpsampler = (class UpsamplerBase **)m_pEnviron->AllocMem(sizeof(class UpsamplerBase *) * m_ucCount);
    memset(m_ppUpsampler,0,sizeof(class Upsampler *) * m_ucCount);
    
    for(i = 0;i < m_ucCount;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE sx = comp->SubXOf();
      UBYTE sy = comp->SubYOf();
      
      if (sx > 1 || sy > 1) {
	m_ppUpsampler[i] = UpsamplerBase::CreateUpsampler(m_pEnviron,sx,sy,
							  m_ulPixelWidth,m_ulPixelHeight);
	m_bSubsampling   = true;
      }
    }
  }
}
///

/// BlockBitmapRequester::ResetToStartOfImage
// Reset all components on the image side of the control to the
// start of the image. Required when re-requesting the image
// for encoding or decoding.
void BlockBitmapRequester::ResetToStartOfImage(void)
{
  for(UBYTE i = 0;i < m_ucCount;i++) {
    m_pppQImage[i]     = &m_ppQTop[i];
    m_pppRImage[i]     = &m_ppRTop[i];
    m_pulReadyLines[i] = 0;
  }
}
///


/// BlockBitmapRequester::ColorTrafoOf
// Return the color transformer responsible for this scan.
class ColorTrafo *BlockBitmapRequester::ColorTrafoOf(bool encoding)
{
  return m_pFrame->TablesOf()->ColorTrafoOf(m_pFrame,
					    PixelTypeOf(),m_pFrame->HiddenPrecisionOf(),m_ucCount,encoding);
}
///

/// BlockBitmapRequester::BuildImageRow
// Create the next row of the image such that m_pppImage[i] is valid.
class QuantizedRow *BlockBitmapRequester::BuildImageRow(class QuantizedRow **qrow,int i)
{
  if (*qrow == NULL) {
    class Component *comp = m_pFrame->ComponentOf(i);
    UBYTE subx      = comp->SubXOf();
    ULONG width     = (m_ulPixelWidth  + subx - 1) / subx;
    *qrow = new(m_pEnviron) class QuantizedRow(m_pEnviron);
    (*qrow)->AllocateRow(width);
  }
  return *qrow;
}
///

/// BlockBitmapRequester::EncodeRegion
// Encode a region without downsampling but color transformation
void BlockBitmapRequester::EncodeRegion(class BitMapHook *bmh,const struct RectangleRequest *)
{
  ULONG maxmcu                 = MAX_ULONG;
  ULONG maxval                 = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  class ColorTrafo *ctrafo;
  RectAngle<LONG> region;
  int i;
  //
  // Install the full image region. The code cannot encode just a part of the
  // image and leave the rest undefined.
  region.ra_MinX = 0;
  region.ra_MinY = m_ulPixelHeight;
  region.ra_MaxX = m_ulPixelWidth  - 1;
  region.ra_MaxY = m_ulPixelHeight - 1;
  //
  // Find the region to request.
  for(i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < ULONG(region.ra_MinY))
      region.ra_MinY = m_pulReadyLines[i];
  }
  for(i = 0;i < m_ucCount;i++) {
    ULONG max;
    //
    // Components are always requested completely on encoding.
    RequestUserData(bmh,region,i);
    // All components must have the same sample precision here.
    max = (m_ppBitmap[i]->ibm_ulHeight - 1) >> 3;
    if (max < maxmcu)
      maxmcu = max; 
    if (LONG(m_ppBitmap[i]->ibm_ulHeight) - 1 < region.ra_MaxY)
      region.ra_MaxY = m_ppBitmap[i]->ibm_ulHeight - 1;
  }
  
  // 
  // Now that the pixel type is known, request the color transformer.
  ctrafo = ColorTrafoOf(true);

  if (m_bSubsampling) { 
    RectAngle<LONG> r;
    ULONG minx   = region.ra_MinX >> 3;
    ULONG maxx   = region.ra_MaxX >> 3;
    ULONG miny   = region.ra_MinY >> 3;
    ULONG maxy   = region.ra_MaxY >> 3;
    ULONG x,y;
    LONG *const *buffer      = ctrafo->BufferOf();
    
    assert(maxy <= maxmcu);
    //
    // First part: Collect the data from
    // the user and push it into the color transformer buffer.
    // For that first build the downsampler.
    for(i = 0;i < m_ucCount;i++) {
      if (m_ppDownsampler[i]) {
	m_ppDownsampler[i]->SetBufferedRegion(region);
      }
    }
    
    for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
      r.ra_MaxY = (r.ra_MinY & -8) + 7;
      if (r.ra_MaxY > region.ra_MaxY)
	r.ra_MaxY = region.ra_MaxY;
	
      for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
	r.ra_MaxX = (r.ra_MinX & -8) + 7;
	if (r.ra_MaxX > region.ra_MaxX)
	  r.ra_MaxX = region.ra_MaxX;
	
	for(i = 0;i < m_ucCount;i++) {
	  // Collect the source data.
	  ExtractBitmap(m_ppTempIBM[i],r,i);
	}
	
	//
	// Run the color transformer.
	ctrafo->RGB2YCbCr(r,m_ppTempIBM,(maxval + 1) >> 1,maxval);
	
	// Now push the transformed data into either the downsampler, 
	// or the forward DCT block row.
	for(i = 0;i < m_ucCount;i++) {
	  if (m_ppDownsampler[i]) {
	    // Just collect the data in the downsampler for the time
	    // being. Will be taken care of as soon as it is complete.
	    m_ppDownsampler[i]->DefineRegion(x,y,buffer[i]);
	  } else { 
	    class QuantizedRow *qrow = BuildImageRow(m_pppQImage[i],i);
	    LONG *dst                = qrow->BlockAt(x)->m_Data;
	    m_ppDCT[i]->TransformBlock(buffer[i],dst,(maxval + 1) >> 1);
	  }
	}
      }
      //
      // Advance the quantized rows for the non-subsampled components,
      // downsampled components will be advanced later.
      for(i = 0;i < m_ucCount;i++) {
	m_pulReadyLines[i]    += 8; // somehwere in the buffer.
	if (m_ppDownsampler[i] == NULL) {
	  class QuantizedRow *qrow = BuildImageRow(m_pppQImage[i],i);
	  m_pppQImage[i] = &(qrow->NextOf());
	} else {
	  LONG bx,by;
	  RectAngle<LONG> blocks;
	  // Collect the downsampled blocks and push that into the DCT.
	  m_ppDownsampler[i]->GetCollectedBlocks(blocks);
	  for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
	    class QuantizedRow *qr = BuildImageRow(m_pppQImage[i],i);
	    for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
	      LONG src[64]; // temporary buffer, the DCT requires a 8x8 block
	      LONG *dst = (qr)?(qr->BlockAt(bx)->m_Data):NULL;
	      m_ppDownsampler[i]->DownsampleRegion(bx,by,src);
	      m_ppDCT[i]->TransformBlock(src,dst,(maxval + 1) >> 1);
	    }
	    m_ppDownsampler[i]->RemoveBlocks(by);
	    m_pppQImage[i] = &(qr->NextOf());
	  }
	}
      }
    }
  } else { // No downsampling required, residual coding possible.
    RectAngle<LONG> r;
    ULONG minx   = region.ra_MinX >> 3;
    ULONG maxx   = region.ra_MaxX >> 3;
    ULONG miny   = region.ra_MinY >> 3;
    ULONG maxy   = region.ra_MaxY >> 3;
    ULONG x,y;

    assert(maxy <= maxmcu);
    
    for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
      r.ra_MaxY = (r.ra_MinY & -8) + 7;
      if (r.ra_MaxY > region.ra_MaxY)
	r.ra_MaxY = region.ra_MaxY;
	
      for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
	r.ra_MaxX = (r.ra_MinX & -8) + 7;
	if (r.ra_MaxX > region.ra_MaxX)
	  r.ra_MaxX = region.ra_MaxX;

	for(i = 0;i < m_ucCount;i++) {      
	  ExtractBitmap(m_ppTempIBM[i],r,i);
	}
	
	ctrafo->RGB2YCbCr(r,m_ppTempIBM,(maxval + 1) >> 1,maxval);

	for(i = 0;i < m_ucCount;i++) {
	  class QuantizedRow *qrow = BuildImageRow(m_pppQImage[i],i);
	  LONG *dst = qrow->BlockAt(x)->m_Data;
	  LONG *src = ctrafo->BufferOf()[i];
	  
	  m_ppDCT[i]->TransformBlock(src,dst,(maxval + 1) >> 1);
	}
	
	//
	// If any residuals are required, compute them now.
	if (m_pResidualHelper) {
	  for(i = 0;i < m_ucCount;i++) { 
	    class QuantizedRow *qrow = *m_pppQImage[i];
	    class QuantizedRow *rrow = BuildImageRow(m_pppRImage[i],i);
	    assert(qrow && rrow);
	    m_ppQTemp[i] = qrow->BlockAt(x)->m_Data;
	    m_ppRTemp[i] = rrow->BlockAt(x)->m_Data;
	  }
	  m_pResidualHelper->ComputeResiduals(r,this,m_ppTempIBM,m_ppQTemp,m_ppRTemp,maxval);
	}
      }
      for(i = 0;i < m_ucCount;i++) {
	class QuantizedRow *qrow = *m_pppQImage[i];
	class QuantizedRow *rrow = *m_pppRImage[i];
	m_pppQImage[i] = &(qrow->NextOf());
	if (rrow) m_pppRImage[i] = &(rrow->NextOf()); // the residual is optional
	assert(m_pResidualHelper == NULL || rrow);
	m_pulReadyLines[i]   += 8;
      }
    }
  }
       
  for(i = 0;i < m_ucCount;i++) {
    ReleaseUserData(bmh,region,i);
  }
}
///

/// BlockBitmapRequester::ReconstructRegion
// Reconstruct a block, or part of a block
void BlockBitmapRequester::ReconstructRegion(class BitMapHook *bmh,const struct RectangleRequest *rr)
{
  ULONG maxmcu  = MAX_ULONG;
  UBYTE i;
  ULONG maxval  = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  RectAngle<LONG> region = rr->rr_Request;
  class ColorTrafo *ctrafo;


  ClipToImage(region);
  
  for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    RequestUserData(bmh,region,i);
    ULONG max = (BitmapOf(i).ibm_ulHeight >> 3) - 1;
    if (max < maxmcu)
      maxmcu = max;
  }

  //
  // Get the color tranformer. Must delay this until here now that we have the pixel type.
  //
  ctrafo = ColorTrafoOf(false);
  if (m_bSubsampling) { 
    for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE subx            = comp->SubXOf();
      UBYTE suby            = comp->SubYOf();
      class UpsamplerBase *up;  // upsampler
      LONG bx,by;
      RectAngle<LONG> blocks;
      //
      // Compute the region of blocks
      assert(subx > 0 && suby > 0);
      if ((up = m_ppUpsampler[i])) {
	LONG bwidth           = ((m_ulPixelWidth  + subx - 1) / subx + 7) >> 3;
	LONG bheight          = ((m_ulPixelHeight + suby - 1) / suby + 7) >> 3;
	LONG rx               = (subx > 1)?(1):(0);
	LONG ry               = (suby > 1)?(1):(0);
	// The +/-1 include additional lines required for subsampling expansion
	blocks.ra_MinX        = ((region.ra_MinX / subx - rx) >> 3);
	blocks.ra_MaxX        = ((region.ra_MaxX / subx + rx) >> 3);
	blocks.ra_MinY        = ((region.ra_MinY / suby - ry) >> 3);
	blocks.ra_MaxY        = ((region.ra_MaxY / suby + ry) >> 3);
	// Clip.
	if (blocks.ra_MinX < 0)        blocks.ra_MinX = 0;
	if (blocks.ra_MaxX >= bwidth)  blocks.ra_MaxX = bwidth - 1;
	if (blocks.ra_MinY < 0)        blocks.ra_MinY = 0;
	if (blocks.ra_MaxY >= bheight) blocks.ra_MaxY = bheight - 1;
	up->SetBufferedRegion(blocks); // also removes the rectangle of blocks already buffered.
	//
	for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
	  class QuantizedRow *qrow = *m_pppQImage[i];
	  for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
	    LONG *src = (qrow)?(qrow->BlockAt(bx)->m_Data):NULL;
	    LONG dst[64];
	    m_ppDCT[i]->InverseTransformBlock(dst,src,(maxval + 1) >> 1);
	    up->DefineRegion(bx,by,dst);
	  }
	  if (qrow) m_pppQImage[i] = &(qrow->NextOf());
	}
      }
    }
    // Now push blocks into the color transformer from the upsampler.
    {
      RectAngle<LONG> r;
      ULONG minx   = region.ra_MinX >> 3;
      ULONG maxx   = region.ra_MaxX >> 3;
      ULONG miny   = region.ra_MinY >> 3;
      ULONG maxy   = region.ra_MaxY >> 3;
      ULONG x,y;
      LONG *const *buffer      = ctrafo->BufferOf();
      
      if (maxy > maxmcu)
	maxy = maxmcu;

      for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
	r.ra_MaxY = (r.ra_MinY & -8) + 7;
	if (r.ra_MaxY > region.ra_MaxY)
	  r.ra_MaxY = region.ra_MaxY;
	
	for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
	  r.ra_MaxX = (r.ra_MinX & -8) + 7;
	  if (r.ra_MaxX > region.ra_MaxX)
	    r.ra_MaxX = region.ra_MaxX;
	  
	  for(i = 0;i < m_ucCount;i++) {
	    if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
	      ExtractBitmap(m_ppTempIBM[i],r,i);
	      if (m_ppUpsampler[i]) {
		// Upsampled case, take from the upsampler, transform
		// into the color buffer.
		m_ppUpsampler[i]->UpsampleRegion(r,buffer[i]);
	      } else {
		class QuantizedRow *qrow = *m_pppQImage[i];
		LONG *src = (qrow)?(qrow->BlockAt(x)->m_Data):NULL;
		// Plain case. Transform directly into the color buffer.
		m_ppDCT[i]->InverseTransformBlock(buffer[i],src,(maxval + 1) >> 1);
	      }
	    } else {
	      // Not requested, zero the buffer.
	      memset(buffer[i],0,sizeof(LONG) * 64);
	    }
	  }
	  ctrafo->YCbCr2RGB(r,m_ppTempIBM,(maxval + 1) >> 1,maxval);
	}
	//
	// Advance the quantized rows for the non-subsampled components,
	// upsampled components have been advanced above.
	for(i = 0;i < m_ucCount;i++) {
	  if (m_ppUpsampler[i] == NULL) {
	    class QuantizedRow *qrow = *m_pppQImage[i];
	    if (qrow) m_pppQImage[i] = &(qrow->NextOf());
	  }
	}
      }
    }
  } else { // direct case, no upsampling required, residual coding possible.
    RectAngle<LONG> r;
    class ColorTrafo *ctrafo = ColorTrafoOf(false);
    ULONG minx   = region.ra_MinX >> 3;
    ULONG maxx   = region.ra_MaxX >> 3;
    ULONG miny   = region.ra_MinY >> 3;
    ULONG maxy   = region.ra_MaxY >> 3;
    ULONG x,y;
      
    if (maxy > maxmcu)
      maxy = maxmcu;

    for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
      r.ra_MaxY = (r.ra_MinY & -8) + 7;
      if (r.ra_MaxY > region.ra_MaxY)
	r.ra_MaxY = region.ra_MaxY;
	
      for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
	r.ra_MaxX = (r.ra_MinX & -8) + 7;
	if (r.ra_MaxX > region.ra_MaxX)
	  r.ra_MaxX = region.ra_MaxX;

	for(i = 0;i < m_ucCount;i++) {      
	  LONG *dst = ctrafo->BufferOf()[i];
	  if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
	    class QuantizedRow *qrow = *m_pppQImage[i];
	    LONG *src = (qrow)?(qrow->BlockAt(x)->m_Data):(NULL);
	    //
	    ExtractBitmap(m_ppTempIBM[i],r,i);
	    m_ppDCT[i]->InverseTransformBlock(dst,src,(maxval + 1) >> 1);
	  } else {
	    memset(dst,0,sizeof(LONG) * 64);
	  }
	}
	//
	// Perform the color transformation now.
	ctrafo->YCbCr2RGB(r,m_ppTempIBM,(maxval + 1) >> 1,maxval);
	if (m_pResidualHelper) {
	  for(i = rr->rr_usFirstComponent; i <= rr->rr_usLastComponent; i++) {
	    class QuantizedRow *rrow = *m_pppRImage[i];
	    m_ppRTemp[i] = (rrow)?(rrow->BlockAt(x)->m_Data):(NULL);
	  }
	  m_pResidualHelper->AddResidual(r,m_ppTempIBM,m_ppRTemp,maxval);
	}
      } // of loop over x
      //
      // Advance the rows.
      for(i = 0;i < m_ucCount;i++) {
	class QuantizedRow *qrow = *m_pppQImage[i];
	class QuantizedRow *rrow = *m_pppRImage[i];
	if (qrow) m_pppQImage[i] = &(qrow->NextOf());
	if (rrow) m_pppRImage[i] = &(rrow->NextOf());
      }
    }
  }

  for(i = rr->rr_usFirstComponent;i <=rr->rr_usLastComponent;i++) {
    ReleaseUserData(bmh,region,i);
  }
}
///

/// BlockBitmapRequester::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder.
bool BlockBitmapRequester::isNextMCULineReady(void) const
{
  int i;

  for(i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_ulPixelHeight) { // There is still data to encode
      class Component *comp = m_pFrame->ComponentOf(i);
      ULONG codedlines      = m_pulCurrentY[i] * comp->SubYOf();
      // codedlines + comp->SubYOf() << 3 * comp->MCUHeightOf() is the number of 
      // lines that must be buffered to encode the next MCU
      if (m_pulReadyLines[i] < codedlines + (comp->SubYOf() << 3) * comp->MCUHeightOf())
	return false;
    }
  }

  return true;
}
///

/// BlockBitmapRequester::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool BlockBitmapRequester::isImageComplete(void) const
{
  for(UBYTE i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_ulPixelHeight)
      return false;
  }
  return true;
}
///
