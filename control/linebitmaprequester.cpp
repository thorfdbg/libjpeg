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
** This class pulls blocks from the frame and reconstructs from those
** quantized block lines or encodes from them.
**
** $Id: linebitmaprequester.cpp,v 1.40 2023/02/21 09:17:33 thor Exp $
**
*/

/// Includes
#include "control/bitmapctrl.hpp"
#include "control/linebitmaprequester.hpp"
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
#include "tools/line.hpp"
#include "dct/dct.hpp"
#include "colortrafo/colortrafo.hpp"
#include "std/string.hpp"
///

/// LineBitmapRequester::LineBitmapRequester
LineBitmapRequester::LineBitmapRequester(class Frame *frame)
  : LineBuffer(frame), BitmapCtrl(frame), m_pEnviron(frame->EnvironOf()), m_pFrame(frame),
    m_pulReadyLines(NULL),
    m_ppDownsampler(NULL), m_ppUpsampler(NULL), m_ppTempIBM(NULL), 
    m_pppImage(NULL), m_bSubsampling(false)
{ 
  m_ucCount       = frame->DepthOf();
  m_ulPixelWidth  = frame->WidthOf();
  m_ulPixelHeight = frame->HeightOf();
}
///

/// LineBitmapRequester::~LineBitmapRequester
LineBitmapRequester::~LineBitmapRequester(void)
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

  if (m_pppImage)
    m_pEnviron->FreeMem(m_pppImage,m_ucCount * sizeof(struct Line **));
}
///

/// LineBitmapRequester::BuildCommon
// Build common structures for encoding and decoding
void LineBitmapRequester::BuildCommon(void)
{
  UBYTE i;

  BitmapCtrl::BuildCommon();
  LineBuffer::BuildCommon();

  if (m_ppTempIBM == NULL) {
    m_ppTempIBM = (struct ImageBitMap **)m_pEnviron->AllocMem(sizeof(struct ImageBitMap **) * m_ucCount);
    memset(m_ppTempIBM,0,sizeof(struct ImageBitMap *) * m_ucCount);
  }

  if (m_pulReadyLines == NULL) {
    m_pulReadyLines = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * m_ucCount);
    memset(m_pulReadyLines,0,sizeof(ULONG) * m_ucCount);
  }

  if (m_pppImage == NULL) {
    m_pppImage    = (struct Line ***)m_pEnviron->AllocMem(sizeof(struct Line **) * m_ucCount);
    for(i = 0;i < m_ucCount;i++) {
       m_pppImage[i]         = m_ppTop + i;
    }
  }

  for(i = 0;i < m_ucCount;i++) {
    if (m_ppTempIBM[i] == NULL)
      m_ppTempIBM[i]      = new(m_pEnviron) struct ImageBitMap();
  }
}
///

/// LineBitmapRequester::PrepareForEncoding
// First time usage: Collect all the information
void LineBitmapRequester::PrepareForEncoding(void)
{  
  UBYTE i;
  
  BuildCommon();

  if (m_ppDownsampler == NULL) {
    m_ppDownsampler = (class DownsamplerBase **)m_pEnviron->AllocMem(sizeof(class DownsamplerBase *) * m_ucCount);
    memset(m_ppDownsampler,0,sizeof(class DownsamplerBase *) * m_ucCount);
    
    for(i = 0;i < m_ucCount;i++) {
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
}
///

/// LineBitmapRequester::PrepareForDecoding
// First time usage: Collect all the information
void LineBitmapRequester::PrepareForDecoding(void)
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
                                                          m_ulPixelWidth,m_ulPixelHeight,
                                                          m_pFrame->TablesOf()->isChromaCentered());
        m_bSubsampling   = true;
      }
    }
  }
}
///

/// LineBitmapRequester::ResetToStartOfImage
// Reset all components on the image side of the control to the
// start of the image. Required when re-requesting the image
// for encoding or decoding.
void LineBitmapRequester::ResetToStartOfImage(void)
{
  for(UBYTE i = 0;i < m_ucCount;i++) {
    m_pppImage[i]      = &m_ppTop[i];
    m_pulReadyLines[i] = 0;
  }
}
///

/// LineBitmapRequester::ColorTrafoOf
// Return the color transformer responsible for this scan.
class ColorTrafo *LineBitmapRequester::ColorTrafoOf(bool encoding,bool disabletorgb)
{
  return m_pFrame->TablesOf()->ColorTrafoOf(m_pFrame,NULL,PixelTypeOf(),
                                            encoding,disabletorgb);
}
///

/// LineBitmapRequester::Start8Lines
// Get the next block of eight lines of the image
struct Line *LineBitmapRequester::Start8Lines(UBYTE c)
{
  if (*m_pppImage[c] == NULL) {
    struct Line **target = m_pppImage[c];
    int cnt = 8;
    do {
      *target = new(m_pEnviron) struct Line;
      (*target)->m_pData = (LONG *)m_pEnviron->AllocMem(m_pulWidth[c] * sizeof(LONG));
      target  = &((*target)->m_pNext);
    } while(--cnt);
  }
  return *m_pppImage[c];
}
///

/// LineBitmapRequester::Next8Lines
// Advance the image line pointer by the next eight lines
// which is here a "pseudo"-MCU block.
void LineBitmapRequester::Next8Lines(UBYTE c)
{
  int cnt = 8;
  do {
    struct Line *row = *m_pppImage[c];
    if (!row)
      break;
    m_pppImage[c] = &(row->m_pNext);
  } while(--cnt && *m_pppImage[c]);
}
///

/// LineBitmapRequester::CropEncodingRegion
// First step of a region encoder: Find the region that can be pulled in the next step,
// from a rectangle request. This potentially shrinks the rectangle, which should be
// initialized to the full image.
void LineBitmapRequester::CropEncodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *)
{
  int i;  

  ClipToImage(region);

  // Find the region to request.
  for(i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < ULONG(region.ra_MinY))
      region.ra_MinY = m_pulReadyLines[i];
  } 
}
///

/// LineBitmapRequester::RequestUserDataForEncoding
// Request user data for encoding for the given region, potentially clip the region to the
// data available from the user.
void LineBitmapRequester::RequestUserDataForEncoding(class BitMapHook *bmh,RectAngle<LONG> &region,bool alpha)
{ 
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
}
///

/// LineBitmapRequester::EncodeRegion
// Encode a region without downsampling but color transformation
void LineBitmapRequester::EncodeRegion(const RectAngle<LONG> &region)
{
  class ColorTrafo *ctrafo = ColorTrafoOf(true,false);
  int i;
  
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
            DefineRegion(x,Start8Lines(i),m_ppCTemp[i],i);
          }
        }
      }
      //
      // Advance the quantized rows for the non-subsampled components,
      // downsampled components will be advanced later.
      for(i = 0;i < m_ucCount;i++) {
        m_pulReadyLines[i]    += 8; // somewhere in the buffer.
        if (m_ppDownsampler[i] == NULL) {
          Next8Lines(i);
        } else {
          LONG bx,by;
          RectAngle<LONG> blocks;
          // Collect the downsampled blocks and push that into the DCT.
          m_ppDownsampler[i]->GetCollectedBlocks(blocks);
          for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
            struct Line *row = Start8Lines(i);
            for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
              LONG src[64]; // temporary buffer, the DCT requires a 8x8 block
              m_ppDownsampler[i]->DownsampleRegion(bx,by,src);
              DefineRegion(bx,row,src,i);
            }
            m_ppDownsampler[i]->RemoveBlocks(by);
            Next8Lines(i);
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
        
      for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
        r.ra_MaxX = (r.ra_MinX & -8) + 7;
        if (r.ra_MaxX > region.ra_MaxX)
          r.ra_MaxX = region.ra_MaxX;

        for(i = 0;i < m_ucCount;i++) {      
          ExtractBitmap(m_ppTempIBM[i],r,i);
        }
        
        ctrafo->RGB2YCbCr(r,m_ppTempIBM,m_ppCTemp);

        for(i = 0;i < m_ucCount;i++) {
          DefineRegion(x,Start8Lines(i),m_ppCTemp[i],i);
        }
      }
      for(i = 0;i < m_ucCount;i++) {
        Next8Lines(i);
        m_pulReadyLines[i]   += 8;
      }
    }
  }
}
///
 
/// LineBitmapRequester::RequestUserDataForDecoding
// Pull data buffers from the user data bitmap hook
void LineBitmapRequester::RequestUserDataForDecoding(class BitMapHook *bmh,RectAngle<LONG> &region,
                                                     const struct RectangleRequest *rr,bool alpha)
{ 
  int i;

  ResetBitmaps();
  
  m_ulMaxMCU = MAX_ULONG;
  
  for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    RequestUserData(bmh,region,i,alpha);
    ULONG max = (BitmapOf(i).ibm_ulHeight >> 3) - 1;
    if (max < m_ulMaxMCU)
      m_ulMaxMCU = max;
  }
}
///

/// LineBitmapRequester::ReconstructRegion
// Reconstruct a block, or part of a block
void LineBitmapRequester::ReconstructRegion(const RectAngle<LONG> &orgregion,const struct RectangleRequest *rr)
{
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
          for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
            LONG dst[64];
            if (*m_pppImage[i]) {
              FetchRegion(bx,*m_pppImage[i],dst);
            } else {
              memset(dst,0,sizeof(dst));
            }
            up->DefineRegion(bx,by,dst);
          }
          Next8Lines(i);
        }
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
            // ExtractBitMap must go here, noting that the requested components
            // correspond to transformed components in YUV space, not to components
            // in RGB space.
            ExtractBitmap(m_ppTempIBM[i],r,i);
            if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
              if (m_ppUpsampler[i]) {
                // Upsampled case, take from the upsampler, transform
                // into the color buffer.
                m_ppUpsampler[i]->UpsampleRegion(r,m_ppCTemp[i]);
              } else if (*m_pppImage[i]) {
                FetchRegion(x,*m_pppImage[i],m_ppCTemp[i]);
              } else {
                memset(m_ppCTemp[0],0,sizeof(LONG) * 64);
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
            Next8Lines(i);
        }
      }
    }
  } else { // direct case, no upsampling required, residual coding possible.
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
            if (*m_pppImage[i]) {
              FetchRegion(x,*m_pppImage[i],dst);
            } else {
              memset(dst,0,sizeof(LONG) * 64);
            }
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
        Next8Lines(i);
      }
    }
  }
}
///

/// LineBitmapRequester::isNextMCULineReady
// Return true if the next MCU line is buffered and can be pushed
// to the encoder.
bool LineBitmapRequester::isNextMCULineReady(void) const
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

/// LineBitmapRequester::isImageComplete
// Return an indicator whether all of the image has been loaded into
// the image buffer.
bool LineBitmapRequester::isImageComplete(void) const
{
  for(UBYTE i = 0;i < m_ucCount;i++) {
    if (m_pulReadyLines[i] < m_ulPixelHeight)
      return false;
  }
  return true;
}
///
