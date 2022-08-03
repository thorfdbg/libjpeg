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
** This class represents a single frame and the frame dimensions.
**
** $Id: frame.cpp,v 1.134 2022/08/03 12:19:58 thor Exp $
**
*/

/// Includes
#include "tools/environment.hpp"
#include "tools/checksum.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "io/bytestream.hpp"
#include "io/checksumadapter.hpp"
#include "std/string.hpp"
#include "codestream/tables.hpp"
#include "codestream/image.hpp"
#include "marker/scan.hpp"
#include "control/bitmapctrl.hpp"
#include "control/lineadapter.hpp"
#include "control/blockbitmaprequester.hpp"
#include "control/linebitmaprequester.hpp"
#include "control/hierarchicalbitmaprequester.hpp"
#include "control/blocklineadapter.hpp"
#include "control/linelineadapter.hpp"
#include "control/residualblockhelper.hpp"
#include "boxes/databox.hpp"
#include "boxes/checksumbox.hpp"
#include "dct/dct.hpp"
///

/// Frame::Frame
// Construct a frame object
Frame::Frame(class Image *image,class Tables *tables,ScanType t)
  : JKeeper(tables->EnvironOf()), m_pParent(image), m_pNext(NULL), m_pTables(tables), 
    m_pScan(NULL), m_pLast(NULL), m_pCurrent(NULL), m_pImage(NULL),
    m_pBlockHelper(NULL), m_Type(t), m_ucPrecision(0), m_ucDepth(0), 
    m_ppComponent(NULL), m_pCurrentRefinement(NULL), m_pAdapter(NULL),
    m_bWriteDNL(false), m_bBuildRefinement(false),
    m_bCreatedRefinement(false), m_bEndOfFrame(false), m_bStartedTables(false),
    m_usRefinementCount(0)
{
}
///

/// Frame::~Frame
Frame::~Frame(void)
{
  class Scan *scan;
  int i;
  
  if (m_ppComponent) {
    for(i = 0;i < m_ucDepth;i++) {
      delete m_ppComponent[i];
    }
    m_pEnviron->FreeMem(m_ppComponent,sizeof(class Component *) * m_ucDepth);
  }

  while((scan = m_pScan)) {
    m_pScan = scan->NextOf();
    delete scan;
  }

  delete m_pAdapter;
  delete m_pBlockHelper;
}
///

/// Frame::ParseMarker
void Frame::ParseMarker(class ByteStream *io)
{
  class Frame *first = ImageOf()->FirstFrameOf();
  LONG len = io->GetWord();
  LONG data;
  int i;

  if (len < 8)
    JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","start of frame marker size invalid");

  m_ucPrecision = io->Get();

  switch(m_Type) {
  case Lossless:
  case DifferentialLossless:
  case ACLossless:
  case ACDifferentialLossless:
  case JPEG_LS: 
    if (m_ucPrecision < 2 || m_ucPrecision > 16)
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame precision in lossless mode must be between 2 and 16");
    break;
  case Residual:
  case ACResidual:
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
    if (m_ucPrecision < 2 || m_ucPrecision > 17)
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame precision in residual mode must be between 2 and 17");
    break;
  case Baseline:
    if (m_ucPrecision != 8)
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame precision in baseline mode must be 8");
    break;
  default: // all others.
    if (m_ucPrecision != 8 && m_ucPrecision != 12)
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame precision in lossy mode must be 8 or 12");
    break;
  }

  data = io->GetWord();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame marker run out of data");

  m_ulHeight = data; // may be 0. In that case, the DNL marker will provide it.

  data = io->GetWord();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame marker run out of data");
  if (data == 0)
    JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","image width must not be zero");

  m_ulWidth = data;

  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame marker run out of data");

  switch(m_Type) {
  case Progressive:
  case ACProgressive:
  case ACDifferentialProgressive:
    if (data <= 0 || data > 4)
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","number of components must be between 1 and 4 for progressive mode");
    m_ucDepth = data;
    break;
  default:
    if (data <= 0 || data > 255)
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","number of components must be between 1 and 255");
    m_ucDepth = data;
    break;
  }

  len -= 8;
  if (len != 3 * m_ucDepth)
    JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","frame header marker size is invalid");

  assert(m_ppComponent == NULL);
  m_ppComponent = (class Component **)m_pEnviron->AllocMem(sizeof(class Component *) * m_ucDepth);
  memset(m_ppComponent,0,sizeof(class Component *) * m_ucDepth);

  m_ucMaxMCUWidth  = 0;
  m_ucMaxMCUHeight = 0;
  for(i = 0;i < m_ucDepth;i++) {
    m_ppComponent[i] = new(m_pEnviron) class Component(m_pEnviron,i,m_ucPrecision);
    m_ppComponent[i]->ParseMarker(io);
    
    if (m_ppComponent[i]->MCUWidthOf() > m_ucMaxMCUWidth)
      m_ucMaxMCUWidth = m_ppComponent[i]->MCUWidthOf();

    if (m_ppComponent[i]->MCUHeightOf() > m_ucMaxMCUHeight)
      m_ucMaxMCUHeight = m_ppComponent[i]->MCUHeightOf();
  }
  //
  // Now complete the components: Subsampling requires maximum.
  for(i = 0;i < m_ucDepth;i++) {
    // Ensure the MCU dimensions are consistent throughout the hierarchical process.
    // Note that "first" may, in fact, be identical to this very class.
    if (first->ComponentOf(i)->MCUWidthOf()  != m_ppComponent[i]->MCUWidthOf() ||
        first->ComponentOf(i)->MCUHeightOf() != m_ppComponent[i]->MCUHeightOf())
      JPG_THROW(MALFORMED_STREAM,"Frame::ParseMarker","MCU dimensions are not consistent throughout the process, cannot decode");
    m_ppComponent[i]->SetSubsampling(m_ucMaxMCUWidth,m_ucMaxMCUHeight);
  }
}
///


/// Frame::ComputeMCUSizes
// Compute the MCU sizes of the components from the subsampling values
void Frame::ComputeMCUSizes(void)
{ 
  UBYTE i;
  UWORD maxx,maxy;
  
  // Compute the maximum MCU width and height.
  assert(m_ppComponent[0]);
  maxx = m_ppComponent[0]->SubXOf();
  maxy = m_ppComponent[0]->SubYOf();

  for(i = 1;i < m_ucDepth;i++) {
    assert(m_ppComponent[i]);
    maxx = m_ppComponent[i]->SubXOf() * maxx / gcd(m_ppComponent[i]->SubXOf(),maxx);
    maxy = m_ppComponent[i]->SubYOf() * maxy / gcd(m_ppComponent[i]->SubYOf(),maxy);
    if (maxx > MAX_UBYTE || maxy > MAX_UBYTE)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::ComputeMCUSizes",
                "the smallest common multiple of all subsampling factors must be smaller than 255");
  }
  
  m_ucMaxMCUWidth  = maxx;
  m_ucMaxMCUHeight = maxy;

  for(i = 0;i < m_ucDepth;i++) {
    m_ppComponent[i]->SetMCUSize(maxx,maxy);
  } 
  //
  // Check whether the scm is actually part of the MCU sizes written. If not,
  // then JPEG cannot support this subsampling setting. Weird.
  for(i = 0;i < m_ucDepth;i++) {
    if (m_ppComponent[i]->SubXOf() != m_ucMaxMCUWidth  / m_ppComponent[i]->MCUWidthOf() ||
        m_ppComponent[i]->SubYOf() != m_ucMaxMCUHeight / m_ppComponent[i]->MCUHeightOf())
      JPG_THROW(INVALID_PARAMETER,"Frame::ComputeMCUSizes",
                "the given set of subsampling parameters is not supported by JPEG");
  }
}
///

/// Frame::WriteMarker
// Write the frame header
void Frame::WriteMarker(class ByteStream *io)
{
  UWORD len = 8 + 3 * m_ucDepth;
  int i;

  io->PutWord(len);
  io->Put(m_ucPrecision);

  assert(m_ulHeight <= MAX_UWORD);
  assert(m_ulWidth  <= MAX_UWORD && m_ulWidth > 0);

  if (m_bWriteDNL) {
    io->PutWord(0);
  } else {
    io->PutWord(m_ulHeight);
  }
  io->PutWord(m_ulWidth);
  io->Put(m_ucDepth);

  ComputeMCUSizes();
  
  for(i = 0;i < m_ucDepth;i++) {
    m_ppComponent[i]->WriteMarker(io);
  }
}
///

/// Frame::WriteTrailer
// Write the scan trailer of this frame. This is only the
// DNL marker if it is enabled.
void Frame::WriteTrailer(class ByteStream *io)
{       
  assert(m_pCurrent);

  //
  // The DNL marker does not go into the refinement
  // scan.
  if (!m_pCurrent->isHidden() && m_bWriteDNL) {
    io->PutWord(0xffdc); // DNL marker
    io->PutWord(4);      // its size
    io->PutWord(m_ulHeight); // the height
    m_bWriteDNL = false;
  }
}
///

/// Frame::CompleteRefinementScan
// Complete the current refinement scan if there is one.
void Frame::CompleteRefimentScan(class ByteStream *io)
{
  assert(m_pCurrent);

  if (m_pCurrent->isHidden()) {
    assert(m_pCurrentRefinement);
    m_pCurrentRefinement->Flush(io,m_usRefinementCount++);
    m_pCurrentRefinement = NULL;
  }
}
///

/// Frame::FindComponent
// Find a component by a component identifier. Throws if the component does not exist.
class Component *Frame::FindComponent(UBYTE id) const
{
  int i;

  for(i = 0;i < m_ucDepth;i++) {
    if (m_ppComponent[i]->IDOf() == id)
      return m_ppComponent[i];
  }

  JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::FindComponent","found a component ID that does not exist");
  return NULL; // dummy.
}
///

/// Frame::DefineComponent
// Define a component for writing. Must be called exactly once per component for encoding.
// idx is the component index (not its label, which is generated automatically), and
// the component subsampling factors.
class Component *Frame::DefineComponent(UBYTE idx,UBYTE subx,UBYTE suby)
{
  if (m_ucDepth == 0)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::DefineComponent",
              "Frame depth must be specified first before defining the component properties");

  if (m_ucPrecision == 0)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::DefineComponent",
              "Frame precision must be specified first before defining the component properties");

  if (idx >= m_ucDepth)
    JPG_THROW(OVERFLOW_PARAMETER,"Frame::DefineComponent",
              "component index is out of range, must be between 0 and depth-1");
  
  if (m_ppComponent == NULL) {
    m_ppComponent = (class Component **)m_pEnviron->AllocMem(sizeof(class Component *) * m_ucDepth);
    memset(m_ppComponent,0,sizeof(class Component *) * m_ucDepth);
  }

  if (m_ppComponent[idx])
    JPG_THROW(OBJECT_EXISTS,"Frame::DefineComponent",
              "the indicated component is already defined");

  m_ppComponent[idx] = new(m_pEnviron) Component(m_pEnviron,idx,m_ucPrecision,subx,suby);

  return m_ppComponent[idx];
}
///

/// Frame::HiddenPrecisionOf
// Return the precision including the hidden bits.
UBYTE Frame::HiddenPrecisionOf(void) const
{
  return m_ucPrecision + m_pTables->HiddenDCTBitsOf();
}
///

/// Frame::PointPreShiftOf
// Return the point preshift, the adjustment of the
// input samples by a shift that moves them into the
// limits of JPEG. This is the parameter R_b
UBYTE Frame::PointPreShiftOf(void) const
{
  if (m_pTables) {
    class MergingSpecBox *res;
    
    if ((res = m_pTables->ResidualSpecsOf())) {
      return res->ResidualBitsOf();
    }
  }
  return 0;
}
///

/// Frame::CreateSequentialScanParameters
// Helper function to create a regular scan from the tags.
// There are no scan tags here, instead all components are included.
// If breakup is set, then each component gets its own scan, otherwise
// groups of four components get into one scan.
void Frame::CreateSequentialScanParameters(bool breakup,ULONG tagoffset,const struct JPG_TagItem *tags)
{ 
  UBYTE maxdepth = 4;
  UBYTE depth    = m_ucDepth;
  UBYTE comp     = 0;  

  if (breakup)
    maxdepth = 1;

  // Create multiple scans for more than maxdepth components.
  while(depth) {
    class Scan *scan;
    UBYTE curdepth = (depth > maxdepth)?(maxdepth):(depth);
    struct JPG_TagItem ctags[] = {
      JPG_ValueTag((curdepth > 0)?(JPGTAG_SCAN_COMPONENT0 + tagoffset):JPGTAG_TAG_IGNORE,comp + 0),
      JPG_ValueTag((curdepth > 1)?(JPGTAG_SCAN_COMPONENT1 + tagoffset):JPGTAG_TAG_IGNORE,comp + 1),
      JPG_ValueTag((curdepth > 2)?(JPGTAG_SCAN_COMPONENT2 + tagoffset):JPGTAG_TAG_IGNORE,comp + 2),
      JPG_ValueTag((curdepth > 3)?(JPGTAG_SCAN_COMPONENT3 + tagoffset):JPGTAG_TAG_IGNORE,comp + 3),
      JPG_Continue(tags)
    };
    
    scan = new(m_pEnviron) class Scan(this);
    if (m_pScan == NULL) {
      assert(m_pScan == NULL && m_pLast == NULL);
      m_pScan = scan;
    } else {
      assert(m_pScan && m_pLast);
      m_pLast->TagOn(scan);
    }
    m_pLast = scan;
    scan->InstallDefaults(curdepth,tagoffset,ctags);
    comp += maxdepth;
    depth-= curdepth;
  }
}
///

/// Frame::CreateProgressiveScanParameters
// Helper function to create progressive scans. These need to be broken
// up over several components. A progressive scan cannot contain more
// than one component if it includes AC parameters.
void Frame::CreateProgressiveScanParameters(bool breakup,ULONG tagoffset,const struct JPG_TagItem *,
                                            const struct JPG_TagItem *scantags)
{  
  UBYTE i;
  
  // Frist check whether the "chroma" mechanism is used to create multiple scans.
  if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENTS_CHROMA + tagoffset) || 
      scantags->FindTagItem(JPGTAG_SCAN_COMPONENTS_CHROMA)) {
    // This actually creates a group of tags if the spectral selection contains
    // AC bands.
    if (m_ucDepth > 1) {
      // Need to break up the scan into several scans if AC components are included.
      if (breakup) {
        struct JPG_TagItem ctags[] = {
          JPG_ValueTag(JPGTAG_SCAN_COMPONENT0 + tagoffset,0),
          JPG_Continue(scantags)
        };
        for(i = 1; i < m_ucDepth;i++) {
          class Scan *scan = new(m_pEnviron) class Scan(this);
          if (m_pScan == NULL) {
            m_pScan = scan;
          } else {
            m_pLast->TagOn(scan);
          }
          m_pLast = scan;
          ctags[0].ti_Data.ti_lData = i;
          scan->InstallDefaults(1,tagoffset,ctags);
        }
      } else {
        struct JPG_TagItem ctags[] = {
          JPG_ValueTag((m_ucDepth > 1)?(JPGTAG_SCAN_COMPONENT0 + tagoffset):JPGTAG_TAG_IGNORE,1),
          JPG_ValueTag((m_ucDepth > 2)?(JPGTAG_SCAN_COMPONENT1 + tagoffset):JPGTAG_TAG_IGNORE,2),
          JPG_ValueTag((m_ucDepth > 3)?(JPGTAG_SCAN_COMPONENT2 + tagoffset):JPGTAG_TAG_IGNORE,3),
          JPG_Continue(scantags)
        };
        class Scan *scan = new(m_pEnviron) class Scan(this);
        if (m_pScan == NULL) {
          m_pScan = scan;
        } else {
          m_pLast->TagOn(scan);
        }
        m_pLast = scan;
        scan->InstallDefaults(m_ucDepth - 1,tagoffset,ctags);
      }
    } // Nothing to do if chroma channels are not present.
  } else {
    // The "chroma" magic is not used.
    UBYTE create,depth = 0;
    if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT0 + tagoffset) || 
        scantags->FindTagItem(JPGTAG_SCAN_COMPONENT0)) depth++;
    if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT1 + tagoffset) ||
        scantags->FindTagItem(JPGTAG_SCAN_COMPONENT1)) depth++;
    if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT2 + tagoffset) ||
        scantags->FindTagItem(JPGTAG_SCAN_COMPONENT2)) depth++;
    if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT3 + tagoffset) ||
        scantags->FindTagItem(JPGTAG_SCAN_COMPONENT3)) depth++;
    //
    // The number of scans to create.
    create = (depth == 0)?(m_ucDepth):(depth);
    // Break up the scans?
    if (breakup) {
      struct JPG_TagItem ctags[] = {
        JPG_ValueTag(JPGTAG_SCAN_COMPONENT0 + tagoffset,0),
        JPG_ValueTag(JPGTAG_SCAN_COMPONENT1 + tagoffset,0),
        JPG_ValueTag(JPGTAG_SCAN_COMPONENT2 + tagoffset,0),
        JPG_ValueTag(JPGTAG_SCAN_COMPONENT3 + tagoffset,0),
        JPG_Continue(scantags)
      };
      for(i = 0;i < create;i++) {
        const struct JPG_TagItem *comp = scantags->FindTagItem(JPGTAG_SCAN_COMPONENT0 + i + tagoffset);
        if (comp == NULL) 
          comp = scantags->FindTagItem(JPGTAG_SCAN_COMPONENT0 + i);
        if (depth == 0 || comp) {
          ctags[0].ti_Data.ti_lData = (comp)?(comp->ti_Data.ti_lData):(i);
          class Scan *scan = new(m_pEnviron) class Scan(this);
          if (m_pScan == NULL) {
            m_pScan = scan;
          } else {
            m_pLast->TagOn(scan);
          }
          m_pLast = scan;
          scan->InstallDefaults(1,tagoffset,ctags);
        }
      }
    } else {
      class Scan *scan = new(m_pEnviron) class Scan(this);
      if (m_pScan == NULL) {
        m_pScan = scan;
      } else {
        m_pLast->TagOn(scan);
      }
      m_pLast = scan;
      scan->InstallDefaults(create,tagoffset,scantags);
    }
  }
}
///

/// Frame::InstallDefaultParameters
// Define default scan parameters. Returns the scan for further refinement if required.
// tagoffset is an offset added to the tags - used to read from the residual scan types
// rather the regular ones if this is a residual frame.
class Scan *Frame::InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,UBYTE prec,
                                            bool writednl,const UBYTE *psubx,const UBYTE *psuby,
                                            ULONG tagoffset,
                                            const struct JPG_TagItem *tags)
{
  UBYTE i;
  bool breakup;
  bool colortrafo = m_pTables->hasSeparateChroma(m_ucDepth);
  
  if (m_pScan || m_ucDepth != 0 || m_ucPrecision != 0)
    JPG_THROW(OBJECT_EXISTS,"Frame::InstallDefaultScanParameters","the scan has already been installed");

  if (width > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters","image dimensions must be < 65536");
  m_ulWidth = width;

  if (height > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters","image dimensions must be < 65535");
  m_ulHeight = height;

  if (depth < 1 || depth > 4)
    JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters","image depth must be between 1 and 4");
  m_ucDepth     = depth;
  //
  // Potentially clamp the precision to be in range. Only for the DCT operations.
  m_ucPrecision = prec;
  //
  // Check the validity of the precision.
  switch(m_Type) {
  case Baseline: 
    if (m_ucPrecision != 8)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters",
                "image precision for baseline scan must be 8");
    break;
  case Sequential:
  case Progressive:
  case DifferentialSequential:
  case DifferentialProgressive:
  case ACSequential:
  case ACProgressive:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
    //
    if (m_ucPrecision != 8 && m_ucPrecision != 12)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters",
                "image precision must be 8 or 12");
    break;
  case Residual:
  case ACResidual:
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
    // Disable subsampling if lossless
    if (TablesOf()->isLossless()) {
      psubx = NULL;
      psuby = NULL;
    }
    if (m_ucPrecision < 2 || m_ucPrecision > 17)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters",
                "image precision for residual coding must be between 2 and 17");
    break;
  default: // lossless, residual.
    if (m_ucPrecision < 2 || m_ucPrecision > 16)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters","image precision in lossless mode must be between 2 and 16");
    break;
  }
  m_bWriteDNL   = writednl;
  //
  //
  // Define the components. This call here does not support subsampling.
  for(i = 0;i < m_ucDepth;i++) {
    // Get subsampling parameters
    UBYTE sx = (psubx)?(*psubx++):(1);
    UBYTE sy = (psuby)?(*psuby++):(1);
    //
    // End of the array - fall back to one.
    if (sx == 0) {
      sx = 1;psubx = NULL;
    }
    if (sy == 0) {
      sy = 1;psuby = NULL;
    }
    //
    class Component *comp = DefineComponent(i,sx,sy);
    comp->SetComponentID(i);        // simple 1-1 mapping.
    comp->SetQuantizer((colortrafo == false || i == 0)?0:1); // one lume and one chroma quantizer
  }

  ComputeMCUSizes();

  assert(m_pScan == NULL);

  // If this is only the DHP marker segment, do not create a scan.
  if (m_Type == Dimensions)
    return NULL;

  switch(m_Type) {
  case Progressive:
  case ACProgressive:
  case DifferentialProgressive:
  case ACDifferentialProgressive:
    if (m_ucDepth > 4)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultParameters",
                "progressive mode allows only up to four components");
    //
    while(tags && (tags = tags->FindTagItem(JPGTAG_IMAGE_SCAN + tagoffset))) {
      const struct JPG_TagItem *scantags = (struct JPG_TagItem *)(tags->ti_Data.ti_pPtr);
      if (scantags) {
        LONG sstart  = scantags->GetTagData(JPGTAG_SCAN_SPECTRUM_START);
        sstart = scantags->GetTagData(JPGTAG_SCAN_SPECTRUM_START + tagoffset,sstart);
        //
        // If there are AC frequencies included, break up the scans.
        breakup = (sstart > 0)?true:false;
        CreateProgressiveScanParameters(breakup,tagoffset,tags,scantags);
      }
      tags = tags->NextTagItem();
    }
    break;
  case Residual:
  case ACResidual:
  case ResidualDCT:
  case ACResidualDCT:
    // Create a residual scan?
    if (m_pTables->UseResiduals()) {
      // Always create separate scans here. Not actually required, but performs better.
      CreateSequentialScanParameters(true,tagoffset,tags);
    }
    break;
  case ResidualProgressive:
  case ACResidualProgressive:
    //
    if (m_pTables->UseResiduals()) { 
      if (m_ucDepth > 4)
        JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultParameters",
                  "progressive mode allows only up to four components");
      //
      while(tags && (tags = tags->FindTagItem(JPGTAG_IMAGE_SCAN + tagoffset))) {
        const struct JPG_TagItem *scantags = (struct JPG_TagItem *)(tags->ti_Data.ti_pPtr);
        if (scantags) {
          // Must always break up the scan.
          CreateProgressiveScanParameters(true,tagoffset,tags,scantags);
        }
        tags = tags->NextTagItem();
      }
    }
    break;
  default:
    // Create a regular scan
    breakup = false;
    if (m_Type == JPEG_LS) {
      if (tags->GetTagData(JPGTAG_SCAN_LS_INTERLEAVING,JPGFLAG_SCAN_LS_INTERLEAVING_NONE) == 
          JPGFLAG_SCAN_LS_INTERLEAVING_NONE)
        breakup = true;
    }
    CreateSequentialScanParameters(breakup,tagoffset,tags);
  }
  //
  // Create refinements for this scan?
  if (m_pTables->UseRefinements()) {
    switch(m_Type) {
    case Lossless:
    case ACLossless:
    case JPEG_LS:
      JPG_THROW(INVALID_PARAMETER,"Frame::InstallDefaultScanParameters",
                "the lossless scans do not support hidden refinement scans");
      break;
    case DifferentialSequential:
    case DifferentialProgressive:
    case DifferentialLossless:
    case ACDifferentialSequential:
    case ACDifferentialProgressive:
    case ACDifferentialLossless:
      // Hmm. At this time, simply disallow. There is probably a way how to fit this into
      // the highest hierarchical level, but not now.
      JPG_THROW(NOT_IMPLEMENTED,"Frame::InstallDefaultScanParameters",
                "the hierarchical mode does not yet allow hidden refinement coding");
      break;
    case Residual:
    case ACResidual:
    case ResidualProgressive:
    case ACResidualProgressive:
      // Create hidden refinement scans for residual scans.
      {
        UBYTE hiddenbits;
        UBYTE totalhidden = m_pTables->HiddenDCTBitsOf();
        UBYTE component;
        class Scan *scan;
        for(hiddenbits = 0;hiddenbits < totalhidden;hiddenbits++) {
          component = m_ucDepth;
          do {
            component--;
            scan = new(m_pEnviron) class Scan(this);
            scan->TagOn(m_pScan);
            m_pScan = scan; // The AC part
            scan->MakeHiddenRefinementScan(hiddenbits,ComponentOf(component),0,63);
          } while(component);
        }
      }
      break;
    default:
      // Create hidden refinement scans for regular scans: Separate DC scans are required here.
      {
        UBYTE hiddenbits;
        UBYTE totalhidden = m_pTables->HiddenDCTBitsOf();
        UBYTE component;
        class Scan *scan;
        for(hiddenbits = 0;hiddenbits < totalhidden;hiddenbits++) {
          component = m_ucDepth;
          do {
            component--;
            scan = new(m_pEnviron) class Scan(this);
            scan->TagOn(m_pScan);
            m_pScan = scan; // The AC part
            scan->MakeHiddenRefinementScan(hiddenbits,ComponentOf(component),1,63);
          } while(component);
          scan = new(m_pEnviron) class Scan(this);
          scan->TagOn(m_pScan);
          m_pScan = scan; // The DC part
          scan->MakeHiddenRefinementScan(hiddenbits,NULL,0,0);
        }
      }
      break;
    }
  }
  m_pCurrent = m_pScan;
  return m_pScan;
}
///

/// Frame::AttachScan
// Attach a new scan to the frame, return the scan
// and make this the current scan.
class Scan *Frame::AttachScan(void)
{
  class Scan *scan = new(m_pEnviron) class Scan(this);

  if (m_pScan == NULL) {
    assert(m_pLast == NULL);
    m_pScan = scan;
  } else {
    assert(m_pLast != NULL);
    m_pLast->TagOn(scan);
  }
  m_pLast          = scan;
  m_pCurrent       = scan; 
  m_bStartedTables = false;
  
  return scan;
}
///

/// Frame::StartParseScan
class Scan *Frame::StartParseScan(class ByteStream *io,class Checksum *chk)
{
  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartParseScan",
              "frame is currently not available for parsing");
  //
  // Not yet reached the EOF.
  m_bEndOfFrame = false;
  //
  // If there is a residual marker, and this is the final scan,
  // build it now.
  if (m_bBuildRefinement && !m_bCreatedRefinement) {
    class DataBox *box = m_pTables->RefinementDataOf(m_usRefinementCount++);
    assert(box);
    class ByteStream *stream = box->DecoderBufferOf();
    //
    // De-activate unless re-activated on the next scan/trailer.
    // The refinement scans are not checksummed.
    m_pTables->ParseTables(stream,NULL,false,(m_Type == JPEG_LS)?true:false);
    m_bBuildRefinement = false;
    if (ScanForScanHeader(stream)) {
      class Scan *scan = AttachScan();
      scan->StartParseHiddenRefinementScan(stream,m_pImage);
      return scan;
    }
  } else {
    // Regular scan.
    if (m_bStartedTables) {
      if (m_pTables->ParseTablesIncremental(io,chk,false,(m_Type == JPEG_LS)?true:false)) {
        // Re-iterate the scan header parsing, not yet done.
        return NULL;
      }
    } else {
      // Indicate that we currently do not yet have a scan, neither an EOF.
      m_pTables->ParseTablesIncrementalInit(false);
      m_bStartedTables = true;
      return NULL;
    }
    //
    // The checksum could also come here, i.e. in the scan header.
    chk = m_pParent->CreateChecksumWhenNeeded(chk);
    // 
    // Everything else is checksummed.
    if (chk && m_pTables->ChecksumTables()) {
      assert(m_pAdapter == NULL);
      //
      // The scan requires a valid IO stream that stays over this
      // call.
      m_pAdapter = new(m_pEnviron) class ChecksumAdapter(io,chk,false);
      if (ScanForScanHeader(m_pAdapter)) {
        class Scan *scan = AttachScan();
        scan->ParseMarker(m_pAdapter);
        scan->StartParseScan(m_pAdapter,chk,m_pImage);
        return scan;
      }
    } else {
      if (ScanForScanHeader(io)) {
        class Scan *scan = AttachScan();
        scan->ParseMarker(io);
        scan->StartParseScan(io,chk,m_pImage);
        return scan;
      }
    }
  }

  m_bEndOfFrame    = true;
  m_bStartedTables = false;
  return NULL;
}
///

/// Frame::ScanForScanHeader
// Start parsing a hidden scan, let it be either the residual or the refinement
// scan.
bool Frame::ScanForScanHeader(class ByteStream *stream)
{ 
  LONG data;

  //
  data = stream->GetWord();
  if (data != 0xffda) {
    JPG_WARN(MALFORMED_STREAM,"Frame::StartParseHiddenScan","Start of Scan SOS marker missing");
    // Advance to the next marker if there is something next.
    if (data == ByteStream::EOF)
      return false; // Nope, nothing next.
    //
    do {
      stream->LastUnDo();
      do {
        data = stream->Get();
      } while(data != 0xff && data != ByteStream::EOF);
      //
      if (data == ByteStream::EOF)
        break; // Try from the main stream
      stream->LastUnDo();
      //
      // If this is SOS, we recovered. Maybe.
      data = stream->GetWord();
      if (data == ByteStream::EOF)
        break;
      // Check for the proper marker.
    } while(data != 0xffda);
  }

  return (data == 0xffda)?(true):(false);
}
///

/// Frame::StartWriteScan
// Start writing a single scan. Scan parameters must have been installed before.
class Scan *Frame::StartWriteScan(class ByteStream *io,class Checksum *chk)
{
  if (m_pCurrent == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartWriteScan",
              "scan parameters have not been defined yet"); 
  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartWriteScan",
              "frame is currently not available for measurements");
  //
  // Create a compatible image buffer and put it into BitmapCtrl,
  // or re-use it.
  //m_pTables->WriteTables(io);

  // If this scan is not hidden, write the previously collected
  // refinement data out. Scan creation puts all the hidden scans first.
  if (!m_pCurrent->isHidden()) {
    assert(m_pCurrentRefinement == NULL);
    if (m_pTables->ChecksumTables()) {
      // The checksum is computed toplevel.
      m_pCurrent->StartWriteScan(io,NULL,m_pImage);
    } else {
      m_pCurrent->StartWriteScan(io,chk,m_pImage);
    }
  } else {
    // Write into the refinement box.
    class DataBox *box = m_pTables->AppendRefinementData();
    assert(box); 
    assert(m_pCurrentRefinement == NULL);
    m_pCurrentRefinement   = box;
    m_pCurrent->StartWriteScan(box->EncoderBufferOf(),NULL,m_pImage);
  }

  return m_pCurrent;
}
///

/// Frame::StartMeasureScan
// Start a measurement scan
class Scan *Frame::StartMeasureScan(void)
{
  if (m_pCurrent == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartMeasureScan",
              "scan parameters have not been defined yet");
  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartMeasureScan",
              "frame is currently not available for measurements");
  // 
  // Create a compatible image buffer and put it into BitmapCtrl,
  // or re-use it.
  m_pCurrent->StartMeasureScan(m_pImage);

  return m_pCurrent;
}
///

/// Frame::EndParseScan
// End parsing the current scan.
void Frame::EndParseScan(void)
{
  assert(m_pCurrent);
  //
  if (m_pAdapter && m_pTables->ChecksumTables() == false) {
    //
    // Compute the checksum so far and let the thing go.
    m_pAdapter->Close(); 
    delete m_pAdapter;
    m_pAdapter = NULL;
  }
}
///

/// Frame::EndWriteScan
// End writing the current scan
void Frame::EndWriteScan(void)
{
  assert(m_pCurrent);
  m_pCurrent->Flush();
  if (m_pAdapter && m_pTables->ChecksumTables() == false) {
    //
    // Compute the checksum so far and let the thing go.
    m_pAdapter->Close(); 
    delete m_pAdapter;
    m_pAdapter = NULL;
  }
}
///

/// Frame::NextScan
class Scan *Frame::NextScan(void)
{
  if (m_pCurrent == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::NextScan","no scan iteration has been started, cannot advance the scan");

  return m_pCurrent = m_pCurrent->NextOf();
}
///

/// Frame::WriteFrameType
// Write the marker that identifies this type of frame, and all the scans within it.
void Frame::WriteFrameType(class ByteStream *io) const
{
  if (m_pScan == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::WriteFrameType",
              "frame parameters have not yet been installed, cannot write frame type");

  m_pScan->WriteFrameType(io);
}
///

/// Frame::ParseTrailer
// Parse off the EOI marker at the end of the image. Return false
// if there are no more scans in the file, true otherwise.
bool Frame::ParseTrailer(class ByteStream *io)
{
  // The frame itself has not trailer (only the image has).
  // Anyhow, there is no data to be checksummed here, so conclude with
  // the checksum.
  delete m_pAdapter;m_pAdapter = NULL;
  //
  do {
    LONG marker = io->PeekWord();
    class DataBox *box;
    
    switch(marker) {
    case 0xffb1: // residual sequential
    case 0xffb2: // residual progressive.
    case 0xffb3:
    case 0xffb9:
    case 0xffba:
    case 0xffbb:
    case 0xffc0:
    case 0xffc1:
    case 0xffc2:
    case 0xffc3:
    case 0xffc9:
    case 0xffca:
    case 0xffcb:
    case 0xfff7: // JPEG LS SOF55
      // All non-differential frames, may not appear in a hierarchical process.
      JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
               "found a non-differential frame start behind the initial frame");
      return false;
    case 0xffde: // DHP, should not go here.
      JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
               "found a double DHP marker behind a frame start");
      return false;
    case 0xffc5:
    case 0xffc6:
    case 0xffc7:
    case 0xffcd:
    case 0xffce:
    case 0xffcf:
      // All differential types, may only appear in a differential frame.
      if (!m_pParent->isHierarchical())
        JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
                 "found a differential frame start outside a hierarchical process");
      return false;
    case 0xffda: // This is an SOS marker, i.e. the frame does not end here.
      return true;
    case 0xffd9: // The EOI still needs to be seen by the image.
      // Once we run into the EOI, check for the refinement scans.
      if ((box = m_pTables->RefinementDataOf(m_usRefinementCount)) && !m_bCreatedRefinement) {
        assert(m_pImage);
        // This must be the start of a new scan. No filler bytes allowed here.
        m_bBuildRefinement = true;
        return true;
      } 
      // No refinement scans, or refinement scans done:
      // We're done here.
      return false;
    case 0xffff:
      // A filler byte. Remove the filler, try again.
      io->Get();
      break;
    case 0xffd0:
    case 0xffd1:
    case 0xffd2:
    case 0xffd3:
    case 0xffd4:
    case 0xffd5:
    case 0xffd6:
    case 0xffd7: // Restart markers.
      io->GetWord();
      JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer","found a stray restart marker segment, ignoring");
      break;
    case ByteStream::EOF:
      JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
               "missing an EOI marker at the end of the stream");
      return false;
    default:
      if (marker < 0xff00) {
        JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
                 "expecting a marker or marker segment - stream is out of sync");
        // Advance to the next marker and see how it goes from there...
        io->Get(); // Remove the invalid thing.
        do {
          marker = io->Get();
        } while(marker != 0xff && marker != ByteStream::EOF);
        //
        if (marker == ByteStream::EOF) {
          JPG_WARN(UNEXPECTED_EOF,"Frame::ParseTrailer",
                   "run into an EOF while scanning for the next marker");
          return false;
        }
        io->LastUnDo();
        // Continue parsing, check what the next marker might be.
      } else {
        // Something that looks like a valid marker. This could be the
        // tables/misc section of the next frame or next scan, depending
        // on whether we are hierarchical (next frame) or progressive (next scan).
        // Unfortunately, what is what we only know after having received either
        // the SOS marker (next scan) or an SOF marker (next frame).
        // Thus, at this time, parse of the tables, place its data in the
        // global table namespace, overriding what was there, then
        // continue parsing here until we know what we have.
        assert(m_pTables);
        // This might include EXP if we are hierarchical.
        m_pTables->ParseTables(io,NULL,m_pParent->isHierarchical(),(m_Type == JPEG_LS)?true:false);
      }
    }
  } while(true);
  
  return true; // code never goes here.
}
///

/// Frame::BuildLineAdapter
// Build the line adapter fitting to the frame type.
class LineAdapter *Frame::BuildLineAdapter(void)
{
  switch(m_Type) {
  case Baseline: // also sequential
  case Sequential:
  case Progressive:  
  case DifferentialSequential:
  case DifferentialProgressive:
  case ACSequential:
  case ACProgressive: 
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case Residual:
  case ACResidual:
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
    return new(m_pEnviron) class BlockLineAdapter(this); // all block based.
  case Lossless:
  case ACLossless:
  case DifferentialLossless:
  case ACDifferentialLossless:
  case JPEG_LS:
    return new(m_pEnviron) class LineLineAdapter(this); // all line based.
  case Dimensions:
    break;
  }
  JPG_THROW(INVALID_PARAMETER,"Frame::BuildLineAdapter","found illegal or unsupported frame type");
  return NULL;
}
///

/// Frame::ExtendImageBuffer
// Extend the image by a merging process, and install it
// here.
void Frame::ExtendImageBuffer(class BufferCtrl *img,class Frame *residual)
{
  switch(m_Type) {
  case Baseline: // also sequential
  case Sequential:
  case Progressive:  
  case ACSequential:
  case ACProgressive: 
    if (m_pBlockHelper == NULL) {
      class BlockBitmapRequester *bb = dynamic_cast<class BlockBitmapRequester *>(img);
      if (bb && m_pTables->ResidualDataOf()) {
        m_pBlockHelper  = new(m_pEnviron) class ResidualBlockHelper(this,residual);
        bb->SetBlockHelper(m_pBlockHelper);
      }
    }
    break;
  case Lossless:
  case ACLossless:
  case JPEG_LS:
    JPG_THROW(MALFORMED_STREAM,"Frame::ExtendImage",
              "Lossless codestreams cannot be extended by a residual stream");
    break;
  case Residual:
  case ACResidual:  
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
    JPG_THROW(MALFORMED_STREAM,"Frame::ExtendImage",
              "Residual scans cannot be extended by residuals itself");
    break;
  default: 
    JPG_THROW(MALFORMED_STREAM,"Frame::ExtendImage",
              "Hierarchical codestreams cannot be extended a residual stream");
    break;
  }
}
///

/// Frame::BuildImageBuffer
// Build the image buffer type fitting to the frame type.
class BitmapCtrl *Frame::BuildImageBuffer(void)
{
  switch(m_Type) {
  case Baseline: // also sequential
  case Sequential:
  case Progressive:  
  case ACSequential:
  case ACProgressive: 
    return new(m_pEnviron) class BlockBitmapRequester(this); 
  case Lossless:
  case ACLossless:
  case DifferentialLossless:
  case ACDifferentialLossless:
  case JPEG_LS:
    return new(m_pEnviron) class LineBitmapRequester(this);
  case Dimensions:
    return new(m_pEnviron) class HierarchicalBitmapRequester(this);
  case Residual:
  case ACResidual:
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
    return NULL; // No image required.
  default:
    break; // Everything else is part of a hierarchical scan and does not have a full image buffer by itself.
  }
  JPG_THROW(MALFORMED_STREAM,"Frame::BuildLineAdapter","found illegal or unsupported frame type");
  return NULL;
}
///

/// Frame::PostImageHeight
// Define the image size if it is not yet known here. This is
// called whenever the DNL marker is parsed in.
void Frame::PostImageHeight(ULONG height)
{
  assert(height > 0 && m_pImage);
  
  if (m_ulHeight == 0) {
    m_ulHeight = height;
    m_pImage->PostImageHeight(height);
  } else if (m_ulHeight == height) {
    JPG_WARN(MALFORMED_STREAM,"Frame::PostImageHeight",
             "found a double DNL marker for a frame, frame size is known already");
  } else {
    JPG_THROW(MALFORMED_STREAM,"Frame::PostImageHeight",
              "found a double DNL marker for a frame, indicating an inconsistent frame height");
  }
}
///

/// Frame::OptimizeDCTBlock
// Optimize a single DCT block through all scans of this frame for
// ideal R/D performance.
void Frame::OptimizeDCTBlock(LONG bx,LONG by,UBYTE compidx,class DCT *dct,LONG block[64])
{
  class Scan *scan;
  DOUBLE lambda   = dct->EstimateCriticalSlope();

  for(scan = m_pScan;scan;scan = scan->NextOf()) {
    scan->OptimizeDCTBlock(bx,by,compidx,lambda,dct,block);
  }
}
///

/// Frame::StartOptimizeScan
// Start an optimization scan for the R/D optimizer.
class Scan *Frame::StartOptimizeScan(void)
{ 
  if (m_pCurrent == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartOptimizeScan",
              "scan parameters have not been defined yet");
  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartOptimizeScan",
              "frame is currently not available for optimization");
  // 
  // Create a compatible image buffer and put it into BitmapCtrl,
  // or re-use it.
  m_pCurrent->StartOptimizeScan(m_pImage);

  return m_pCurrent;
}
///

/// Frame::isDCTBased
// Return an indicator whether this is a DCT-based frame type.
bool Frame::isDCTBased(void) const
{
  switch(m_Type) {
  case Lossless:
  case ACLossless:
  case JPEG_LS:
    return false;
  case DifferentialLossless:
  case ACDifferentialLossless:
    // This is a bit touchy. We are in a hiearchical process, hence the
    // DCT mode (and for that the preshift) is determined by the first
    // frame of the hierarchical process.
    return (m_pParent->FirstFrameOf()->isDCTBased());
  default:
    return true;
  }
}
///
