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
** This class represents a single frame and the frame dimensions.
**
** $Id: frame.cpp,v 1.82 2012-09-23 14:10:12 thor Exp $
**
*/

/// Includes
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "io/bytestream.hpp"
#include "std/string.hpp"
#include "codestream/tables.hpp"
#include "marker/scan.hpp"
#include "marker/residualmarker.hpp"
#include "marker/residualspecsmarker.hpp"
#include "control/bitmapctrl.hpp"
#include "control/lineadapter.hpp"
#include "control/blockbitmaprequester.hpp"
#include "control/linebitmaprequester.hpp"
#include "control/hierarchicalbitmaprequester.hpp"
#include "control/blocklineadapter.hpp"
#include "control/linelineadapter.hpp"
#include "control/residualblockhelper.hpp"
///

/// Frame::Frame
// Construct a frame object
Frame::Frame(class Tables *tables,ScanType t)
  : JKeeper(tables->EnvironOf()), m_pNext(NULL), m_pTables(tables), 
    m_pScan(NULL), m_pLast(NULL), m_pCurrent(NULL), m_pImage(NULL),
    m_pBlockHelper(NULL), m_Type(t), m_ucPrecision(0), m_ucDepth(0), m_ppComponent(NULL),
    m_bWriteDNL(false), 
    m_bBuildResidual(false), m_bCreatedResidual(false),
    m_bBuildRefinement(false), m_bCreatedRefinement(false)
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

  delete m_pBlockHelper;
}
///

/// Frame::ParseMarker
void Frame::ParseMarker(class ByteStream *io)
{
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
  // then JPEG cannot support this subsampling setting. Wierd.
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
  if (m_bWriteDNL) {
    io->PutWord(0xffdc); // DNL marker
    io->PutWord(4);      // its size
    io->PutWord(m_ulHeight); // the height
    m_bWriteDNL = false;
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
// limits of JPEG.
UBYTE Frame::PointPreShiftOf(void) const
{
  if (m_pTables) {
    class ResidualSpecsMarker *resmarker;
    
    if ((resmarker  = m_pTables->ResidualSpecsOf())) {
      return resmarker->PointPreShiftOf();
    }
  }
  return 0;
}
///

/// Frame::InstallDefaultParameters
// Define default scan parameters. Returns the scan for further refinement if required.
class Scan *Frame::InstallDefaultParameters(ULONG width,ULONG height,UBYTE depth,UBYTE prec,
					    bool writednl,const UBYTE *psubx,const UBYTE *psuby,
					    const struct JPG_TagItem *tags)
{
  int i;
  
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
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters","image precision must be 8 or 12");
    break;
  default: // lossless
    if (m_ucPrecision < 2 || m_ucPrecision > 16)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultScanParameters","image precision must be between 2 and 16");
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
    if (m_pTables->UseColortrafo()) {
      comp->SetQuantizer(i == 0?0:1); // one lume and one chroma quantizer
    } else {
      comp->SetQuantizer(0);          // only one quantizer
    }
  }

  ComputeMCUSizes();

  assert(m_pScan == NULL);

  // If this is only the DHP marker segment, do not create a scan.
  if (m_Type == Dimensions)
    return NULL;
  
  if (m_Type == Progressive             || 
      m_Type == ACProgressive           ||
      m_Type == DifferentialProgressive || 
      m_Type == ACDifferentialProgressive) {
    if (m_ucDepth > 4)
      JPG_THROW(OVERFLOW_PARAMETER,"Frame::InstallDefaultParameters",
		"progressive mode allows only up to four components");
    //
    while(tags && (tags = tags->FindTagItem(JPGTAG_IMAGE_SCAN))) {
      const struct JPG_TagItem *scantags = (struct JPG_TagItem *)(tags->ti_Data.ti_pPtr);
      if (scantags) {
	if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENTS_CHROMA)) {
	  // This actually creates a group of tags if the spectral selection contains
	  // AC bands.
	  if (m_ucDepth > 1) {
	    if (scantags->GetTagData(JPGTAG_SCAN_SPECTRUM_START) > 0) {
	      UBYTE i;
	      struct JPG_TagItem ctags[] = {
		JPG_ValueTag(JPGTAG_SCAN_COMPONENT0,0),
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
		scan->InstallDefaults(1,ctags);
	      }
	    } else {
	      struct JPG_TagItem ctags[] = {
		JPG_ValueTag((m_ucDepth > 1)?JPGTAG_SCAN_COMPONENT0:JPGTAG_TAG_IGNORE,1),
		JPG_ValueTag((m_ucDepth > 2)?JPGTAG_SCAN_COMPONENT1:JPGTAG_TAG_IGNORE,2),
		JPG_ValueTag((m_ucDepth > 3)?JPGTAG_SCAN_COMPONENT2:JPGTAG_TAG_IGNORE,3),
		JPG_Continue(scantags)
	      };
	      class Scan *scan = new(m_pEnviron) class Scan(this);
	      if (m_pScan == NULL) {
		m_pScan = scan;
	      } else {
		m_pLast->TagOn(scan);
	      }
	      m_pLast = scan;
	      scan->InstallDefaults(m_ucDepth - 1,ctags);
	    }
	  } // Nothing to do if chroma channels are not present.
	} else { 
	  UBYTE depth = m_ucDepth;
	  if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT0)) depth = 1;
	  if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT1)) depth = 2;
	  if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT2)) depth = 3;
	  if (scantags->FindTagItem(JPGTAG_SCAN_COMPONENT3)) depth = 4;
	  //
	  // If this is an AC scan, and there is more than one component, separate
	  // into several scans.
	  if (depth > 1 && scantags->GetTagData(JPGTAG_SCAN_SPECTRUM_START) > 0) {
	    UBYTE i;
	    struct JPG_TagItem ctags[] = {
	      JPG_ValueTag(JPGTAG_SCAN_COMPONENT0,0),
	      JPG_ValueTag(JPGTAG_SCAN_COMPONENT1,0),
	      JPG_ValueTag(JPGTAG_SCAN_COMPONENT2,0),
	      JPG_ValueTag(JPGTAG_SCAN_COMPONENT3,0),
	      JPG_Continue(scantags)
	    };
	    for(i = 0;i < depth;i++) {
	      const struct JPG_TagItem *comp = scantags->FindTagItem(JPGTAG_SCAN_COMPONENT0 + i);
	      ctags[0].ti_Data.ti_lData = (comp)?(comp->ti_Data.ti_lData):(i);
	      class Scan *scan = new(m_pEnviron) class Scan(this);
	      if (m_pScan == NULL) {
		m_pScan = scan;
	      } else {
		m_pLast->TagOn(scan);
	      }
	      m_pLast = scan;
	      scan->InstallDefaults(1,ctags);
	    }
	  } else {
	    class Scan *scan = new(m_pEnviron) class Scan(this);
	    if (m_pScan == NULL) {
	      m_pScan = scan;
	    } else {
	      m_pLast->TagOn(scan);
	    }
	    m_pLast = scan;
	    scan->InstallDefaults(depth,scantags);
	  }
	}
      }
      tags = tags->NextTagItem();
    }
  } else {
    UBYTE maxdepth    = 4;
    if (m_Type == JPEG_LS) {
      if (tags->GetTagData(JPGTAG_SCAN_LS_INTERLEAVING,JPGFLAG_SCAN_LS_INTERLEAVING_NONE) == 
	  JPGFLAG_SCAN_LS_INTERLEAVING_NONE)
	maxdepth = 1;
    }
    UBYTE depth       = m_ucDepth;
    UBYTE comp        = 0;
    //
    // Create multiple scans for more than maxdepth components.
    while(depth) {
      class Scan *scan;
      UBYTE curdepth = (depth > maxdepth)?(maxdepth):(depth);
      struct JPG_TagItem ctags[] = {
	JPG_ValueTag((curdepth > 0)?JPGTAG_SCAN_COMPONENT0:JPGTAG_TAG_IGNORE,comp + 0),
	JPG_ValueTag((curdepth > 1)?JPGTAG_SCAN_COMPONENT1:JPGTAG_TAG_IGNORE,comp + 1),
	JPG_ValueTag((curdepth > 2)?JPGTAG_SCAN_COMPONENT2:JPGTAG_TAG_IGNORE,comp + 2),
	JPG_ValueTag((curdepth > 3)?JPGTAG_SCAN_COMPONENT3:JPGTAG_TAG_IGNORE,comp + 3),
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
      scan->InstallDefaults(curdepth,ctags);
      comp += maxdepth;
      depth-= curdepth;
    }
  }
  //
  // Create a residual scan?
  if (m_pTables->UseResiduals()) {
    switch(m_Type) {
    case Lossless:
    case ACLossless:
    case JPEG_LS:
      JPG_THROW(INVALID_PARAMETER,"Frame::InstallDefaultScanParameters",
		"the lossless scans do not create residuals, no need to code them");
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
		"the hierarchical mode does not yet allow residual coding");
      break;
    default:
      // Make the first scan a residual scan.
      { 
	UBYTE component = m_ucDepth;
	class Scan *scan;
	do {
	  component--;
	  scan = new(m_pEnviron) class Scan(this);
	  scan->TagOn(m_pScan);
	  m_pScan = scan;
	  scan->MakeResidualScan(ComponentOf(component));
	} while(component);
      }
      break;
    }
  }
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
    default:
      // Create hidden refinement scans.
      {
	UBYTE hiddenbits;
	UBYTE component;
	class Scan *scan;
	for(hiddenbits = 0;hiddenbits < m_pTables->HiddenDCTBitsOf();hiddenbits++) {
	  component = m_ucDepth;
	  do {
	    component--;
	    scan = new(m_pEnviron) class Scan(this);
	    scan->TagOn(m_pScan);
	    m_pScan = scan;
	    scan->MakeHiddenRefinementACScan(hiddenbits,ComponentOf(component));
	  } while(component);
	  scan = new(m_pEnviron) class Scan(this);
	  scan->TagOn(m_pScan);
	  m_pScan = scan;
	  scan->MakeHiddenRefinementDCScan(hiddenbits);
	}
      }
      break;
    }
  }
  m_pCurrent = m_pScan;
  return m_pScan;
}
///
 
/// Frame::StartParseScan
class Scan *Frame::StartParseScan(class ByteStream *io)
{
  class Scan *scan;

  if (m_pImage == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Frame::StartParseScan",
	      "frame is currently not available for parsing");

  scan = new(m_pEnviron) class Scan(this);

  if (m_pScan == NULL) {
    assert(m_pLast == NULL);
    m_pScan = scan;
  } else {
    assert(m_pLast != NULL);
    m_pLast->TagOn(scan);
  }
  m_pLast    = scan;
  m_pCurrent = scan; 

  //
  // If there is a residual marker, and this is the final scan,
  // build it now.
  if (m_bBuildRefinement && !m_bCreatedRefinement) {
    assert(m_pTables->RefinementDataOf());
    class ByteStream *stream = m_pTables->RefinementDataOf()->StreamOf();
    //
    // De-activate unless re-activated on the next scan/trailer.
    m_pTables->ParseTables(stream);
    m_bBuildRefinement = false;
    if (ScanForScanHeader(stream)) {
      scan->StartParseHiddenRefinementScan(m_pImage);
      return scan;
    }
  } else if (m_bBuildResidual && !m_bCreatedResidual) {
    assert(m_pTables->ResidualDataOf());
    class ByteStream *stream = m_pTables->ResidualDataOf()->StreamOf();
    class BlockBitmapRequester *bb = dynamic_cast<class BlockBitmapRequester *>(m_pImage);
    assert(stream && bb);
    //
    // De-activate unless re-activated on the next scan/trailer.
    m_pTables->ParseTables(stream); 
    if (m_pBlockHelper == NULL)
      m_pBlockHelper  = new(m_pEnviron) class ResidualBlockHelper(this);
    bb->SetBlockHelper(m_pBlockHelper);

    m_bBuildResidual = false;
    if (ScanForScanHeader(stream)) {
      scan->StartParseResidualScan(m_pImage);
      return scan;
    }
  } else {
    // Regular scan.
    m_pTables->ParseTables(io);
    //
    if (ScanForScanHeader(io)) {
      scan->ParseMarker(io);
      scan->StartParseScan(io,m_pImage);
      return scan;
    }
  }

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
class Scan *Frame::StartWriteScan(class ByteStream *io)
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
  m_pCurrent->StartWriteScan(io,m_pImage);

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
  do {
    LONG marker = io->PeekWord();
    
    switch(marker) {
    case 0xffc0:
    case 0xffc1:
    case 0xffc2:
    case 0xffc3:
    case 0xffc9:
    case 0xffca:
    case 0xffcb:
    case 0xfff7:
      // All non-differential frames, may not appear in a hierarchical process.
      JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
	       "found a non-differential frame start behind the initial frame");
      return true;
    case 0xffd9: // The EOI still needs to be seen by the image.
    case 0xffc5:
    case 0xffc6:
    case 0xffc7:
    case 0xffcd:
    case 0xffce:  
    case 0xffcf:
    case 0xffdf: // EXP-marker also terminates this frame.
      // All differential frame starts: The frame ends here.
      // If we have a residual scan that is not yet parsed off,
      // create it now and parse it last so it sees the original
      // data. 
      if (m_pTables->RefinementDataOf() && !m_bCreatedRefinement) {
	assert(m_pImage);
	if (dynamic_cast<class BlockBitmapRequester *>(m_pImage)) {
	  LONG data;
	  do {
	    data = m_pTables->RefinementDataOf()->StreamOf()->PeekWord();
	    if (data == 0xffff) {
	      m_pTables->RefinementDataOf()->StreamOf()->Get(); // Filler byte
	    }
	  } while(data == 0xffff);
	  if (data != ByteStream::EOF && data != 0xffd9) {
	    m_bBuildRefinement = true;
	    //
	    // Come here again when the refinement is parsed off.
	    return true;
	  } else {
	    // Do not come again, refinement is there already.
	    m_bCreatedRefinement = true;
	  }
	}
      }
      if (m_pTables->ResidualDataOf() && !m_bCreatedResidual) {
	assert(m_pImage);
	if (dynamic_cast<class BlockBitmapRequester *>(m_pImage)) { 
	  LONG data;
	  do {
	    data = m_pTables->ResidualDataOf()->StreamOf()->PeekWord();
	    if (data == 0xffff) {
	      m_pTables->ResidualDataOf()->StreamOf()->Get(); // Filler byte
	    }
	  } while(data == 0xffff);
	  if (data != ByteStream::EOF && data != 0xffd9) {
	    m_bBuildResidual = true;
	    //
	    // Come here again when the residual is parsed off.
	    return true;
	  } else {
	    // Do not come again, refinement is there already.
	    m_bCreatedResidual = true;
	  }
	}
      }
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
	       "expecting an EOI marker at the end of the stream");
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
      } else if (marker < 0xffc0) {
	JPG_WARN(MALFORMED_STREAM,"Frame::ParseTrailer",
		 "detected an unknown marker - stream is out of sync");
	return true;
      } else {
	return true;
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

/// Frame::BuildImage
// Build the image buffer type fitting to the frame type.
class BitmapCtrl *Frame::BuildImage(void)
{
  switch(m_Type) {
  case Baseline: // also sequential
  case Sequential:
  case Progressive:  
  case ACSequential:
  case ACProgressive: 
    {
      if (m_pTables->ResidualDataOf()) {
	m_pBlockHelper  = new(m_pEnviron) class ResidualBlockHelper(this);
      }
      class BlockBitmapRequester *bb = new(m_pEnviron) class BlockBitmapRequester(this); 
      bb->SetBlockHelper(m_pBlockHelper);
      return bb;
    }
  case Lossless:
  case ACLossless:
  case DifferentialLossless:
  case ACDifferentialLossless:
  case JPEG_LS:
    return new(m_pEnviron) class LineBitmapRequester(this);
  case Dimensions:
    return new(m_pEnviron) class HierarchicalBitmapRequester(this);
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
