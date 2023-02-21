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
** $Id: blockbitmaprequester.cpp,v 1.76 2023/02/21 09:17:33 thor Exp $
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
#include "marker/scantypes.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/huffmancoder.hpp"
#include "dct/dct.hpp"
#include "dct/deringing.hpp"
#include "colortrafo/colortrafo.hpp"
#include "std/string.hpp"
///

/// BlockBitmapRequester::BlockBitmapRequester
BlockBitmapRequester::BlockBitmapRequester(class Frame *frame)
  : BlockBuffer(frame), BitmapCtrl(frame), m_pEnviron(frame->EnvironOf()), m_pFrame(frame),
    m_pulReadyLines(NULL),
    m_ppDownsampler(NULL), m_ppResidualDownsampler(NULL),
    m_ppUpsampler(NULL), m_ppResidualUpsampler(NULL), m_ppOriginalImage(NULL),
    m_ppTempIBM(NULL), m_ppOriginalIBM(NULL),
    m_ppQTemp(NULL), m_ppRTemp(NULL), m_ppDTemp(NULL),
    m_plResidualColorBuffer(NULL), m_plOriginalColorBuffer(NULL), 
    m_pppQImage(NULL), m_pppRImage(NULL),
    m_pResidualHelper(NULL), m_ppDeRinger(NULL), 
    m_bSubsampling(false), m_bOpenLoop(false), m_bDeRing(false)
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

  if (m_ppDTemp)
    m_pEnviron->FreeMem(m_ppDTemp,m_ucCount * sizeof(LONG *));
  
  if (m_plResidualColorBuffer)
    m_pEnviron->FreeMem(m_plResidualColorBuffer,m_ucCount * 64 * sizeof(LONG));

  if (m_plOriginalColorBuffer)
    m_pEnviron->FreeMem(m_plOriginalColorBuffer,m_ucCount * 64 * sizeof(LONG));

  if (m_ppDownsampler) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppDownsampler[i];
    }
    m_pEnviron->FreeMem(m_ppDownsampler,m_ucCount * sizeof(class DownsamplerBase *));
  }

  if (m_ppResidualDownsampler) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppResidualDownsampler[i];
    }
    m_pEnviron->FreeMem(m_ppResidualDownsampler,m_ucCount * sizeof(class DownsamplerBase *));
  }

  if (m_ppUpsampler) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppUpsampler[i];
    }
    m_pEnviron->FreeMem(m_ppUpsampler,m_ucCount * sizeof(class UpsamplerBase *));
  }

  if (m_ppResidualUpsampler) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppResidualUpsampler[i];
    }
    m_pEnviron->FreeMem(m_ppResidualUpsampler,m_ucCount * sizeof(class UpsamplerBase *));
  }

  if (m_ppOriginalImage) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppOriginalImage[i];
    }
    m_pEnviron->FreeMem(m_ppOriginalImage,m_ucCount * sizeof(class DownsamplerBase *));
  } 

  if (m_ppTempIBM) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppTempIBM[i];
    }
    m_pEnviron->FreeMem(m_ppTempIBM,m_ucCount * sizeof(struct ImageBitMap *));
  }
  
  if (m_ppOriginalIBM) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppOriginalIBM[i];
    }
    m_pEnviron->FreeMem(m_ppOriginalIBM,m_ucCount * sizeof(struct ImageBitMap *));
  }

  if (m_ppDeRinger) {
    for(i = 0;i < m_ucCount;i++) {
      delete m_ppDeRinger[i];
    }
    m_pEnviron->FreeMem(m_ppDeRinger,m_ucCount * sizeof(class DeRinger *));
  }
  
  if (m_pulReadyLines)
    m_pEnviron->FreeMem(m_pulReadyLines,m_ucCount * sizeof(ULONG));

  if (m_pppQImage)
    m_pEnviron->FreeMem(m_pppQImage,m_ucCount * sizeof(class QuantizedRow **));

  if (m_pppRImage)
    m_pEnviron->FreeMem(m_pppRImage,m_ucCount * sizeof(class QuantizedRow **));

  if (m_ppQTemp)
    m_pEnviron->FreeMem(m_ppQTemp,m_ucCount * sizeof(LONG *));

  if (m_ppRTemp)
    m_pEnviron->FreeMem(m_ppRTemp,m_ucCount * sizeof(LONG *));

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

  if (m_ppDeRinger == NULL) {
    m_ppDeRinger = (class DeRinger **)m_pEnviron->AllocMem(sizeof(class DeRinger *) * m_ucCount);
    memset(m_ppDeRinger,0,sizeof(class DeRinger *) * m_ucCount);
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
  
  if (m_pppQImage == NULL) {
    m_pppQImage   = (class QuantizedRow ***)m_pEnviron->AllocMem(sizeof(class QuantizedRow **) * 
                                                                 m_ucCount);
    for(i = 0;i < m_ucCount;i++) {
      m_pppQImage[i]        = m_ppQTop + i;
    }
  }
  
  if (m_pppRImage == NULL) {
    m_pppRImage   = (class QuantizedRow ***)m_pEnviron->AllocMem(sizeof(class QuantizedRow **) * 
                                                                 m_ucCount);
    for(i = 0;i < m_ucCount;i++) {
      m_pppRImage[i]        = m_ppRTop + i;
    }
  }

  if (m_ppQTemp == NULL)
    m_ppQTemp     = (LONG **)m_pEnviron->AllocMem(sizeof(LONG *) * m_ucCount);

  if (m_ppRTemp == NULL)
    m_ppRTemp     = (LONG **)m_pEnviron->AllocMem(sizeof(LONG *) * m_ucCount);

  for(i = 0;i < m_ucCount;i++) {
    if (m_ppTempIBM[i] == NULL)
      m_ppTempIBM[i]      = new(m_pEnviron) struct ImageBitMap();
  }
}
///

/// BlockBitmapRequester::PrepareForEncoding
// First time usage: Collect all the information
void BlockBitmapRequester::PrepareForEncoding(void)
{  
  class Tables *tables = m_pFrame->TablesOf();
  UBYTE i;
  
  BuildCommon();
  
  // Build the DCT transformers.
  ResetToStartOfScan(NULL);

  //
  // This flag is only used on encoding.
  m_bOpenLoop = tables->isOpenLoop();
  m_bOptimize = tables->Optimization();
  m_bDeRing   = tables->isDeringingEnabled();
  
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
                                                                tables->isDownsamplingInterpolated());
        m_bSubsampling     = true;
      }
    }
  }

  if (m_bDeRing) {
    assert(m_ppDCT && m_ppDeRinger);
    for(i = 0;i < m_ucCount;i++) {
      if (m_ppDCT[i] && m_ppDeRinger[i] == NULL) {
        m_ppDeRinger[i] = new(m_pEnviron) class DeRinger(m_pFrame,m_ppDCT[i]);
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
                                                          m_ulPixelWidth,m_ulPixelHeight,
                                                          m_pFrame->TablesOf()->isChromaCentered());
        m_bSubsampling   = true;
      }
    }
  }
}
///

/// BlockBitmapRequester::SetBlockHelper
// Install a block helper for residual coding.
void BlockBitmapRequester::SetBlockHelper(class ResidualBlockHelper *helper)
{
  m_pResidualHelper = helper;

  if (helper) {
    class Frame *residualframe = m_pResidualHelper->ResidualFrameOf();
    UBYTE i;
    
    if (m_ppDownsampler && m_ppResidualDownsampler == NULL) {
      m_ppResidualDownsampler = (class DownsamplerBase **)m_pEnviron->AllocMem(sizeof(class DownsamplerBase *) * 
                                                                               m_ucCount);
      memset(m_ppResidualDownsampler,0,sizeof(class DownsamplerBase *) * m_ucCount);
      
      for(i = 0;i < m_ucCount;i++) {
        class Component *comp = residualframe->ComponentOf(i);
        UBYTE sx = comp->SubXOf();
        UBYTE sy = comp->SubYOf();
        
        if (sx > 1 || sy > 1) {
          // Residual coding does not work with interpolation.
          m_ppResidualDownsampler[i] = DownsamplerBase::CreateDownsampler(m_pEnviron,sx,sy,
                                                                          m_ulPixelWidth,m_ulPixelHeight,
                                                                          residualframe->TablesOf()->
                                                                          isDownsamplingInterpolated());
          m_bSubsampling     = true;
        }
      }
    }
    
    //
    // The encoder also requires the upsampler.
    if ((m_ppUpsampler || m_ppDownsampler) && m_ppResidualUpsampler == NULL) {
      m_ppResidualUpsampler = (class UpsamplerBase **)m_pEnviron->AllocMem(sizeof(class UpsamplerBase *) * 
                                                                           m_ucCount);
      memset(m_ppResidualUpsampler,0,sizeof(class Upsampler *) * m_ucCount);

      for(i = 0;i < m_ucCount;i++) {
        class Component *comp = residualframe->ComponentOf(i);
        UBYTE sx = comp->SubXOf();
        UBYTE sy = comp->SubYOf();

        if (sx > 1 || sy > 1) {
          m_ppResidualUpsampler[i] = UpsamplerBase::CreateUpsampler(m_pEnviron,sx,sy,
                                                                    m_ulPixelWidth,m_ulPixelHeight,
                                                                    m_pFrame->TablesOf()->isChromaCentered());
          
          m_bSubsampling     = true;
        }
      }
    }
    //
    // Build the residual color buffer which buffers the output of the 
    // upsampler.
    if (m_ppDTemp == NULL)
      m_ppDTemp     = (LONG **)m_pEnviron->AllocMem(m_ucCount * sizeof(LONG *));
    
    if (m_plResidualColorBuffer == NULL)
      m_plResidualColorBuffer = (LONG *)m_pEnviron->AllocMem(m_ucCount * 64 * sizeof(LONG));
    
    for(i = 0;i < m_ucCount;i++) {
      m_ppDTemp[i]  = m_plResidualColorBuffer + i * 64;
    }

    //
    // If we are encoding and require any type of downsampler and residual coding
    // If there is a downsampler, we also need an upsampler for residual coding
    // and downsampling on all components or things get too complicated. The
    // downsampler acts as an image buffer.
    if (m_ppDownsampler) {
      if (m_ppUpsampler == NULL) {
        m_ppUpsampler = (class UpsamplerBase **)m_pEnviron->AllocMem(sizeof(class UpsamplerBase *) * 
                                                                     m_ucCount);
        memset(m_ppUpsampler,0,sizeof(class Upsampler *) * m_ucCount);
      }
      if (m_ppOriginalImage == NULL) {
        m_ppOriginalImage = (class DownsamplerBase **)m_pEnviron->AllocMem(sizeof(class DownsamplerBase *) *
                                                                           m_ucCount);
        memset(m_ppOriginalImage,0,sizeof(class DownsamplerBase *) * m_ucCount);
      }

      if (m_plOriginalColorBuffer == NULL)
        m_plOriginalColorBuffer = (LONG *)m_pEnviron->AllocMem(m_ucCount * 64 * sizeof(LONG));

      if (m_ppOriginalIBM == NULL) {
        m_ppOriginalIBM = (struct ImageBitMap **)m_pEnviron->AllocMem(m_ucCount * sizeof(struct ImageBitMap *));
        memset(m_ppOriginalIBM,0,m_ucCount * sizeof(struct ImageBitMap *));
      }

      for(i = 0;i < m_ucCount;i++) {
        class Component *comp = m_pFrame->ComponentOf(i);
        UBYTE sx = comp->SubXOf();
        UBYTE sy = comp->SubYOf();
        

        if (m_ppOriginalIBM[i] == NULL) {
          m_ppOriginalIBM[i] = new(m_pEnviron) struct ImageBitMap();
          // Make it use the original color buffer.
          m_ppOriginalIBM[i]->ibm_ulWidth  = 8;
          m_ppOriginalIBM[i]->ibm_ulHeight = 8;
          m_ppOriginalIBM[i]->ibm_cBytesPerPixel = sizeof(LONG);
          m_ppOriginalIBM[i]->ibm_lBytesPerRow   = 8 * sizeof(LONG);
          m_ppOriginalIBM[i]->ibm_pData          = m_plOriginalColorBuffer + i * 64;
        }

        if (m_ppUpsampler[i] == NULL) {
          bool centered = m_pFrame->TablesOf()->isChromaCentered();
          // For closed loop coding, the upsampler has to upsample the reconstructed data,
          // hence, real upsampling is needed. Otherwise, it just stores the original
          // LDR image.
          if (m_bOpenLoop) {
            m_ppUpsampler[i]   = UpsamplerBase::CreateUpsampler(m_pEnviron,1,1,
                                                                m_ulPixelWidth,m_ulPixelHeight,
                                                                false);
          } else {
            m_ppUpsampler[i]   = UpsamplerBase::CreateUpsampler(m_pEnviron,sx,sy,
                                                                m_ulPixelWidth,m_ulPixelHeight,
                                                                centered);
          }
        }
        
        if (m_ppDownsampler[i] == NULL)
          m_ppDownsampler[i] = DownsamplerBase::CreateDownsampler(m_pEnviron,sx,sy,
                                                                  m_ulPixelWidth,m_ulPixelHeight,
                                                                  false);
        
        // We need to buffer the original image until the encoded image becomes available as reference.
        // This is done here.
        if (m_ppOriginalImage[i] == NULL) {
          m_ppOriginalImage[i] = DownsamplerBase::CreateDownsampler(m_pEnviron,1,1,
                                                                    m_ulPixelWidth,m_ulPixelHeight,
                                                                    false);
        }
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
class ColorTrafo *BlockBitmapRequester::ColorTrafoOf(bool encoding,bool disabletorgb)
{
  return m_pFrame->TablesOf()->ColorTrafoOf(m_pFrame,(m_pResidualHelper)?(m_pResidualHelper->ResidualFrameOf()):(NULL),
                                            PixelTypeOf(),encoding,disabletorgb);
}
///

/// BlockBitmapRequester::BuildImageRow
// Create the next row of the image such that m_pppImage[i] is valid.
class QuantizedRow *BlockBitmapRequester::BuildImageRow(class QuantizedRow **qrow,class Frame *frame,int i)
{
  if (*qrow == NULL) {
    class Component *comp = frame->ComponentOf(i);
    UBYTE subx      = comp->SubXOf();
    ULONG width     = (m_ulPixelWidth  + subx - 1) / subx;
    *qrow = new(m_pEnviron) class QuantizedRow(m_pEnviron);
    (*qrow)->AllocateRow(width);
  }
  return *qrow;
}
///

/// BlockBitmapRequester::AdvanceQRows
// Forward the state machine for the quantized rows by one image-8-block line
void BlockBitmapRequester::AdvanceQRows(void)
{
  ULONG maxval   = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  UBYTE i;
  // Advance the quantized rows for the non-subsampled components,
  // downsampled components will be advanced later.
  for(i = 0;i < m_ucCount;i++) {
    m_pulReadyLines[i]    += 8; // somehwere in the buffer.
    if (m_ppDownsampler[i] == NULL) { 
      // Residual coding should always have downsamplers that act as
      // image buffers.
      assert(m_pResidualHelper == NULL);
      //
      {
        class QuantizedRow *qrow;
        qrow = BuildImageRow(m_pppQImage[i],m_pFrame,i);
        m_pppQImage[i] = &(qrow->NextOf());
      }
    } else {
      LONG bx,by;
      RectAngle<LONG> blocks;
      //
      // Collect the downsampled blocks and push that into the DCT.
      m_ppDownsampler[i]->GetCollectedBlocks(blocks);
      //
      // Extend the buffered region.
      // For openloop coding, the upsampler includes the original LDR
      // data and need not to be filled here by the reconstructed LDR
      // data.
      if (m_pResidualHelper && m_bOpenLoop == false) {
        assert(m_ppUpsampler[i]);
        // Only make larger, do not throw old stuff away.
        if (m_ppUpsampler[i]) {
          m_ppUpsampler[i]->ExtendBufferedRegion(blocks);
        }
      }
      //
      // Push the blocks into the DCT.
      for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
        class QuantizedRow *qr  = BuildImageRow(m_pppQImage[i],m_pFrame,i);
        for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
          LONG src[64]; // temporary buffer, the DCT requires a 8x8 block
          LONG *dst = (qr)?(qr->BlockAt(bx)->m_Data):NULL;
          m_ppDownsampler[i]->DownsampleRegion(bx,by,src);
          if (m_bDeRing) {
            m_ppDeRinger[i]->DeRing(src,dst,(maxval + 1) >> 1);
          } else {
            m_ppDCT[i]->TransformBlock(src,dst,(maxval + 1) >> 1);
          }
          if (m_bOptimize) {
            m_pFrame->OptimizeDCTBlock(bx,by,i,m_ppDCT[i],dst);
          }
          //
          // Inversely reconstruct and feed into the upsampler to get the residual signal.
          // For openloop coding, the upsampler already contains the original LDR
          // data.
          if (m_pResidualHelper && m_bOpenLoop == false) {
            m_ppDCT[i]->InverseTransformBlock(src,dst,(maxval + 1) >> 1);
            assert(m_ppUpsampler[i]);
            m_ppUpsampler[i]->DefineRegion(bx,by,src);
          }
        }
        m_ppDownsampler[i]->RemoveBlocks(by);
        m_pppQImage[i] = &(qr->NextOf());
      }
    }
  }
}
///

/// BlockBitmapRequester::AdvanceRRows
// Compute the residual data and move that into the R-output buffers.
void BlockBitmapRequester::AdvanceRRows(const RectAngle<LONG> &region,class ColorTrafo *ctrafo)
{
  RectAngle<LONG> r,buffered;
  LONG minx,miny;
  LONG maxx,maxy;
  LONG x,y;
  UBYTE i;
  
  // At this point, the reconstructed image is in the m_ppUpsampler buffer. Let's see
  // how much we have. Note that we must feed the components jointly into the
  // color transformation, thus the overlap of all buffered regions is relevant here.
  minx = miny = 0;
  maxx = maxy = MAX_LONG;
  for(i = 0;i < m_ucCount;i++) {
    assert(m_ppUpsampler[i]);
    // On closed loop coding, the upsampler contains now the reconstructed,
    // upsampled data.
    // For openloop coding, it is just a copy of the original LDR data.
    m_ppUpsampler[i]->GetCollectedBlocks(buffered);
    if (buffered.ra_MinX > minx) minx = buffered.ra_MinX;
    if (buffered.ra_MinY > miny) miny = buffered.ra_MinY;
    if (buffered.ra_MaxX < maxx) maxx = buffered.ra_MaxX;
    if (buffered.ra_MaxY < maxy) maxy = buffered.ra_MaxY;
  }
  //
  // Define the regions for the residual downsampler
  buffered.ra_MinX = minx << 3;
  buffered.ra_MinY = miny << 3;
  buffered.ra_MaxX = (maxx << 3) + 7;
  if (buffered.ra_MaxX >= LONG(m_ulPixelWidth))
    buffered.ra_MaxX = m_ulPixelWidth  - 1;
  buffered.ra_MaxY = (maxy << 3) + 7;
  if (buffered.ra_MaxY >= LONG(m_ulPixelHeight))
    buffered.ra_MaxY = m_ulPixelHeight - 1;
  for(i = 0;i < m_ucCount;i++) {
    if (m_ppResidualDownsampler[i])
      m_ppResidualDownsampler[i]->SetBufferedRegion(buffered);
  }
  //
  // Ok, the rectangle of available samples is now known. 
  // Let's upsample it and compute from it the residual.
  for(y = miny,r.ra_MinY = miny << 3;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
    r.ra_MaxY = (r.ra_MinY & -8) + 7;
    if (r.ra_MaxY >= LONG(m_ulPixelHeight))
      r.ra_MaxY = m_ulPixelHeight - 1;
    
    for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
      r.ra_MaxX = (r.ra_MinX & -8) + 7;
      if (r.ra_MaxX >= LONG(m_ulPixelWidth))
        r.ra_MaxX = m_ulPixelWidth - 1;
      // Place the reconstructed upsampled data into the D-buffer. 
      // Since the C-buffer is no longer required, use this for downsampling
      // the residual if this is requested, otherwise copy directly into
      // the destination.
      for(i = 0;i < m_ucCount;i++) {
        // Read the reconstructed data out of the upsampler and push it
        // into D.
        m_ppUpsampler[i]->UpsampleRegion(r,m_ppDTemp[i]);
        //
        // And prepare the residual downsampler which is the output.
        if (m_ppResidualDownsampler[i]) {
          m_ppRTemp[i] = m_ppCTemp[i];
        } else {
          class QuantizedRow *rrow = BuildImageRow(m_pppRImage[i],m_pResidualHelper->ResidualFrameOf(),i);
          m_ppRTemp[i] = rrow->BlockAt(x)->m_Data;
        } 
        //
        // Build the output buffer for the downsampler that stored the original data.
        m_ppOriginalImage[i]->DownsampleRegion(x,y,(LONG *)(m_ppOriginalIBM[i]->ibm_pData));
      }
      // OriginalIBM has now the original image data buffered in the downsampler.
      // ppDTemp contains the upsampled reconstructed data as reference.
      // m_ppRTemp points to the destination buffer where the data is supposed to go.
      // This is either the DCT buffer itself, or the input buffer for the downsampler. 
      ctrafo->RGB2Residual(r,m_ppOriginalIBM,m_ppDTemp,m_ppRTemp);
      // Residual data is now in m_ppRTemp, which is either the final destination,
      // or the the C-Buffer. If it is in the C-Buffer, but it back into the
      // Downsampler.
      for(i = 0;i < m_ucCount;i++) {
        // Just collect the data in the downsampler for the time
        // being. Will be taken care of as soon as it is complete.
        if (m_ppResidualDownsampler[i]) {
          m_ppResidualDownsampler[i]->DefineRegion(x,y,m_ppCTemp[i]);
        } else {
          // Otherwise, already quantize now as the data is in its final destination.
          m_pResidualHelper->QuantizeResidual(m_ppDTemp[i],m_ppRTemp[i],i,x,y);
        }
      }
    }
    //
    // Remove the block line from the upsampler now,
    // and from the original image since that's already used.
    for(i = 0;i < m_ucCount;i++) {
      m_ppUpsampler[i]->RemoveBlocks(y);
      m_ppOriginalImage[i]->RemoveBlocks(y);
      // No further downsampling required? If so,
      // just push the residual out.
      if (m_ppResidualDownsampler[i] == NULL) { 
        class QuantizedRow *rrow = BuildImageRow(m_pppRImage[i],m_pResidualHelper->ResidualFrameOf(),i);
        m_pppRImage[i] = &(rrow->NextOf());
      }
    }
  }
  //
  // Now handle the downsampled versions of the residual once we are complete.
  for(i = 0;i < m_ucCount;i++) {
    if (m_ppResidualDownsampler[i]) { 
      LONG bx,by;
      RectAngle<LONG> blocks;
      //
      // Collect the downsampled blocks and push that into the DCT.
      m_ppResidualDownsampler[i]->GetCollectedBlocks(blocks);
      for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
        class QuantizedRow *qr = BuildImageRow(m_pppRImage[i],m_pResidualHelper->ResidualFrameOf(),i);
        for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
          LONG *dst = (qr)?(qr->BlockAt(bx)->m_Data):NULL;
          m_ppResidualDownsampler[i]->DownsampleRegion(bx,by,dst);
          m_pResidualHelper->QuantizeResidual(NULL,dst,i,bx,by);
        }
        m_ppResidualDownsampler[i]->RemoveBlocks(by);
        m_pppRImage[i] = &(qr->NextOf());
      }
    }
  }
}
///

/// BlockBitmapRequester::PullSourceData
// Get the source data from the source image(s)
// and place them into the downsampler and the original
// image buffer.
void BlockBitmapRequester::PullSourceData(const RectAngle<LONG> &region,class ColorTrafo *ctrafo)
{ 
  ULONG maxval   = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  RectAngle<LONG> r;
  LONG minx   = region.ra_MinX >> 3;
  LONG maxx   = region.ra_MaxX >> 3;
  LONG miny   = region.ra_MinY >> 3;
  LONG maxy   = region.ra_MaxY >> 3;
  LONG x,y;
  UBYTE i;
  //
  // First part: Collect the data from
  // the user and push it into the color transformer buffer.
  // For that first build the downsampler.
  for(i = 0;i < m_ucCount;i++) {
    if (m_ppDownsampler[i]) {
      m_ppDownsampler[i]->SetBufferedRegion(region);
    }   
    // Ditto for the original image, but do not throw old stuff away.
    if (m_pResidualHelper) {
      if (m_ppOriginalImage[i]) {
        m_ppOriginalImage[i]->ExtendBufferedRegion(region);
      }
      // For openloop coding, fill the upsampler with the original data instead
      // of the reconstructed data.
      if (m_bOpenLoop && m_ppUpsampler[i]) {
        // Note that the upsampler requires block coordinates, not image coordinates.   
        // Also, the dummy open loop upsampler is only an image buffer with 1x1
        // subsampling factors, thus coordinates carry over directly.
        r.ra_MinX = minx;
        r.ra_MinY = miny;
        r.ra_MaxX = maxx;
        r.ra_MaxY = maxy;
        m_ppUpsampler[i]->ExtendBufferedRegion(r);
      }
    }
  }
  //
  //
  // Loop over the blocks in the available region.
  for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
    r.ra_MaxY = (r.ra_MinY & -8) + 7;
    if (r.ra_MaxY > region.ra_MaxY)
      r.ra_MaxY = region.ra_MaxY;
    
    for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
      r.ra_MaxX = (r.ra_MinX & -8) + 7;
      if (r.ra_MaxX > region.ra_MaxX)
        r.ra_MaxX = region.ra_MaxX;
      
      // If the user supplied a dedicated LDR image.
      if (hasLDRImage()) {
        for(i = 0;i < m_ucCount;i++) {
          ExtractLDRBitmap(m_ppTempIBM[i],r,i);
        }
        
        ctrafo->LDRRGB2YCbCr(r,m_ppTempIBM,m_ppCTemp); 
        
        // Extract now the HDR image.
        for(i = 0;i < m_ucCount;i++) {      
          ExtractBitmap(m_ppTempIBM[i],r,i);
        }
      } else {
        // Take the LDR from the HDR image.
        for(i = 0;i < m_ucCount;i++) {
          // Collect the source data.
          ExtractBitmap(m_ppTempIBM[i],r,i);
        }
        //
        // Run the color transformer.
        ctrafo->RGB2YCbCr(r,m_ppTempIBM,m_ppCTemp);
      }
      
      // Now push the transformed data into either the downsampler, 
      // or the forward DCT block row.
      for(i = 0;i < m_ucCount;i++) {
        //
        // Now for the actual downsampling.
        if (m_ppDownsampler[i]) {
          // Just collect the data in the downsampler for the time
          // being. Will be taken care of as soon as it is complete.
          m_ppDownsampler[i]->DefineRegion(x,y,m_ppCTemp[i]);
        } else { 
          class QuantizedRow *qrow = BuildImageRow(m_pppQImage[i],m_pFrame,i);
          LONG *dst                = qrow->BlockAt(x)->m_Data;
          LONG *src                = m_ppCTemp[i];
          if (m_bDeRing) {
            m_ppDeRinger[i]->DeRing(src,dst,(maxval + 1) >> 1);
          } else {
            m_ppDCT[i]->TransformBlock(src,dst,(maxval + 1) >> 1);
          }
          if (m_bOptimize) {
            m_pFrame->OptimizeDCTBlock(x,y,i,m_ppDCT[i],dst);
          }
        }
      }
      //
      // For residual coding: Also keep the original image, undownsampled
      // here in the downsampler base until we can make use of it and
      // the reconstructed image becomes available.
      if (m_pResidualHelper) {
        // For openloop coding, store the transformed source data now
        // in the upsampler. This will be used later to compute
        // the residual. Note that CTemp contains now the LDR
        // image.
        if (m_bOpenLoop) {
          for(i = 0;i < m_ucCount;i++) {
            assert(m_ppUpsampler[i]);
            m_ppUpsampler[i]->DefineRegion(x,y,m_ppCTemp[i]);
          }
        }
        //
        // Get the original HDR image unaltered, move it to the
        // dummy downsampler to store it there until it is needed.
        ctrafo->RGB2RGB(r,m_ppTempIBM,m_ppCTemp);
        //
        for(i = 0;i < m_ucCount;i++) {
          // Get the original image unaltered, move it to the dummy downsampler
          // to store it there until it is needed.
          assert(m_ppOriginalImage[i]);
          m_ppOriginalImage[i]->DefineRegion(x,y,m_ppCTemp[i]);
        }
      }
      //
      // If residual coding is enabled, all the data should go into the
      // downsampler, even though it does not sample much, but rather
      // acts as image buffer.
      for(i = 0;i < m_ucCount;i++) {
        assert(m_ppDownsampler[i] || m_pResidualHelper == NULL);
      }
    }
    //
    AdvanceQRows();
  }
}
///

/// BlockBitmapRequester::EncodeUnsampled
// The encoding procedure without subsampling, which is the much simpler case.
void BlockBitmapRequester::EncodeUnsampled(const RectAngle<LONG> &region,class ColorTrafo *ctrafo)
{  
  ULONG maxval = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  RectAngle<LONG> r;
  ULONG minx   = region.ra_MinX >> 3;
  ULONG maxx   = region.ra_MaxX >> 3;
  ULONG miny   = region.ra_MinY >> 3;
  ULONG maxy   = region.ra_MaxY >> 3;
  ULONG x,y;
  UBYTE i;
  
  for(y = miny,r.ra_MinY = region.ra_MinY;y <= maxy;y++,r.ra_MinY = r.ra_MaxY + 1) {
    r.ra_MaxY = (r.ra_MinY & -8) + 7;
    if (r.ra_MaxY > region.ra_MaxY)
      r.ra_MaxY = region.ra_MaxY;
    
    for(x = minx,r.ra_MinX = region.ra_MinX;x <= maxx;x++,r.ra_MinX = r.ra_MaxX + 1) {
      r.ra_MaxX = (r.ra_MinX & -8) + 7;
      if (r.ra_MaxX > region.ra_MaxX)
        r.ra_MaxX = region.ra_MaxX;
      
      //
      // If the user supplied a dedicated LDR image.
      if (hasLDRImage()) {
        for(i = 0;i < m_ucCount;i++) {      
          ExtractLDRBitmap(m_ppTempIBM[i],r,i);
        }
        
        ctrafo->LDRRGB2YCbCr(r,m_ppTempIBM,m_ppCTemp); 
        
        // Extract the HDR image now.
        for(i = 0;i < m_ucCount;i++) {      
          ExtractBitmap(m_ppTempIBM[i],r,i);
        }
      } else {
        for(i = 0;i < m_ucCount;i++) {      
          ExtractBitmap(m_ppTempIBM[i],r,i);
        }
        
        //if (x == 245 && y == 126 && m_ucCount == 1)
        //printf("gotcha");     
        ctrafo->RGB2YCbCr(r,m_ppTempIBM,m_ppCTemp);
      }
      
      for(i = 0;i < m_ucCount;i++) {
        class QuantizedRow *qrow = BuildImageRow(m_pppQImage[i],m_pFrame,i);
        LONG *dst = qrow->BlockAt(x)->m_Data;
        LONG *src = m_ppCTemp[i];
        
        if (m_bDeRing) {
          m_ppDeRinger[i]->DeRing(src,dst,(maxval + 1) >> 1);
        } else {
          m_ppDCT[i]->TransformBlock(src,dst,(maxval + 1) >> 1);
        }
        if (m_bOptimize) {
          m_pFrame->OptimizeDCTBlock(x,y,i,m_ppDCT[i],dst);
        }
      }
      //
      // If any residuals are required, compute them now.
      if (m_pResidualHelper) {
        for(i = 0;i < m_ucCount;i++) { 
          class QuantizedRow *qrow = *m_pppQImage[i];
          class QuantizedRow *rrow = BuildImageRow(m_pppRImage[i],m_pResidualHelper->ResidualFrameOf(),i);
          assert(qrow && rrow);
          m_ppQTemp[i] = qrow->BlockAt(x)->m_Data;
          m_ppRTemp[i] = rrow->BlockAt(x)->m_Data;
          if (m_bOpenLoop) {
            memcpy(m_ppDTemp[i],m_ppCTemp[i],64 * sizeof(LONG));
          } else {
            m_ppDCT[i]->InverseTransformBlock(m_ppDTemp[i],m_ppQTemp[i],(maxval + 1) >> 1);
          }
        }
        // Step One:
        // Feed now the color transformer with the residual data.
        //if (x == 117 && y == 34)
        //printf("gotcha");
        ctrafo->RGB2Residual(r,m_ppTempIBM,m_ppDTemp,m_ppRTemp);
        //
        // Step two: Compute the residuals by means of the color transformer.
        // This also computes the forwards transformation of the residual.
        // Quantization and DCT are still missing.
        for(i = 0;i < m_ucCount;i++) { 
          m_pResidualHelper->QuantizeResidual(m_ppDTemp[i],m_ppRTemp[i],i,x,y);
        }
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
///

/// BlockBitmapRequester::CropEncodingRegion
// First step of a region encoder: Find the region that can be pulled in the next step,
// from a rectangle request. This potentially shrinks the rectangle, which should be
// initialized to the full image.
void BlockBitmapRequester::CropEncodingRegion(RectAngle<LONG> &region,const struct RectangleRequest *)
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

/// BlockBitmapRequester::RequestUserDataForEncoding
// Request user data for encoding for the given region, potentially clip the region to the
// data available from the user.
void BlockBitmapRequester::RequestUserDataForEncoding(class BitMapHook *bmh,RectAngle<LONG> &region,bool alpha)
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

/// BlockBitmapRequester::EncodeRegion
// Encode a region with downsampling and color transformation
void BlockBitmapRequester::EncodeRegion(const RectAngle<LONG> &region)
{
  class ColorTrafo *ctrafo = ColorTrafoOf(true,false);
 
  if (m_bSubsampling) { 
    // Step one: Pull the source data into the input buffers
    // and generate the Q-output (legacy output).
    PullSourceData(region,ctrafo);
    //
    // Now create the residual if we need to.
    if (m_pResidualHelper) {
      AdvanceRRows(region,ctrafo);
    }
  } else { 
    // No downsampling required. Much simpler here.
    EncodeUnsampled(region,ctrafo);
  }
}
///

/// BlockBitmapRequester::ReconstructUnsampled
// Reconstruct a region not using any subsampling.
void BlockBitmapRequester::ReconstructUnsampled(const struct RectangleRequest *rr,const RectAngle<LONG> &orgregion,
                                                ULONG maxmcu,class ColorTrafo *ctrafo)
{   
  ULONG maxval  = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  RectAngle<LONG> r;
  RectAngle<LONG> region = orgregion;
  SubsampledRegion(region,rr);
  ULONG minx   = region.ra_MinX >> 3;
  ULONG maxx   = region.ra_MaxX >> 3;
  ULONG miny   = region.ra_MinY >> 3;
  ULONG maxy   = region.ra_MaxY >> 3;
  ULONG x,y;
  UBYTE i;
  
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
        LONG *dst = m_ppCTemp[i];
        // Bitmap extraction must go here as the components requested
        // refer to components in YUV space, and not in target RGB space.
        ExtractBitmap(m_ppTempIBM[i],r,i);
        if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent && m_ppDCT[i]) {
          class QuantizedRow *qrow = *m_pppQImage[i];
          const LONG *src = (qrow)?(qrow->BlockAt(x)->m_Data):(NULL);
          m_ppDCT[i]->InverseTransformBlock(dst,src,(maxval + 1) >> 1);
        } else {
          memset(dst,0,sizeof(LONG) * 64);
        }
      }
      //
      // Perform the color transformation now.
      if (m_pResidualHelper) {
        for(i = rr->rr_usFirstComponent; i <= rr->rr_usLastComponent; i++) {
          class QuantizedRow *rrow = *m_pppRImage[i];
          m_pResidualHelper->DequantizeResidual(m_ppCTemp[i],m_ppDTemp[i],rrow->BlockAt(x)->m_Data,i);
        }
      }
      //
      // Otherwise, the residual remains unused.
      ctrafo->YCbCr2RGB(r,m_ppTempIBM,m_ppCTemp,m_ppDTemp);
    } // of loop over x
    //
    // Advance the rows.
    for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
      class QuantizedRow *qrow = *m_pppQImage[i];
      class QuantizedRow *rrow = *m_pppRImage[i];
      if (qrow) m_pppQImage[i] = &(qrow->NextOf());
      if (rrow) m_pppRImage[i] = &(rrow->NextOf());
    }
  }
}
///

/// BlockBitmapRequester::PullQData
// Pull the quantized data into the upsampler if there is one.
void BlockBitmapRequester::PullQData(const struct RectangleRequest *rr,const RectAngle<LONG> &region)
{
  ULONG maxval  = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  UBYTE i;

  for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    class UpsamplerBase *up;  // upsampler
    LONG bx,by;
    RectAngle<LONG> blocks;
    //
    // Compute the region of blocks
    if ((up = m_ppUpsampler[i])) {
      //
      // Feed the upsampler.
      blocks = region;
      up->SetBufferedImageRegion(blocks);
      //
      for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
        class QuantizedRow *qrow = *m_pppQImage[i];
        for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
          LONG *src = (qrow)?(qrow->BlockAt(bx)->m_Data):NULL;
          LONG dst[64];
          if (m_ppDCT[i]) {
            m_ppDCT[i]->InverseTransformBlock(dst,src,(maxval + 1) >> 1);
          } else {
            memset(dst,0,sizeof(dst));
          }
          up->DefineRegion(bx,by,dst);
        }
        if (qrow) m_pppQImage[i] = &(qrow->NextOf());
      }
    }
  }
}
///

/// BlockBitmapRequester::PullRData
// Get the residual data and potentially move it into the
// residual upsampler
void BlockBitmapRequester::PullRData(const struct RectangleRequest *rr,const RectAngle<LONG> &region)
{ 
  UBYTE i;

  for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    class UpsamplerBase *up;  // upsampler
    LONG bx,by;
    RectAngle<LONG> blocks;
    //
    // Compute the region of blocks
    if ((up = m_ppResidualUpsampler[i])) {
      //
      // Feed the upsampler.
      blocks = region;
      up->SetBufferedImageRegion(blocks);
      //
      for(by = blocks.ra_MinY;by <= blocks.ra_MaxY;by++) {
        class QuantizedRow *rrow = *m_pppRImage[i];
        for(bx = blocks.ra_MinX;bx <= blocks.ra_MaxX;bx++) {
          LONG *src = (rrow)?(rrow->BlockAt(bx)->m_Data):NULL;
          LONG dst[64];
          m_pResidualHelper->DequantizeResidual(NULL,dst,src,i);
          up->DefineRegion(bx,by,dst);
        }
        if (rrow) m_pppRImage[i] = &(rrow->NextOf());
      }
    }
  }
}
///

/// BlockBitmapRequester::PushReconstructedData
// Generate the final output of the reconstructed data.
void BlockBitmapRequester::PushReconstructedData(const struct RectangleRequest *rr,const RectAngle<LONG> &region,
                                                 ULONG maxmcu,class ColorTrafo *ctrafo)
{  
  ULONG maxval = (1UL << m_pFrame->HiddenPrecisionOf()) - 1;
  RectAngle<LONG> r;
  ULONG minx   = region.ra_MinX >> 3;
  ULONG maxx   = region.ra_MaxX >> 3;
  ULONG miny   = region.ra_MinY >> 3;
  ULONG maxy   = region.ra_MaxY >> 3;
  ULONG x,y;
  UBYTE i;
  
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
        ExtractBitmap(m_ppTempIBM[i],r,i);
        if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
          if (m_ppUpsampler[i]) {
            // Upsampled case, take from the upsampler, transform
            // into the color buffer.
            m_ppUpsampler[i]->UpsampleRegion(r,m_ppCTemp[i]);
          } else if (m_ppDCT[i]) {
            class QuantizedRow *qrow = *m_pppQImage[i];
            LONG *src = (qrow)?(qrow->BlockAt(x)->m_Data):NULL;
            // Plain case. Transform directly into the color buffer.
            m_ppDCT[i]->InverseTransformBlock(m_ppCTemp[i],src,(maxval + 1) >> 1);
          } else {
            memset(m_ppCTemp[i],0,sizeof(LONG) * 64);
          }
        } else {
          // Not requested, zero the buffer.
          memset(m_ppCTemp[i],0,sizeof(LONG) * 64);
        }
        //
        // Now for the residual image.
        if (m_pResidualHelper) {
          if (i >= rr->rr_usFirstComponent && i <= rr->rr_usLastComponent) {
            if (m_ppResidualUpsampler[i]) {
              m_ppResidualUpsampler[i]->UpsampleRegion(r,m_ppDTemp[i]);
            } else {
              class QuantizedRow *rrow = *m_pppRImage[i];
              m_pResidualHelper->DequantizeResidual(NULL,m_ppDTemp[i],rrow->BlockAt(x)->m_Data,i);
            }
          }
        }
      }
      ctrafo->YCbCr2RGB(r,m_ppTempIBM,m_ppCTemp,m_ppDTemp);
    }
    //
    // Advance the quantized rows for the non-subsampled components,
    // upsampled components have been advanced above.
    for(i = 0;i < m_ucCount;i++) {
      if (m_ppUpsampler[i] == NULL) {
        class QuantizedRow *qrow = *m_pppQImage[i];
        if (qrow) m_pppQImage[i] = &(qrow->NextOf());
      }
      if (m_pResidualHelper && m_ppResidualUpsampler[i] == NULL) {
        class QuantizedRow *rrow = *m_pppRImage[i];
        if (rrow) m_pppRImage[i] = &(rrow->NextOf());
      }
    }
  }
}
///

/// BlockBitmapRequester::RequestUserDataForDecoding
// Pull data buffers from the user data bitmap hook
void BlockBitmapRequester::RequestUserDataForDecoding(class BitMapHook *bmh,RectAngle<LONG> &region,
                                                      const struct RectangleRequest *rr,bool alpha)
{
  UBYTE i;
  
  m_ulMaxMCU = MAX_ULONG;

  ResetBitmaps();
  
  for(i = rr->rr_usFirstComponent;i <= rr->rr_usLastComponent;i++) {
    RequestUserData(bmh,region,i,alpha);
    ULONG max = (BitmapOf(i).ibm_ulHeight >> 3) - 1;
    if (max < m_ulMaxMCU)
      m_ulMaxMCU = max;
  }
}
///

/// BlockBitmapRequester::ReconstructRegion
// Reconstruct a block, or part of a block
void BlockBitmapRequester::ReconstructRegion(const RectAngle<LONG> &region,const struct RectangleRequest *rr)
{
  class ColorTrafo *ctrafo = ColorTrafoOf(false,!rr->rr_bColorTrafo);

  if (ctrafo == NULL)
    return;
  
  if (m_bSubsampling && rr->rr_bUpsampling) {
    //
    // Feed data into the regular upsampler
    PullQData(rr,region);
    //
    // Is there a residual to reconstruct?
    if (m_pResidualHelper) { 
      PullRData(rr,region);
    }
    //
    // Now push blocks into the color transformer from the upsampler.
    PushReconstructedData(rr,region,m_ulMaxMCU,ctrafo);
  } else { 
    // direct case, no upsampling required, the easy case.
    ReconstructUnsampled(rr,region,m_ulMaxMCU,ctrafo);
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
