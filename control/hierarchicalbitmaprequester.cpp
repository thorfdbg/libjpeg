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
** This is the top-level bitmap requester that distributes data to
** image scales on encoding, and collects data from image scales on
** decoding. It also keeps the top-level color transformer and the
** toplevel subsampling expander.
**
** $Id: hierarchicalbitmaprequester.cpp,v 1.46 2023/02/21 09:17:33 thor Exp $
**
*/

/// Includes
#include "control/hierarchicalbitmaprequester.hpp"
#include "control/lineadapter.hpp"
#include "control/linemerger.hpp"
#include "std/string.hpp"
#include "upsampling/downsamplerbase.hpp"
#include "upsampling/upsamplerbase.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "colortrafo/colortrafo.hpp"
#include "codestream/rectanglerequest.hpp"
#include "codestream/tables.hpp"
///

/// HierarchicalBitmapRequester::HierarchicalBitmapRequester
// Construct from a frame - the frame is just a "dummy frame"
// that contains the dimensions, actually a DHP marker segment
// without any data in it.
HierarchicalBitmapRequester::HierarchicalBitmapRequester(class Frame *dimensions)
  : BitmapCtrl(dimensions)
#if ACCUSOFT_CODE
  , m_ppDownsampler(NULL), m_ppUpsampler(NULL), 
    m_ppTempIBM(NULL), m_pSmallestScale(NULL), m_pLargestScale(NULL), m_pTempAdapter(NULL),
    m_pulReadyLines(NULL), m_pulY(NULL), m_pulHeight(NULL),
    m_ppEncodingMCU(NULL), m_ppDecodingMCU(NULL),
    m_bSubsampling(false)
#endif
{
}
///

/// HierarchicalBitmapRequester::~HierarchicalBitmapRequester
HierarchicalBitmapRequester::~HierarchicalBitmapRequester(void)
{
#if ACCUSOFT_CODE
  class LineAdapter *la;
  UBYTE i;
  
  if (m_ppEncodingMCU) {
    assert(m_pLargestScale);
    for(i = 0;i < (m_ucCount << 3);i++) {
      m_pLargestScale->DropLine(m_ppEncodingMCU[i],i >> 3);
    }
    m_pEnviron->FreeMem(m_ppEncodingMCU,sizeof(struct Line *) * m_ucCount * 8);
  }

  if (m_ppDecodingMCU) {
    assert(m_pLargestScale);
    for(i = 0;i < (m_ucCount << 3);i++) {
      m_pLargestScale->ReleaseLine(m_ppDecodingMCU[i],i >> 3);
    }
    m_pEnviron->FreeMem(m_ppDecodingMCU,sizeof(struct Line *) * m_ucCount * 8);
  }

  //
  // Dispose the tree of line adapters
  while((la = m_pLargestScale)) {
    m_pLargestScale = la->LowPassOf();
    delete la->HighPassOf();
    delete la;
  }
  delete m_pTempAdapter;
  
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

  if (m_pulY)
    m_pEnviron->FreeMem(m_pulY,m_ucCount * sizeof(ULONG));

  if (m_pulHeight)
    m_pEnviron->FreeMem(m_pulHeight,m_ucCount * sizeof(ULONG));
#endif
}
///

/// HierarchicalBitmapRequester::BuildCommon
// Build common structures for encoding and decoding
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::BuildCommon(void)
{
  BitmapCtrl::BuildCommon();
  if (m_ppTempIBM == NULL) {
    m_ppTempIBM = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap **) * m_ucCount);
    memset(m_ppTempIBM,0,sizeof(struct ImageBitMap *) * m_ucCount);
    for (UBYTE i = 0;i < m_ucCount;i++) {
      m_ppTempIBM[i] = new(m_pEnviron) struct ImageBitMap();
    }
  }

  if (m_pulReadyLines == NULL) {
    m_pulReadyLines = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    memset(m_pulReadyLines,0,sizeof(ULONG) * m_ucCount);
  }

  if (m_pulY == NULL) {
    m_pulY = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    memset(m_pulY,0,sizeof(ULONG) * m_ucCount);
  }

  if (m_pulHeight == NULL) {
    m_pulHeight = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    for(UBYTE i = 0;i < m_ucCount;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE suby            = comp->SubYOf();
      m_pulHeight[i]        = (m_ulPixelHeight + suby - 1) / suby;
    }
  }
}
#endif
///

/// HierarchicalBitmapRequester::PrepareForEncoding
// First time usage: Collect all the information for encoding.
// May throw on out of memory situations
void HierarchicalBitmapRequester::PrepareForEncoding(void)
{
#if ACCUSOFT_CODE
  
  BuildCommon();

  if (m_ppEncodingMCU == NULL) {
    m_ppEncodingMCU = (struct Line **)m_pEnviron->AllocMem(sizeof(struct Line *) * m_ucCount *8);
    memset(m_ppEncodingMCU,0,sizeof(struct Line *) * m_ucCount * 8);
  }
  
  if (m_ppDownsampler == NULL) {
    m_ppDownsampler = (class DownsamplerBase **)m_pEnviron->AllocMem(sizeof(class DownsamplerBase *) * m_ucCount);
    memset(m_ppDownsampler,0,sizeof(class DownsamplerBase *) * m_ucCount);
    
    for(UBYTE i = 0;i < m_ucCount;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE sx = comp->SubXOf();
      UBYTE sy = comp->SubYOf();

      if (sx > 1 || sy > 1) {
        m_ppDownsampler[i] = DownsamplerBase::CreateDownsampler(m_pEnviron,sx,sy,
                                                                m_ulPixelWidth,m_ulPixelHeight,
                                                                m_pFrame->TablesOf()->
                                                                isDownsamplingInterpolated());
        m_bSubsampling     = true;
      }
    }
  }

  if (m_pLargestScale)
    m_pLargestScale->PrepareForEncoding();
#endif
}
///

/// HierarchicalBitmapRequester::PrepareForDecoding
// First time usage: Collect all the information for encoding.
// May throw on out of memory situations
void HierarchicalBitmapRequester::PrepareForDecoding(void)
{
#if ACCUSOFT_CODE

  UBYTE i;

  BuildCommon();

  if (m_ppDecodingMCU == NULL) {
    m_ppDecodingMCU = (struct Line **)m_pEnviron->AllocMem(sizeof(struct Line *) * m_ucCount*8);
    memset(m_ppDecodingMCU,0,sizeof(struct Line *) * m_ucCount * 8);
  }

  if (m_ppUpsampler == NULL) {
    m_ppUpsampler = (class UpsamplerBase **)m_pEnviron->AllocMem(sizeof(class UpsamplerBase *) * m_ucCount);
    memset(m_ppUpsampler,0,sizeof(class Upsampler *) * m_ucCount);

    for(i = 0;i < m_ucCount;i++) {
      class Component *comp = m_pFrame->ComponentOf(i);
      UBYTE sx = comp->SubXOf();
      UBYTE sy = comp->SubYOf();

      if (m_pLargestScale) {
        class Frame *frame = m_pLargestScale->FrameOf();
        while(frame) {
          if (frame->ComponentOf(i)->SubXOf() != sx || frame->ComponentOf(i)->SubYOf() != sy)
            JPG_THROW(MALFORMED_STREAM,"HierarchicalBitmapRequester::PrepareForDecoding",
                      "component subsampling is inconsistent across hierarchical levels");
          frame = frame->NextOf();
        }
      }

      if (sx > 1 || sy > 1) {
        m_ppUpsampler[i] = UpsamplerBase::CreateUpsampler(m_pEnviron,sx,sy,
                                                          m_ulPixelWidth,m_ulPixelHeight,
                                                          m_pFrame->TablesOf()->isChromaCentered());
        m_bSubsampling   = true;
      }
    }
  }

  if (m_pLargestScale)
    m_pLargestScale->PrepareForDecoding();
#endif
}
///

/// HierarchicalBitmapRequester::ColorTrafoOf
// Return the color transformer responsible for this scan.
class ColorTrafo *HierarchicalBitmapRequester::ColorTrafoOf(bool encoding,bool disabletorgb)
{
  return m_pFrame->TablesOf()->ColorTrafoOf(m_pFrame,NULL,PixelTypeOf(),
                                            encoding,disabletorgb);
}
///

/// HierarchicalBitmapRequester::AddImageScale
// As soon as a frame is parsed off, or created: Add another scale to the image.
// The boolean arguments identify whether the reference frame, i.e. what is
// buffered already from previous frames, will be expanded.
void HierarchicalBitmapRequester::AddImageScale(class Frame *frame,bool expandh,bool expandv)
{
#if ACCUSOFT_CODE
  if (m_pLargestScale == NULL) {
    assert(m_pSmallestScale == NULL);
    assert(expandh == false && expandv == false);
    // Actually, this is the smallest scale... as it is the first we build.
    m_pLargestScale  = frame->BuildLineAdapter();
    m_pSmallestScale = m_pLargestScale;
    frame->SetImageBuffer(m_pLargestScale);
  } else {
    class LineMerger *merger;
    // Two things need to be build: The adapter to the new band, and the merger
    // that merges this band with the output and scales the result
    // appropriately.
    assert(m_pTempAdapter == NULL);
    // This object will pull out lines from the new high-pass...
    m_pTempAdapter   = frame->BuildLineAdapter();
    // ...and this guy will merge them with what we currently have.
    merger           = new(m_pEnviron) class LineMerger(frame,m_pLargestScale,m_pTempAdapter,
                                                        expandh,expandv);
    //
    // And this becomes the next largest scale.
    m_pLargestScale  = merger;
    // and controls now the life-time of its children.
    frame->SetImageBuffer(m_pTempAdapter);
    m_pTempAdapter   = NULL; 
  }
#else
  NOREF(frame);
  NOREF(expandh);
  NOREF(expandv);
#endif
}
///

/// HierarchicalBitmapRequester::GenerateDifferentialImage
// After having written the previous image, compute the differential from the downscaled
// and-re-upscaled version and push it into the next frame, collect the
// residuals, make this frame ready for encoding, and retrieve the downscaling
// data.
void HierarchicalBitmapRequester::GenerateDifferentialImage(class Frame *target,
                                                            bool &hexp,bool &vexp)
{
#if ACCUSOFT_CODE
  class LineAdapter *lap = m_pLargestScale;

  while(lap) {
    // The target frame must be one of the high-passes. The frame of a line
    // adapter is that of the high-pass, so we can check for it.
    if (lap->HighPassOf()->FrameOf() == target) {
      class LineMerger *lm = (class LineMerger *)lap;
      lm->GenerateDifferentialImage();
      hexp = lm->isHorizontallyExpanding();
      vexp = lm->isVerticallyExpanding();
      return;
    }
    lap = lap->LowPassOf();
  }
  assert(!"target band not found");
#else
  NOREF(target);
  NOREF(hexp);
  NOREF(vexp);
  JPG_THROW(NOT_IMPLEMENTED,"HierarchicalBitmapRequester::GenerateDifferentialImage",
            "Lossless JPEG not available in your code release, please contact Accusoft for a full version");
#endif
}
///

/// HierarchicalBitmapRequester::DefineRegion
// Define a single 8x8 block starting at the x offset and the given
// line, taking the input 8x8 buffer.
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::DefineRegion(LONG x,const struct Line *const *line,
                                               const LONG *buffer,UBYTE comp)
{
  int cnt = 8;
  
  assert(comp < m_ucCount);
  NOREF(comp);

  x <<= 3;
  
  do {
    if (*line) memcpy((*line)->m_pData + x,buffer,8 * sizeof(LONG));
    buffer += 8;
    line++;
  } while(--cnt);
}
#endif
///

/// HierarchicalBitmapRequester::FetchRegion
// Define a single 8x8 block starting at the x offset and the given
// line, taking the input 8x8 buffer.
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::FetchRegion(LONG x,const struct Line *const *line,LONG *buffer)
{
  int cnt = 8;
  do {
    if (*line) 
      memcpy(buffer,(*line)->m_pData + (x << 3),8 * sizeof(LONG));
    buffer += 8;
    line++;
  } while(--cnt);
}
#endif
///

/// HierarchicalBitmapRequester::Allocate8Lines
// Get the next block of eight lines of the image
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::Allocate8Lines(UBYTE c)
{
  int cnt;
  ULONG y = m_pulY[c];
  //
  // Allocate a line block from the encoding line adapter.
  for(cnt = 0;cnt < 8 && y < m_pulHeight[c];cnt++) {
    assert(m_ppEncodingMCU[cnt | (c << 3)] == NULL);
    m_ppEncodingMCU[cnt | (c << 3)] = m_pLargestScale->AllocateLine(c);
    y++;
  }
}
#endif
///

/// HierarchicalBitmapRequester::Push8Lines
// Advance the image line pointer by the next eight lines
// which is here a "pseudo"-MCU block.
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::Push8Lines(UBYTE c)
{
  int cnt;
  ULONG y = m_pulY[c];
  //
  for(cnt = 0;cnt < 8 && y < m_pulHeight[c];cnt++) {
    assert(m_ppEncodingMCU[cnt | (c << 3)]);
    m_pLargestScale->PushLine(m_ppEncodingMCU[cnt | (c << 3)],c);
    m_ppEncodingMCU[cnt | (c << 3)] = NULL;
    y++;
  }

  m_pulY[c] = y;
}
#endif
///

/// HierarchicalBitmapRequester::Pull8Lines
// Pull 8 lines from the top-level and place them into
// the decoder MCU.
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::Pull8Lines(UBYTE c)
{ 
 int cnt;
  ULONG y = m_pulY[c];
  //
  // Allocate a line block from the encoding line adapter.
  for(cnt = 0;cnt < 8 && y < m_pulHeight[c];cnt++) {
    assert(m_ppDecodingMCU[cnt | (c << 3)] == NULL);
    m_ppDecodingMCU[cnt | (c << 3)] = m_pLargestScale->GetNextLine(c);
    y++;
  }
}
#endif
///

/// HierarchicalBitmapRequester::Release8Lines
// Release the currently buffered decoder MCU for the given component.
#if ACCUSOFT_CODE
void HierarchicalBitmapRequester::Release8Lines(UBYTE c)
{ 
  int cnt;
  ULONG y = m_pulY[c];
  //
  for(cnt = 0;cnt < 8 && y < m_pulHeight[c];cnt++) {
    assert(m_ppDecodingMCU[cnt | (c << 3)]);
    m_pLargestScale->ReleaseLine(m_ppDecodingMCU[cnt | (c << 3)],c);
    m_ppDecodingMCU[cnt | (c << 3)] = NULL;
    y++;
  }
}
#endif
///

/// HierarchicalBitmapRequester::CropEncodingRegion
// First step of a region encoder: Find the region that can be pulled in the next step,
// from a rectangle request. This potentially shrinks the rectangle, which should be
// initialized to the full image.
void HierarchicalBitmapRequester::CropEncodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *)
{ 
#if ACCUSOFT_CODE
  int i;

  ClipToImage(region);

  // Find the region to request.
  for(i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < ULONG(region.ra_MinY))
      region.ra_MinY = m_pulReadyLines[i];
  }
#else
  NOREF(region);
#endif
}
///

/// HierarchicalBitmapRequester::RequestUserDataForEncoding
// Request user data for encoding for the given region, potentially clip the region to the
// data available from the user.
void HierarchicalBitmapRequester::RequestUserDataForEncoding(class BitMapHook *bmh,RectAngle<LONG> &region,bool alpha)
{
#if ACCUSOFT_CODE
  int i;

  m_ulMaxMCU = MAX_ULONG;
  
  for(i = 0;i < m_ucCount;i++) {
    ULONG max;
    //
    // Components are always requested completely on encoding.
    RequestUserData(bmh,region,i,alpha);
    // All components must have the same sample precision here.
    max = (m_ppBitmap[i]->ibm_ulHeight - 1) >> 3;
    if (max < m_ulMaxMCU)
      m_ulMaxMCU = max; 
    if (LONG(m_ppBitmap[i]->ibm_ulHeight) - 1 < region.ra_MaxY)
      region.ra_MaxY = m_ppBitmap[i]->ibm_ulHeight - 1;
  }
#else
  NOREF(bmh);
  NOREF(region);
  NOREF(alpha);
#endif
}
///

/// HierarchicalBitmapRequester::RequestUserDataForDecoding
// Pull data buffers from the user data bitmap hook
void HierarchicalBitmapRequester::RequestUserDataForDecoding(class BitMapHook *bmh,RectAngle<LONG> &region,
                                                             const struct RectangleRequest *rr,bool alpha)
{
#if ACCUSOFT_CODE
  int i;

  ResetBitmaps();
  
  if (m_pLargestScale->FrameOf()->WidthOf()   != m_pFrame->WidthOf() ||
      (m_pLargestScale->FrameOf()->HeightOf() != m_pFrame->HeightOf() &&
       m_pLargestScale->FrameOf()->HeightOf() != 0 && m_pFrame->HeightOf() != 0)) {
    JPG_THROW(MALFORMED_STREAM,"HierarchicalBitmapRequester::ReconstructRegion",
              "hierarchical frame hierarchy is damaged, largest frame does not match the image");
  }
  
  if (m_ulPixelHeight == 0) {
    ULONG height = 0;
    if (m_pLargestScale->FrameOf()->HeightOf() != 0) {
      height = m_pLargestScale->FrameOf()->HeightOf();
    } else if (m_pFrame->HeightOf() != 0) {
      height = m_pFrame->HeightOf();
    }
    if (height) {
      PostImageHeight(height);
    }
  }
  
  m_ulMaxMCU = MAX_ULONG;
  
  for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    RequestUserData(bmh,region,i,alpha);
    ULONG max = (BitmapOf(i).ibm_ulHeight >> 3) - 1;
    if (max < m_ulMaxMCU)
      m_ulMaxMCU = max;
  }
#else
  NOREF(bmh);
  NOREF(region);
  NOREF(rr);
  NOREF(alpha);
#endif
}
///

/// HierarchicalBitmapRequester::EncodeRegion
// Encode a region without downsampling but color transformation
void HierarchicalBitmapRequester::EncodeRegion(const RectAngle<LONG> &region)
{
#if ACCUSOFT_CODE
  class ColorTrafo *ctrafo = ColorTrafoOf(true,false);
  int i;
  // 

  if (m_bSubsampling) { 
    RectAngle<LONG> r;
    ULONG minx   = region.ra_MinX >> 3;
    ULONG maxx   = region.ra_MaxX >> 3;
    ULONG miny   = region.ra_MinY >> 3;
    ULONG maxy   = region.ra_MaxY >> 3;
    ULONG x,y;
    
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

      for(i = 0;i < m_ucCount;i++) {
        if (m_ppDownsampler[i] == NULL) {
          Allocate8Lines(i);
        }
      }
      
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
        ctrafo->RGB2YCbCr(r,m_ppTempIBM,m_ppCTemp);
        
        // Now push the transformed data into either the downsampler, 
        // or the forward DCT block row.
        for(i = 0;i < m_ucCount;i++) {
          if (m_ppDownsampler[i]) {
            // Just collect the data in the downsampler for the time
            // being. Will be taken care of as soon as it is complete.
            m_ppDownsampler[i]->DefineRegion(x,y,m_ppCTemp[i]);
          } else { 
            DefineRegion(x,m_ppEncodingMCU + (i << 3),m_ppCTemp[i],i);
          }
        }
      }
      //
      // Advance the quantized rows for the non-subsampled components,
      // downsampled components will be advanced later.
      for(i = 0;i < m_ucCount;i++) {
        m_pulReadyLines[i]    += 8; // somewhere in the buffer.
        if (m_ppDownsampler[i] == NULL) {
          Push8Lines(i);
        } else {
          LONG bx,by;
          RectAngle<LONG> blocks;
          // Collect the downsampled blocks and push that into the DCT.
          m_ppDownsampler[i]->GetCollectedBlocks(blocks);
          for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
            Allocate8Lines(i);
            for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
              LONG src[64]; // temporary buffer, the DCT requires a 8x8 block
              m_ppDownsampler[i]->DownsampleRegion(bx,by,src);
              DefineRegion(bx,m_ppEncodingMCU + (i << 3),src,i);
            }
            m_ppDownsampler[i]->RemoveBlocks(by);
            Push8Lines(i);
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

    for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
      r.ra_MaxY = (r.ra_MinY & -8) + 7;
      if (r.ra_MaxY > region.ra_MaxY)
        r.ra_MaxY = region.ra_MaxY;

      for(i = 0;i < m_ucCount;i++) {
        Allocate8Lines(i);
      }
      
      for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
        r.ra_MaxX = (r.ra_MinX & -8) + 7;
        if (r.ra_MaxX > region.ra_MaxX)
          r.ra_MaxX = region.ra_MaxX;

        for(i = 0;i < m_ucCount;i++) {      
          ExtractBitmap(m_ppTempIBM[i],r,i);
        }
        
        ctrafo->RGB2YCbCr(r,m_ppTempIBM,m_ppCTemp);

        for(i = 0;i < m_ucCount;i++) {
          DefineRegion(x,m_ppEncodingMCU + (i << 3),m_ppCTemp[i],i);
        }
      }
      for(i = 0;i < m_ucCount;i++) {
        Push8Lines(i);
        m_pulReadyLines[i]   += 8;
      }
    }
  }
#else
  NOREF(region);
#endif
}
///

/// HierarchicalBitmapRequester::ReconstructRegion
// Reconstruct a block, or part of a block
void HierarchicalBitmapRequester::ReconstructRegion(const RectAngle<LONG> &orgregion,const struct RectangleRequest *rr)
{
#if ACCUSOFT_CODE
  class ColorTrafo *ctrafo = ColorTrafoOf(false,!rr->rr_bColorTrafo);
  UBYTE i;

  if (ctrafo == NULL)
    return;
  
  if (m_bSubsampling && rr->rr_bUpsampling) { 
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
        blocks.ra_MinX        = ((orgregion.ra_MinX / subx - rx) >> 3);
        blocks.ra_MaxX        = ((orgregion.ra_MaxX / subx + rx) >> 3);
        blocks.ra_MinY        = ((orgregion.ra_MinY / suby - ry) >> 3);
        blocks.ra_MaxY        = ((orgregion.ra_MaxY / suby + ry) >> 3);
        // Clip.
        if (blocks.ra_MinX < 0)        blocks.ra_MinX = 0;
        if (blocks.ra_MaxX >= bwidth)  blocks.ra_MaxX = bwidth - 1;
        if (blocks.ra_MinY < 0)        blocks.ra_MinY = 0;
        if (blocks.ra_MaxY >= bheight) blocks.ra_MaxY = bheight - 1;
        up->SetBufferedRegion(blocks); // also removes the rectangle of blocks already buffered.
        //
        for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
          Pull8Lines(i);
          for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
            LONG dst[64];
            FetchRegion(bx,m_ppDecodingMCU + (i << 3),dst);
            up->DefineRegion(bx,by,dst);
          }
          Release8Lines(i);
        }
      } else {
        // Load into the decoding MCU
        Pull8Lines(i);
      }
    }
    // Now push blocks into the color transformer from the upsampler.
    {
      RectAngle<LONG> r;
      ULONG minx   = orgregion.ra_MinX >> 3;
      ULONG maxx   = orgregion.ra_MaxX >> 3;
      ULONG miny   = orgregion.ra_MinY >> 3;
      ULONG maxy   = orgregion.ra_MaxY >> 3;
      ULONG x,y;
      
      if (maxy > m_ulMaxMCU)
        maxy = m_ulMaxMCU;

      for(y = miny,r.ra_MinY = orgregion.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
        r.ra_MaxY = (r.ra_MinY & -8) + 7;
        if (r.ra_MaxY > orgregion.ra_MaxY)
          r.ra_MaxY = orgregion.ra_MaxY;
        
        for(x = minx,r.ra_MinX = orgregion.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
          r.ra_MaxX = (r.ra_MinX & -8) + 7;
          if (r.ra_MaxX > orgregion.ra_MaxX)
            r.ra_MaxX = orgregion.ra_MaxX;
          
          for(i = 0;i < m_ucCount;i++) {
            // Component extraction must go here as the requested components
            // refer to components in YUV space, not in RGB space.
            ExtractBitmap(m_ppTempIBM[i],r,i);
            if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
              if (m_ppUpsampler[i]) {
                // Upsampled case, take from the upsampler, transform
                // into the color buffer.
                m_ppUpsampler[i]->UpsampleRegion(r,m_ppCTemp[i]);
              } else {
                FetchRegion(x,m_ppDecodingMCU + (i << 3),m_ppCTemp[i]);
              }
            } else {
              // Not requested, zero the buffer.
              memset(m_ppCTemp[i],0,sizeof(LONG) * 64);
            }
          }
          ctrafo->YCbCr2RGB(r,m_ppTempIBM,m_ppCTemp,NULL);
        }
        //
        // Advance the quantized rows for the non-subsampled components,
        // upsampled components have been advanced above.
        for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
          if (m_ppUpsampler[i] == NULL)
            Release8Lines(i);
        }
      }
    }
  } else { 
    // direct case, no upsampling required, residual coding possible, but not applied here.
    RectAngle<LONG> r;
    RectAngle<LONG> region = orgregion;
    SubsampledRegion(region,rr);
    ULONG minx   = region.ra_MinX >> 3;
    ULONG maxx   = region.ra_MaxX >> 3;
    ULONG miny   = region.ra_MinY >> 3;
    ULONG maxy   = region.ra_MaxY >> 3;
    ULONG x,y;
      
    if (maxy > m_ulMaxMCU)
      maxy = m_ulMaxMCU;

    for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
      Pull8Lines(i);
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
          LONG *dst = m_ppCTemp[i];
          ExtractBitmap(m_ppTempIBM[i],r,i);
          if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
            FetchRegion(x,m_ppDecodingMCU + (i << 3),dst);
          } else {
            memset(dst,0,sizeof(LONG) * 64);
          }
        }
        //
        // Perform the color transformation now.
        ctrafo->YCbCr2RGB(r,m_ppTempIBM,m_ppCTemp,NULL);
      } // of loop over x
      //
      // Advance the rows.
      for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
        Release8Lines(i);
      }
    }
  }
#else
  NOREF(orgregion);
  NOREF(rr);
#endif
}
///

/// HierarchicalBitmapRequester::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder.
bool HierarchicalBitmapRequester::isNextMCULineReady(void) const
{
#if ACCUSOFT_CODE
  // MCUs can only be written if the smallest scale, which is written first,
  // is ready.
  return m_pSmallestScale->isNextMCULineReady();
#else
  return false;
#endif
}
///

/// HierarchicalBitmapRequester::ResetToStartOfImage
// Reset all components on the image side of the control to the
// start of the image. Required when re-requesting the image
// for encoding or decoding.
void HierarchicalBitmapRequester::ResetToStartOfImage(void)
{
#if ACCUSOFT_CODE
  for(UBYTE i = 0;i < m_ucCount;i++) {
    m_pulY[i] = 0;
    m_pulReadyLines[i] = 0;
  }
  //
  assert(m_pLargestScale);
  // Now iterate through the tree.
  m_pLargestScale->ResetToStartOfImage();
#endif
}
///

/// HierarchicalBitmapRequester::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool HierarchicalBitmapRequester::isImageComplete(void) const
{ 
#if ACCUSOFT_CODE
  for(UBYTE i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_ulPixelHeight)
      return false;
  }
  return true;
#else
  return false;
#endif
}
///

/// HierarchicalBitmapRequester::BufferedLines
// Return the number of lines available for reconstruction from this scan.
ULONG HierarchicalBitmapRequester::BufferedLines(const struct RectangleRequest *rr) const
{
#if ACCUSOFT_CODE
  ULONG maxlines = m_ulPixelHeight;
  
  for(UBYTE i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    class Component *comp = m_pFrame->ComponentOf(i);
    UBYTE suby            = comp->SubYOf();
    ULONG lines;
    // Since the user here asks for complete(!) lines and the highpass comes last
    // in the codestream, ask the highpass about how many lines are buffered.
    // These lines are counted in subsampled lines.
    lines = m_pLargestScale->BufferedLines(i);
    if (lines >= m_pulHeight[i]) {
      lines = m_ulPixelHeight;
    } else if (suby > 1 && lines > 0) {
      lines = ((lines - 1) * suby) & (-8); // one additional subsampled line, actually,;
    } else {
      lines = (lines * suby) & (-8); 
    }
    if (lines < maxlines)
      maxlines = lines;
  }

  return maxlines;
#else
  NOREF(rr);
  return 0;
#endif
}
///

/// HierarchicalBitmapRequester::PostImageHeight
// Post the height of the frame in lines. This happens
// when the DNL marker is processed.
void HierarchicalBitmapRequester::PostImageHeight(ULONG lines)
{
  BitmapCtrl::PostImageHeight(lines);
#if ACCUSOFT_CODE
  assert(m_pulHeight);

  if (m_pLargestScale)
    m_pLargestScale->PostImageHeight(lines);
  
  for(UBYTE i = 0;i < m_ucCount;i++) {
    class Component *comp = m_pFrame->ComponentOf(i);
    UBYTE suby            = comp->SubYOf();
    m_pulHeight[i]        = (m_ulPixelHeight + suby - 1) / suby;
  }
#endif
}
///
