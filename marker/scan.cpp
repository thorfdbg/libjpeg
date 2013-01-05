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
** Represents all data in a single scan, and hence is the SOS marker.
**
** $Id: scan.cpp,v 1.79 2013-01-05 13:30:27 thor Exp $
**
*/

/// Includes
#include "marker/scan.hpp"
#include "marker/residualmarker.hpp"
#include "io/bytestream.hpp"
#include "marker/frame.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "codestream/entropyparser.hpp"
#include "codestream/sequentialscan.hpp"
#include "codestream/acsequentialscan.hpp"
#include "codestream/losslessscan.hpp"
#include "codestream/aclosslessscan.hpp"
#include "codestream/refinementscan.hpp"
#include "codestream/acrefinementscan.hpp"
#include "codestream/hiddenrefinementscan.hpp"
#include "codestream/singlecomponentlsscan.hpp"
#include "codestream/lineinterleavedlsscan.hpp"
#include "codestream/sampleinterleavedlsscan.hpp"
#include "codestream/vesascan.hpp"
#include "codestream/vesadctscan.hpp"
#include "coding/huffmantemplate.hpp"
#include "marker/huffmantable.hpp"
#include "marker/actable.hpp"
#include "marker/thresholds.hpp"
#include "control/bitmapctrl.hpp"
///

///

/// Scan::Scan
Scan::Scan(class Frame *frame)
  : JKeeper(frame->EnvironOf()), m_pNext(NULL), m_pFrame(frame), m_pParser(NULL),
    m_pHuffman(NULL), m_pConditioner(NULL)
{
  for(int i = 0;i < 4;i++) {
    m_pComponent[i]     = NULL;
    m_ucMappingTable[i] = 0;
  }
}
///

/// Scan::~Scan
Scan::~Scan(void)
{
  delete m_pParser;
  delete m_pHuffman;
  delete m_pConditioner;
}
///

/// Scan::WriteMarker
void Scan::WriteMarker(class ByteStream *io)
{ 
  bool jpegls = (m_pFrame->ScanTypeOf() == JPEG_LS);
  UWORD len   = m_ucCount * 2 + 6; // Size of the SOS marker
  int i;

  if (m_pHuffman) {
    io->PutWord(0xffc4); // DHT table
    m_pHuffman->WriteMarker(io);
  }

  if (m_pConditioner) {
    io->PutWord(0xffcc);
    m_pConditioner->WriteMarker(io);
  }
  
  io->PutWord(0xffda); // SOS marker

  // Size of the marker
  io->PutWord(len);

  // Number of components
  io->Put(m_ucCount);

  for(i = 0;i < m_ucCount;i++) {
    io->Put(m_ucComponent[i]);
    //
    // Write table selectors.
    assert(m_ucDCTable[i] < 16);
    assert(m_ucACTable[i] < 16);
    
    if (jpegls) {
      io->Put(m_ucMappingTable[i]);
    } else {
      io->Put((m_ucDCTable[i] << 4) | m_ucACTable[i]);
    }
  }
  
  io->Put(m_ucScanStart);
  io->Put(m_ucScanStop);
  
  assert(m_ucHighBit < 16);
  assert(m_ucLowBit  < 16);

  io->Put((m_ucHighBit << 4) | m_ucLowBit);
}
///


/// Scan::ParseMarker
// Parse the marker contents. The scan type comes from
// the frame type.
void Scan::ParseMarker(class ByteStream *io)
{
  // Just forward to the generic method.
  Scan::ParseMarker(io,m_pFrame->ScanTypeOf());
}
///

/// Scan::ParseMarker
// Parse the marker contents where the scan type
// comes from an additional parameter.
void Scan::ParseMarker(class ByteStream *io,ScanType type)
{
  LONG len = io->GetWord();
  LONG data;
  int i;

  if (len < 8)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","marker length of the SOS marker invalid, must be at least 8 bytes long");

  data = io->Get();
  if (data < 1 || data > 4)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","number of components in scan is invalid, must be between 1 and 4");

  m_ucCount = data;

  if (len != m_ucCount * 2 + 6)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","length of the SOS marker is invalid");

  for(i = 0;i < m_ucCount;i++) {
    data = io->Get(); // component identifier.
    if (data == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS marker run out of data");

    m_ucComponent[i] = data;
    
    data = io->Get(); // table selectors.
    if (data == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS marker run out of data");

    if (m_pFrame->ScanTypeOf() != JPEG_LS) {
      m_ucDCTable[i] = data >> 4;
      m_ucACTable[i] = data & 0x0f;
      
      if (m_ucDCTable[i] > 3)
	JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","DC table index in SOS marker is out of range, must be at most 4");
      
      if (m_ucACTable[i] > 3)
	JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","AC table index in SOS marker is out of range, must be at most 4");
    } else {
      m_ucMappingTable[i] = data; // JPEG_LS uses this for the mapping table selector.
    }
  }

  // Start of spectral selection or NEAR value.
  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS marker run out of data");
  if (data > 63 && m_pFrame->ScanTypeOf() != JPEG_LS)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","start of scan index is out of range, must be between 0 and 63");
  m_ucScanStart = data;
  
  //
  // End of spectral selection or interleave specifier.
  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS marker run out of data");
  if (m_pFrame->ScanTypeOf() != JPEG_LS) {
    if (data > 63)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","end of scan index is out of range, must be between 0 and 63");
  } else {
    if (data > 4 /*2*/)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","interleave specification is out of range, must be between 0 and 2"); 
  }
  m_ucScanStop = data;
  
  data = io->Get();
  if (data == ByteStream::EOF)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS marker run out of data");

  m_ucHighBit    = data >> 4;
  m_ucLowBit     = data & 0x0f;
  m_ucHiddenBits = m_pFrame->TablesOf()->HiddenDCTBitsOf();

  if (m_ucHighBit > 13)
    JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS high bit approximation is out of range, must be < 13");

  switch(type) {
  case Progressive:
  case ACProgressive:
  case DifferentialProgressive:
  case ACDifferentialProgressive:
    if (m_ucHighBit && m_ucHighBit != m_ucLowBit + 1)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"SOS high bit is invalid, successive approximation must refine by one bit per scan");
    if (m_ucScanStop < m_ucScanStart)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","end of scan is lower than start of scan");
    if (m_ucScanStart == 0 && m_ucScanStop != 0)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","DC component must be in a separate scan in the progressive mode");
    if (m_ucScanStart && m_ucCount != 1)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","AC scans in progressive mode must only contain a single component");
    break;
  case Residual:
  case ACResidual:
    if (m_ucHighBit && m_ucHighBit != m_ucLowBit + 1)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"SOS high bit is invalid, successive approximation must refine by one bit per scan");
    if (m_ucScanStop < m_ucScanStart)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","end of scan is lower than start of scan");
    break;
  case Baseline:
  case Sequential:
  case ACSequential:
  case DifferentialSequential:
  case ACDifferentialSequential:
    if (m_ucScanStop != 63 || m_ucScanStart != 0)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"scan start must be zero and scan stop must be 63 for the sequential operating modes");
    // runs into here.
  case JPEG_LS: 
    // Specs don't say anything what to do about them. Just assume they must be zero.
    if (m_ucHighBit != 0) // Low bit is the point transformation
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"successive approximation parameters must be zero for the sequential operating modes");
    break;
  case Lossless:
  case ACLossless:
    if (m_ucScanStart == 0 || m_ucScanStop > 7) // actually the predictor.
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"predictor for the lossless mode must be between 1 and 7");
    if (m_ucScanStop != 0)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"scan stop parameter must be zero in the lossless mode");
    if (m_ucHighBit != 0)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"successive approximation high bit parameter must be zero for the lossless mode");
    break;
  case DifferentialLossless:
  case ACDifferentialLossless:
    if (m_ucScanStart != 0) // actually the predictor.
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"predictor for the differential lossless mode must be zero");
    if (m_ucScanStop != 0)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"scan stop parameter must be zero in the lossless mode");
    if (m_ucHighBit != 0)
      JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
		"successive approximation high bit parameter must be zero for the lossless mode");
    break;
  default:
    break;
  }
}
///

/// Scan::ComponentOf
// Return the i'th component of the scan.
class Component *Scan::ComponentOf(UBYTE i)
{
  assert(i < 4);

  if (m_pComponent[i] == NULL)
    m_pComponent[i] = m_pFrame->FindComponent(m_ucComponent[i]);

  return m_pComponent[i];
}
///

/// Scan::CreateParser
// Create a suitable parser given the scan type as indicated in the
// header and the contents of the marker. The parser is kept
// here as it is local to the scan.
void Scan::CreateParser(void)
{
  ScanType type        = m_pFrame->ScanTypeOf();
  //
  assert(m_pParser == NULL);
  //
  switch(type) {
  case Baseline:
  case Sequential:
    m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
						     m_ucScanStart,m_ucScanStop,
						     m_ucLowBit + m_ucHiddenBits,0);
    break;
  case DifferentialSequential:
    m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
						     m_ucScanStart,m_ucScanStop,
						     m_ucLowBit + m_ucHiddenBits,0,true);
    break;
  case Lossless:
    m_pParser = new(m_pEnviron) class LosslessScan(m_pFrame,this,m_ucScanStart,
						   m_ucLowBit + m_ucHiddenBits);
    break;
  case DifferentialLossless:
    m_pParser = new(m_pEnviron) class LosslessScan(m_pFrame,this,0,
						   m_ucLowBit + m_ucHiddenBits,true);
    break;
  case ACLossless:
    m_pParser = new(m_pEnviron) class ACLosslessScan(m_pFrame,this,m_ucScanStart,
						     m_ucLowBit + m_ucHiddenBits);
    break;
  case ACDifferentialLossless:
    m_pParser = new(m_pEnviron) class ACLosslessScan(m_pFrame,this,0,
						     m_ucLowBit + m_ucHiddenBits,true);
    break;
  case ACSequential:
    m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit + m_ucHiddenBits,0);
    break;
  case ACDifferentialSequential:
    m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit + m_ucHiddenBits,0,true);
    break;
  case Progressive:
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit,0);
    } else { 
      m_pParser = new(m_pEnviron) class RefinementScan(m_pFrame,this,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit + m_ucHiddenBits,m_ucHighBit);
    }
    break;
  case DifferentialProgressive:
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit + m_ucHiddenBits,true);
    } else { 
      // Even though the specs do not mention this, it makes perfect sense that the
      // refinement scan is a regular refinement scan without modification.
      m_pParser = new(m_pEnviron) class RefinementScan(m_pFrame,this,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit,m_ucHighBit);
    }
    break;
  case ACProgressive: 
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
							 m_ucScanStart,m_ucScanStop,
							 m_ucLowBit + m_ucHiddenBits,0);
    } else { 
      m_pParser = new(m_pEnviron) class ACRefinementScan(m_pFrame,this,
							 m_ucScanStart,m_ucScanStop,
							 m_ucLowBit + m_ucHiddenBits,m_ucHighBit);
    }
    break;
  case JPEG_LS:
    // Depends on the interleaving
    switch(m_ucScanStop) {
    case 0:
      m_pParser = new(m_pEnviron) class SingleComponentLSScan(m_pFrame,this,
							      m_ucScanStart, // NEAR
							      m_ucMappingTable,
							      m_ucLowBit + m_ucHiddenBits); 
      // Point transformation
      break;
    case 1:
      m_pParser = new(m_pEnviron) class LineInterleavedLSScan(m_pFrame,this,
							      m_ucScanStart,
							      m_ucMappingTable,
							      m_ucLowBit + m_ucHiddenBits);
      break;
    case 2:
      m_pParser = new(m_pEnviron) class SampleInterleavedLSScan(m_pFrame,this,
								m_ucScanStart,
								m_ucMappingTable,
								m_ucLowBit + m_ucHiddenBits);
      break;
    case 3:
      m_pParser = new(m_pEnviron) class VesaScan(m_pFrame,this,
						 m_ucScanStart,
						 m_ucMappingTable,
						 m_ucLowBit + m_ucHiddenBits);
      break;
    case 4:
      m_pParser = new(m_pEnviron) class VesaDCTScan(m_pFrame,this,
						    m_ucScanStart,
						    m_ucMappingTable,
						    m_ucLowBit + m_ucHiddenBits);
      
      break; 
    }
    break;
  default:
    JPG_THROW(NOT_IMPLEMENTED,"Scan::CreateParser",
	      "sorry, the coding mode in the codestream is currently not supported");
  }
}
///

/// Scan::InstallDefaults
// Install the defaults for a sequential scan containing the given number of components
void Scan::InstallDefaults(UBYTE depth,const struct JPG_TagItem *tags)
{
  bool ishuffman    = false;
  bool ispredictive = false;
  bool isjpegls     = false;
  bool colortrafo   = (depth > 1)?(m_pFrame->TablesOf()->UseColortrafo()):(false);
  ScanType type     = m_pFrame->ScanTypeOf();

  assert(m_pParser == NULL);
  
  switch(type) {
  case Baseline:
  case Sequential:
  case Progressive:
  case DifferentialSequential:
  case DifferentialProgressive:
  case Residual:
    ishuffman    = true;
    break;
  case Lossless:
  case DifferentialLossless:
    ishuffman    = true;
    ispredictive = true;
    break;
  case ACSequential:
  case ACProgressive:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case ACResidual:
    break;
  case ACLossless:
  case ACDifferentialLossless:
    ispredictive = true;
    break;
  case JPEG_LS:
    ispredictive = true;
    isjpegls     = true;
    break;
  default:
    JPG_THROW(NOT_IMPLEMENTED,"Scan::InstallDefaults",
	      "sorry, unknown frame type, not yet implemented");
  }

  if (depth < 1 || depth > 4)
    JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
	      "JPEG allows only between one and four components per scan");

  m_ucCount = depth;
  
  for(UBYTE i = 0;i < depth;i++) {
    m_ucComponent[i] = i; // simply sequential

    if (/*ishuffman &&*/ colortrafo) {
      m_ucDCTable[i] = (i == 0)?(0):(1);
    } else {
      m_ucDCTable[i] = 0;
    }

    //
    // AC coding not required for predictive.
    if (/*ishuffman &&*/ !ispredictive && colortrafo) {
      m_ucACTable[i] = (i == 0)?(0):(1);
    } else {
      m_ucACTable[i] = 0;
    }
  } 

  if (isjpegls) {
    // None of the below required. 
  } else if (ishuffman) {
    m_pHuffman     = new(m_pEnviron) HuffmanTable(m_pEnviron);
  } else {
    m_pConditioner = new(m_pEnviron) ACTable(m_pEnviron);
  }
  
  switch(type) {
  case Progressive:
  case ACProgressive:
  case DifferentialProgressive:
  case ACDifferentialProgressive:
    m_ucScanStart = 0;
    m_ucScanStop  = 0; // DC only. User must create other scans manually.
    m_ucHighBit   = 0;
    m_ucLowBit    = 0; 
    break;
  case Baseline:
  case Sequential: 
  case ACSequential:
  case DifferentialSequential:
  case ACDifferentialSequential:
  case Residual:
  case ACResidual:
    // Install default start and stop of scan for a sequential run.
    m_ucScanStart = 0;
    m_ucScanStop  = 63;
    m_ucHighBit   = 0;
    m_ucLowBit    = 0; 
    break;
  case Lossless:
  case ACLossless:
    m_ucScanStart = 4; // predictor to use. This is the default.
    m_ucScanStop  = 0; // shall be zero
    m_ucHighBit   = 0; // shall be zero
    m_ucLowBit    = 0; // point transform.
    break;
  case DifferentialLossless:
  case ACDifferentialLossless:
    m_ucScanStart = 0; // no predictor at all.
    m_ucScanStop  = 0; // shall be zero
    m_ucHighBit   = 0; // shall be zero
    m_ucLowBit    = 0; // point transform.
    break;
  case JPEG_LS:
    m_ucScanStart = 0; // default is lossless
    m_ucScanStop  = 0; // not interleaved
    m_ucHighBit   = 0; // shall be zero
    m_ucLowBit    = 0; // point transform.
    break;
  default:
    assert(!"unimplemented scan type");
    break;
  }

  //
  // Get the tags.
  m_ucComponent[0] = tags->GetTagData(JPGTAG_SCAN_COMPONENT0,0);
  m_ucComponent[1] = tags->GetTagData(JPGTAG_SCAN_COMPONENT1,1);
  m_ucComponent[2] = tags->GetTagData(JPGTAG_SCAN_COMPONENT2,2);  
  m_ucComponent[3] = tags->GetTagData(JPGTAG_SCAN_COMPONENT3,3);
  m_ucHiddenBits   = m_pFrame->TablesOf()->HiddenDCTBitsOf();
  //
  // Install and check the scan parameters for the progressive scan.
  switch(type) {
  case Progressive:
  case ACProgressive:
  case DifferentialProgressive:
  case ACDifferentialProgressive: 
    m_ucScanStart    = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_START,m_ucScanStart);
    m_ucScanStop     = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_STOP ,m_ucScanStop);
    if (m_ucScanStart == 0 && m_ucScanStop)
      JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
		"DC coefficients must be in a separate scan in the progressive mode");
    if (m_ucScanStart && m_ucScanStop < m_ucScanStart)
      JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
		"Spectral selection stop must be larger or equal than spectral selection start");
    if (m_ucScanStart && m_ucCount > 1)
      JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
		"In the progressive mode, the AC components must be coded in all separate scans");
    if (m_ucScanStop >= 64)
      JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
		"Spectral selection stop is out of range, must be <= 63");

    m_ucHighBit      = tags->GetTagData(JPGTAG_SCAN_APPROXIMATION_HI,m_ucHighBit);
    m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_APPROXIMATION_LO,m_ucLowBit);
    if (m_ucHighBit > 0 && m_ucHighBit != m_ucLowBit + 1)
      JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
		"Successive approximation refinement must include only a single bitplane");
    break;
  case JPEG_LS:
    // This is the NEAR value of LS.
    m_ucScanStart = tags->GetTagData(JPGTAG_IMAGE_ERRORBOUND,0);
    switch(tags->GetTagData(JPGTAG_SCAN_LS_INTERLEAVING)) {
    case JPGFLAG_SCAN_LS_INTERLEAVING_NONE:
      m_ucScanStop = 0;
      break;
    case JPGFLAG_SCAN_LS_INTERLEAVING_LINE:
      m_ucScanStop = 1;
      break;
    case JPGFLAG_SCAN_LS_INTERLEAVING_SAMPLE:
      m_ucScanStop = 2;
      break;
    case JPGFLAG_SCAN_LS_VESASCAN:
      m_ucScanStop = 3;
      break;
    case JPGFLAG_SCAN_LS_VESADCTSCAN:
      m_ucScanStop = 4;
      break;
     default:
      JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
		"Invalid component interleaving mode for JPEG LS scans");
      break;
    }
    // Runs into the following to read the point transformation.
  case Lossless:
  case ACLossless:
  case DifferentialLossless:
  case ACDifferentialLossless:
    m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_POINTTRANSFORM,0);
    if (m_ucLowBit >= m_pFrame->PrecisionOf())
      JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
		"Point transformation removes more bits than available in the source data");
  default:
    break;
  }

  CompleteSettings();
}
///

/// Scan::MakeResidualScan
// Make this scan not a default scan, but a residual scan with the
// given maximal error.
void Scan::MakeResidualScan(class Component *comp)
{
  assert(m_pParser == NULL);

  m_ucCount        = 1;
  m_ucComponent[0] = comp->IDOf();
  m_ucScanStart    = 0;
  m_ucScanStop     = 63;
  m_ucHighBit      = m_ucLowBit   = 0;
  m_ucHiddenBits   = 0;

  switch(m_pFrame->ScanTypeOf()) {
  case Baseline:
  case Sequential:
  case Progressive:
  case Lossless:
    m_ucDCTable[0]   = m_ucACTable[0] = 0;
    m_pHuffman       = new(m_pEnviron) HuffmanTable(m_pEnviron);
    m_pParser        = new(m_pEnviron) ResidualHuffmanScan(m_pFrame,this,
							   m_pFrame->TablesOf()->ResidualDataOf(),
							   0,63,0,0,true,true);
    break;
  case ACSequential:
  case ACProgressive:
  case ACLossless:
    m_ucACTable[0]   = m_ucDCTable[0] = 0;
    m_pConditioner   = new(m_pEnviron) ACTable(m_pEnviron);
    m_pParser        = new(m_pEnviron) ResidualACScan(m_pFrame,this,
						      m_pFrame->TablesOf()->ResidualDataOf(),
						      0,63,0,0,true,true);
    break;
  case JPEG_LS:
    JPG_THROW(INVALID_PARAMETER,"Scan::MakeResidualACScan",
	      "JPEG LS does not support residual coding");
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Scan::MakeResidualScan",
	      "Hierarchical mode does not yet support residual coding");
    break;
  }
}
///

/// Scan::MakeHiddenRefinementACScan
// Make this scan a hidden refinement scan starting at the indicated
// bit position in the indicated component label.
void Scan::MakeHiddenRefinementACScan(UBYTE bitposition,class Component *comp)
{
  bool colortrafo   = (m_pFrame->DepthOf() > 1)?(m_pFrame->TablesOf()->UseColortrafo()):(false);
 
  assert(m_pParser == NULL);

  m_ucCount        = 1;
  m_ucComponent[0] = comp->IDOf();
  m_ucScanStart    = 1;
  m_ucScanStop     = 63;
  m_ucLowBit       = bitposition;
  m_ucHighBit      = bitposition+1;
  m_ucHiddenBits   = 0; // not here anymore.
  
  
  switch(m_pFrame->ScanTypeOf()) {
  case Baseline:
  case Sequential:
  case Progressive:
    if (colortrafo) {
      m_ucACTable[0] = (comp->IndexOf() == 0)?(0):(1); // Luma uses a separate table.
    } else {
      m_ucACTable[0] = 0;
    }
    m_ucDCTable[0] = m_ucACTable[0]; // Actually, not even used...
    m_pHuffman = new(m_pEnviron) HuffmanTable(m_pEnviron);
    m_pParser  = new(m_pEnviron) HiddenRefinementScan(m_pFrame,this,
						      m_pFrame->TablesOf()->RefinementDataOf(),
						      1,63,
						      bitposition,bitposition+1,
						      false,false);
    break;
  case ACSequential:
  case ACProgressive:
    m_ucACTable[0] = 0;
    m_ucDCTable[0] = 0;
    m_pConditioner = new(m_pEnviron) ACTable(m_pEnviron);
    m_pParser      = new(m_pEnviron) HiddenACRefinementScan(m_pFrame,this,
							    m_pFrame->TablesOf()->RefinementDataOf(),
							    1,63,
							    bitposition,bitposition+1,
							    false,false);
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Scan::MakeHiddenRefinementACScan",
	      "frame type does not support hidden refinement scans");
    break;
  }
}
///

/// Scan::MakeHiddenRefinementDCScan
// Make this scan a hidden refinement scan starting at the indicated
// bit position.
void Scan::MakeHiddenRefinementDCScan(UBYTE bitposition)
{
  UBYTE i;
  bool colortrafo   = (m_pFrame->DepthOf() > 1)?(m_pFrame->TablesOf()->UseColortrafo()):(false);
 
  assert(m_pParser == NULL);

  if (m_pFrame->DepthOf() > 4)
    JPG_THROW(INVALID_PARAMETER,"Scan::MakeHiddenRefinementDCScan",
	      "hidden refinement scans are confined to four components at most");

  m_ucCount        = m_pFrame->DepthOf();
  for(i = 0;i < m_ucCount;i++) {
    m_ucComponent[i] = m_pFrame->ComponentOf(i)->IDOf();
    m_ucDCTable[i]   = 0;
    m_ucACTable[i]   = 0; // Fixed later.
  }

  m_ucScanStart    = 0;
  m_ucScanStop     = 0; // This is a DC scan
  m_ucLowBit       = bitposition;
  m_ucHighBit      = bitposition+1;
  m_ucHiddenBits   = 0; // not here anymore.

  switch(m_pFrame->ScanTypeOf()) {
  case Baseline:
  case Sequential:
  case Progressive:
    if (colortrafo) {
      m_ucDCTable[1] = m_ucDCTable[2] = m_ucDCTable[3] = 1; // Chroma uses a separate table.
    }
    for(i = 0;i < m_ucCount;i++) {
      m_ucACTable[i] = m_ucDCTable[i];
    }
    m_pHuffman = new(m_pEnviron) HuffmanTable(m_pEnviron);
    m_pParser  = new(m_pEnviron) HiddenRefinementScan(m_pFrame,this,
						      m_pFrame->TablesOf()->RefinementDataOf(),
						      0,0,
						      bitposition,bitposition+1,
						      false,false);
    break;
  case ACSequential:
  case ACProgressive:
    m_pConditioner = new(m_pEnviron) ACTable(m_pEnviron);
    m_pParser      = new(m_pEnviron) HiddenACRefinementScan(m_pFrame,this,
							    m_pFrame->TablesOf()->RefinementDataOf(),
							    0,0,
							    bitposition,bitposition+1,
							    false,false);
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Scan::HiddenRefinementACScan",
	      "frame type does not support hidden refinement scans");
    break;
  }
}
///

/// Scan::StartParseHiddenRefinementScan
// Parse off a hidden refinement scan from the given position.
void Scan::StartParseHiddenRefinementScan(class BufferCtrl *ctrl)
{
  class ResidualMarker *marker = m_pFrame->TablesOf()->RefinementDataOf();
  class ByteStream *io         = marker->StreamOf();
  
  if (m_pParser == NULL) {
    ScanType type              = m_pFrame->ScanTypeOf();
    //
    switch(type) {
    case Baseline:
    case Sequential: 
    case Progressive:
      ParseMarker(io,Progressive);
      m_pParser = new(m_pEnviron) HiddenRefinementScan(m_pFrame,this,marker,
						       m_ucScanStart,m_ucScanStop,
						       m_ucLowBit,m_ucHighBit,
						       false,false);
      break;
    case ACSequential:
    case ACProgressive:
      ParseMarker(io,ACProgressive);
      m_pParser = new(m_pEnviron) HiddenACRefinementScan(m_pFrame,this,marker,
							 m_ucScanStart,m_ucScanStop,
							 m_ucLowBit,m_ucHighBit,
							 false,false);
      break; 
    default:
      JPG_THROW(NOT_IMPLEMENTED,"Scan::StartParseHiddenRefinementScan",
		"sorry, the coding mode in the codestream is currently not supported");
    }
  } 

  ctrl->PrepareForDecoding();
  m_pParser->StartParseScan(io,ctrl);
}
///

/// Scan::StartParseResidualScan
// Parse off a hidden refinement scan from the given position.
void Scan::StartParseResidualScan(class BufferCtrl *ctrl)
{
  class ResidualMarker *marker = m_pFrame->TablesOf()->ResidualDataOf();
  class ByteStream *io         = marker->StreamOf();

  if (m_pParser == NULL) {
    ScanType type               = m_pFrame->ScanTypeOf();
    //
    switch(type) {
    case Baseline:
    case Sequential: 
    case Progressive:
      ParseMarker(io,Residual);
      m_pParser = new(m_pEnviron) ResidualHuffmanScan(m_pFrame,this,marker,
						      m_ucScanStart,m_ucScanStop,
						      m_ucLowBit,m_ucHighBit,
						      true,true);
      break;
    case ACSequential:
    case ACProgressive:
      ParseMarker(io,ACResidual);
      m_pParser = new(m_pEnviron) ResidualACScan(m_pFrame,this,marker,
						 m_ucScanStart,m_ucScanStop,
						 m_ucLowBit,m_ucHighBit,
						 true,true);
      break; 
    default:
      JPG_THROW(NOT_IMPLEMENTED,"Scan::StartParseHiddenRefinementScan",
		"sorry, the coding mode in the codestream is currently not supported");
    }
  } 

  ctrl->PrepareForDecoding();
  m_pParser->StartParseScan(io,ctrl);
}
///

/// Scan::StartParseScan
// Fill in the decoding tables required.
void Scan::StartParseScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  //
  // The residual scan has the parser set here already.
  if (m_pParser == NULL) {
    CreateParser();
  }
  
  ctrl->PrepareForDecoding();
  m_pParser->StartParseScan(io,ctrl);
}
///

/// Scan::StartWriteScan
// Fill in the encoding tables.
void Scan::StartWriteScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  assert(m_pParser);

  if (m_pHuffman)
    m_pHuffman->AdjustToStatistics();
  
  ctrl->PrepareForEncoding();
  m_pParser->StartWriteScan(io,ctrl);
}
///

/// Scan::StartMeasureScan
// Start making a measurement run to optimize the
// huffman tables.
void Scan::StartMeasureScan(class BufferCtrl *ctrl)
{
  assert(m_pParser);

  ctrl->PrepareForEncoding();
  m_pParser->StartMeasureScan(ctrl);
}
///

/// Scan::StartMCURow
// Start a MCU scan.
bool Scan::StartMCURow(void)
{
  assert(m_pParser);

  return m_pParser->StartMCURow();
}
///

/// Scan::ParseMCU
// Parse a single MCU in this scan.
bool Scan::ParseMCU(void)
{
  assert(m_pParser);

  return m_pParser->ParseMCU();
}
///

/// Scan::WriteMCU
// Write a single MCU in this scan.
bool Scan::WriteMCU(void)
{
  assert(m_pParser);

  return m_pParser->WriteMCU();
}
///

/// Scan::WriteFrameType
// Write the scan type marker at the beginning of the
// file.
void Scan::WriteFrameType(class ByteStream *io)
{
  assert(m_pParser);

  m_pParser->WriteFrameType(io);
}
///

/// Scan::Flush
// Flush the remaining bits out to the stream on writing.
void Scan::Flush(void)
{
  if (m_pParser)
    m_pParser->Flush(true);
}
///

/// Scan::CompleteSettings
// All settings are now stored, prepare the parser
void Scan::CompleteSettings(void)
{
  if (m_pParser)
    JPG_THROW(OBJECT_EXISTS,"Scan::CompleteSettings",
	      "Settings are already installed and active");
  
  CreateParser();
}
///

/// Scan::FindThresholds
// Find the thresholds of the JPEG LS scan.
class Thresholds *Scan::FindThresholds(void) const
{
  return m_pFrame->TablesOf()->ThresholdsOf();
}
///

/// Scan::DCHuffmanDecoderOf
// Return the huffman decoder of the DC value for the
// indicated component.
class HuffmanDecoder *Scan::DCHuffmanDecoderOf(UBYTE idx) const
{
  class HuffmanTemplate *t;

  assert(idx < 4);
  
  t = m_pFrame->TablesOf()->FindDCHuffmanTable(m_ucDCTable[idx],m_pFrame->ScanTypeOf(),m_pFrame->PrecisionOf(),
					       m_pFrame->HiddenPrecisionOf(),m_pFrame->TablesOf()->UseResiduals());
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::DCHuffmanDecoderOf","requested DC Huffman coding table not defined");

  return t->DecoderOf();
}
///

/// Scan::ACHuffmanDecoderOf
// Return the huffman decoder of the DC value for the
// indicated component.
class HuffmanDecoder *Scan::ACHuffmanDecoderOf(UBYTE idx) const
{
  class HuffmanTemplate *t;

  assert(idx < 4);

  t = m_pFrame->TablesOf()->FindACHuffmanTable(m_ucACTable[idx],m_pFrame->ScanTypeOf(),m_pFrame->PrecisionOf(),
					       m_pFrame->HiddenPrecisionOf(),m_pFrame->TablesOf()->UseResiduals());
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::ACHuffmanDecoderOf","requested AC Huffman coding table not defined");

  return t->DecoderOf();  
}
///

/// Scan::DCHuffmanCoderOf
// Find the Huffman decoder of the indicated index.
class HuffmanCoder *Scan::DCHuffmanCoderOf(UBYTE idx) const
{
  class HuffmanTemplate *t;
  
  assert(idx < 4);

  t = m_pHuffman->DCTemplateOf(m_ucDCTable[idx],m_pFrame->ScanTypeOf(),m_pFrame->PrecisionOf(),
			       m_pFrame->HiddenPrecisionOf(),m_pFrame->TablesOf()->UseResiduals());
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::DCHuffmanCoderOf","requested DC Huffman coding table not defined");

  t->AdjustToStatistics();
  
  return t->EncoderOf();
}
///

/// Scan::ACHuffmanCoderOf
// Find the Huffman decoder of the indicated index.
class HuffmanCoder *Scan::ACHuffmanCoderOf(UBYTE idx) const
{
  class HuffmanTemplate *t;
  
  assert(idx < 4);

  t = m_pHuffman->ACTemplateOf(m_ucACTable[idx],m_pFrame->ScanTypeOf(),m_pFrame->PrecisionOf(),
			       m_pFrame->HiddenPrecisionOf(),m_pFrame->TablesOf()->UseResiduals());
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::ACHuffmanCoderOf","requested DC Huffman coding table not defined");

  t->AdjustToStatistics();
  
  return t->EncoderOf();
}
///

/// Scan::DCHuffmanStatisticsOf
// Find the Huffman decoder of the indicated index.
class HuffmanStatistics *Scan::DCHuffmanStatisticsOf(UBYTE idx) const
{
  class HuffmanTemplate *t;
  
  assert(idx < 4);

  t = m_pHuffman->DCTemplateOf(m_ucDCTable[idx],m_pFrame->ScanTypeOf(),m_pFrame->PrecisionOf(),
			       m_pFrame->HiddenPrecisionOf(),m_pFrame->TablesOf()->UseResiduals());
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::DCHuffmanStatisticsOf","requested DC Huffman coding table not defined");

  return t->StatisticsOf();
}
///

/// Scan::ACHuffmanStatisticsOf
// Find the Huffman decoder of the indicated index.
class HuffmanStatistics *Scan::ACHuffmanStatisticsOf(UBYTE idx) const
{
  class HuffmanTemplate *t;
  
  assert(idx < 4);

  t = m_pHuffman->ACTemplateOf(m_ucACTable[idx],m_pFrame->ScanTypeOf(),m_pFrame->PrecisionOf(),
			       m_pFrame->HiddenPrecisionOf(),m_pFrame->TablesOf()->UseResiduals());
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::ACHuffmanStatisticsOf","requested AC Huffman coding table not defined");

  return t->StatisticsOf();
}
///

/// Scan::DCConditionerOf
// Find the arithmetic coding conditioner table for the indicated
// component and the DC band.
class ACTemplate *Scan::DCConditionerOf(UBYTE idx) const
{ 
  assert(idx < 4);

  if (m_pConditioner) {
    return m_pConditioner->DCTemplateOf(m_ucDCTable[idx]);
  }

  return m_pFrame->TablesOf()->FindDCConditioner(m_ucDCTable[idx]);
}
///

/// Scan::ACConditionerOf
// The same for the AC band.
class ACTemplate *Scan::ACConditionerOf(UBYTE idx) const
{ 
  assert(idx < 4);

  if (m_pConditioner) {
    return m_pConditioner->ACTemplateOf(m_ucACTable[idx]);
  }

  return m_pFrame->TablesOf()->FindACConditioner(m_ucACTable[idx]);
}
///
