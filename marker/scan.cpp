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
** Represents all data in a single scan, and hence is the SOS marker.
**
** $Id: scan.cpp,v 1.120 2024/03/25 18:42:33 thor Exp $
**
*/

/// Includes
#include "marker/scan.hpp"
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
#include "codestream/singlecomponentlsscan.hpp"
#include "codestream/lineinterleavedlsscan.hpp"
#include "codestream/sampleinterleavedlsscan.hpp"
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
    m_pHuffman(NULL), m_pConditioner(NULL), m_bHidden(false)
{
  m_ucScanIndex = 0;
  
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

  //
  // No need to write the DHT marker if this is empty anyhow.
  if (m_pHuffman && m_pHuffman->isEmpty() == false) {
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
  int i,j;

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
    for(j = 0;j < i;j++) {
      if (m_ucComponent[j] == data)
        JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker","SOS includes the same component twice");
    }
    
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
      // The VESA scan types may use this, but the tables are hardwired.
      m_ucDCTable[i]      = (i == 0)?(0):(1);
      m_ucACTable[i]      = (i == 0)?(0):(1);
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
    if (data > 2)
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
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
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
    // fall through
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
  ScanType type = m_pFrame->ScanTypeOf();
  //
  assert(m_pParser == NULL);
  //
  // Check whether all components are there.
  for(UBYTE i = 0;i < m_ucCount && i < 4;i++) {
    if (ComponentOf(i) == NULL) {
      JPG_THROW(MALFORMED_STREAM,"Scan::CreateParser",
                "found a component ID in a scan that does not exist");
    }
  }
  //
  switch(type) {
  case Baseline:
    m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
                                                     m_ucScanStart,m_ucScanStop,
                                                     m_ucLowBit + m_ucHiddenBits,
                                                     m_ucHighBit + m_ucHiddenBits,
                                                     false,false,false,true);
    break;
  case Sequential:
    m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
                                                     m_ucScanStart,m_ucScanStop,
                                                     m_ucLowBit + m_ucHiddenBits,
                                                     m_ucHighBit + m_ucHiddenBits);
    break;
  case DifferentialSequential:
    m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
                                                     m_ucScanStart,m_ucScanStop,
                                                     m_ucLowBit + m_ucHiddenBits,
                                                     m_ucHighBit + m_ucHiddenBits,true);
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
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits);
    break;
  case ACDifferentialSequential:
    m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits,true);
    break;
  case Progressive:
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits);
    } else { 
      m_pParser = new(m_pEnviron) class RefinementScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits);
    }
    break;
  case ResidualProgressive:
    if (m_ucHighBit == 0) { 
      m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits,
                                                       true,true);
    } else { 
      m_pParser = new(m_pEnviron) class RefinementScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits,
                                                       true,true);
    }
    break;
  case DifferentialProgressive:
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class SequentialScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits,true);
    } else { 
      // Even though the specs do not mention this, it makes perfect sense that the
      // refinement scan is a regular refinement scan without modification.
      m_pParser = new(m_pEnviron) class RefinementScan(m_pFrame,this,
                                                       m_ucScanStart,m_ucScanStop,
                                                       m_ucLowBit + m_ucHiddenBits,
                                                       m_ucHighBit + m_ucHiddenBits,true);
    }
    break;
  case ACProgressive: 
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
                                                         m_ucScanStart,m_ucScanStop,
                                                         m_ucLowBit + m_ucHiddenBits,
                                                         m_ucHighBit + m_ucHiddenBits);
    } else { 
      m_pParser = new(m_pEnviron) class ACRefinementScan(m_pFrame,this,
                                                         m_ucScanStart,m_ucScanStop,
                                                         m_ucLowBit + m_ucHiddenBits,
                                                         m_ucHighBit + m_ucHiddenBits);
    }
    break;
  case ACDifferentialProgressive: 
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
                                                         m_ucScanStart,m_ucScanStop,
                                                         m_ucLowBit + m_ucHiddenBits,
                                                         m_ucHighBit + m_ucHiddenBits,
                                                         true);
    } else { 
      m_pParser = new(m_pEnviron) class ACRefinementScan(m_pFrame,this,
                                                         m_ucScanStart,m_ucScanStop,
                                                         m_ucLowBit + m_ucHiddenBits,
                                                         m_ucHighBit + m_ucHiddenBits,
                                                         true);
    }
    break;
  case ACResidualProgressive:  
    if (m_ucHighBit == 0) { // The first scan is parsed off by the regular parser.
      m_pParser = new(m_pEnviron) class ACSequentialScan(m_pFrame,this,
                                                         m_ucScanStart,m_ucScanStop,
                                                         m_ucLowBit + m_ucHiddenBits,
                                                         m_ucHighBit + m_ucHiddenBits,
                                                         false,true);
    } else { 
      m_pParser = new(m_pEnviron) class ACRefinementScan(m_pFrame,this,
                                                         m_ucScanStart,m_ucScanStop,
                                                         m_ucLowBit + m_ucHiddenBits,
                                                         m_ucHighBit + m_ucHiddenBits,
                                                         false,true);
    }
    break;
  case Residual:
    m_pParser = new(m_pEnviron) SequentialScan(m_pFrame,this,
                                               m_ucScanStart,m_ucScanStop,
                                               m_ucLowBit + m_ucHiddenBits,
                                               m_ucHighBit + m_ucHiddenBits,
                                               true,true);
    break;
  case ACResidual:
    m_pParser = new(m_pEnviron) ACSequentialScan(m_pFrame,this,
                                                 m_ucScanStart,m_ucScanStop,
                                                 m_ucLowBit + m_ucHiddenBits,
                                                 m_ucHighBit + m_ucHiddenBits,
                                                 true,true);  
    break;
  case ResidualDCT:
    m_pParser = new(m_pEnviron) SequentialScan(m_pFrame,this,
                                               m_ucScanStart,m_ucScanStop,
                                               m_ucLowBit + m_ucHiddenBits,
                                               m_ucHighBit + m_ucHiddenBits,
                                               false,false,true);
    break; 
  case ACResidualDCT:
    m_pParser = new(m_pEnviron) ACSequentialScan(m_pFrame,this,
                                                 m_ucScanStart,m_ucScanStop,
                                                 m_ucLowBit + m_ucHiddenBits,
                                                 m_ucHighBit + m_ucHiddenBits,
                                                 false,false,true);
    break;
  case JPEG_LS:
    // Depends on the interleaving
    switch(m_ucScanStop) {
    case 0:
      if (m_ucCount != 1)
        JPG_THROW(MALFORMED_STREAM,"Scan::CreateParser",
                  "invalid codestream, found a single comonent scan containing more than one component");
      m_pParser = new(m_pEnviron) class SingleComponentLSScan(m_pFrame,this,
                                                              m_ucScanStart, // NEAR
                                                              m_ucMappingTable,
                                                              m_ucLowBit + m_ucHiddenBits); 
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
void Scan::InstallDefaults(UBYTE depth,ULONG tagoffset,const struct JPG_TagItem *tags)
{
  bool ishuffman    = false;
  bool ispredictive = false;
  bool isjpegls     = false;
  bool colortrafo   = m_pFrame->TablesOf()->hasSeparateChroma(m_pFrame->DepthOf());
  ScanType type     = m_pFrame->ScanTypeOf();

  assert(m_pParser == NULL);
  
  switch(type) {
  case Baseline:
  case Sequential:
  case Progressive:
  case DifferentialSequential:
  case DifferentialProgressive:
  case Residual:
  case ResidualProgressive:
  case ResidualDCT:
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
  case ACResidualProgressive:
  case ACResidualDCT:
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
  case ResidualProgressive:
  case ACResidualProgressive:
  case ResidualDCT:
  case ACResidualDCT:
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
  m_ucComponent[0] = tags->GetTagData(JPGTAG_SCAN_COMPONENT0            ,0);
  m_ucComponent[1] = tags->GetTagData(JPGTAG_SCAN_COMPONENT1            ,1);
  m_ucComponent[2] = tags->GetTagData(JPGTAG_SCAN_COMPONENT2            ,2);  
  m_ucComponent[3] = tags->GetTagData(JPGTAG_SCAN_COMPONENT3            ,3);
  m_ucComponent[0] = tags->GetTagData(JPGTAG_SCAN_COMPONENT0 + tagoffset,m_ucComponent[0]);
  m_ucComponent[1] = tags->GetTagData(JPGTAG_SCAN_COMPONENT1 + tagoffset,m_ucComponent[1]);
  m_ucComponent[2] = tags->GetTagData(JPGTAG_SCAN_COMPONENT2 + tagoffset,m_ucComponent[2]);  
  m_ucComponent[3] = tags->GetTagData(JPGTAG_SCAN_COMPONENT3 + tagoffset,m_ucComponent[3]);
  m_ucHiddenBits   = m_pFrame->TablesOf()->HiddenDCTBitsOf();
  //
  // Install the Huffman table specifications
  // There are only two tables used here, thus this is always fine for baseline.
  for(UBYTE i = 0;i < depth;i++) {
    UBYTE c = m_ucComponent[i]; // get the component.

    if (/*ishuffman &&*/ colortrafo) {
      m_ucDCTable[i] = (c == 0)?(0):(1);
    } else {
      m_ucDCTable[i] = 0;
    }
    //
    // AC coding not required for predictive.
    if (/*ishuffman &&*/ !ispredictive && colortrafo) {
      m_ucACTable[i] = (c == 0)?(0):(1);
    } else {
      m_ucACTable[i] = 0;
    }
  } 
  //
  // Install and check the scan parameters for the progressive scan.
  switch(type) {
  case Progressive:
  case ACProgressive:
  case DifferentialProgressive:
  case ACDifferentialProgressive:
  case ResidualProgressive:
  case ACResidualProgressive:
    m_ucScanStart    = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_START            ,m_ucScanStart);
    m_ucScanStop     = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_STOP             ,m_ucScanStop);    
    m_ucScanStart    = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_START + tagoffset,m_ucScanStart);
    m_ucScanStop     = tags->GetTagData(JPGTAG_SCAN_SPECTRUM_STOP  + tagoffset,m_ucScanStop);
    //
    if (type != ResidualProgressive && type != ACResidualProgressive) {
      if (m_ucScanStart == 0 && m_ucScanStop)
        JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
                  "DC coefficients must be in a separate scan in the progressive mode");
      if (m_ucScanStart && m_ucScanStop < m_ucScanStart)
        JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
                  "Spectral selection stop must be larger or equal than spectral selection start");
      if (m_ucScanStart && m_ucCount > 1)
        JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
                  "In the progressive mode, the AC components must be coded in all separate scans");
    } else {
      if (m_ucScanStop < m_ucScanStart)
        JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
                  "Spectral selection stop must be larger or equal than spectral selection start");
    }
    if (m_ucScanStop >= 64)
      JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
                "Spectral selection stop is out of range, must be <= 63");

    m_ucHighBit      = tags->GetTagData(JPGTAG_SCAN_APPROXIMATION_HI            ,m_ucHighBit);
    m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_APPROXIMATION_LO            ,m_ucLowBit);
    m_ucHighBit      = tags->GetTagData(JPGTAG_SCAN_APPROXIMATION_HI + tagoffset,m_ucHighBit);
    m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_APPROXIMATION_LO + tagoffset,m_ucLowBit);
    if (m_ucHighBit > 0 && m_ucHighBit != m_ucLowBit + 1)
      JPG_THROW(INVALID_PARAMETER,"Scan::InstallDefaults",
                "Successive approximation refinement must include only a single bitplane");
    //
    break;
  case JPEG_LS:
    // This is the NEAR value of LS. Note that this is never a residual scan.
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
    m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_POINTTRANSFORM            ,m_ucLowBit);
    m_ucLowBit       = tags->GetTagData(JPGTAG_SCAN_POINTTRANSFORM + tagoffset,m_ucLowBit);
    if (m_ucLowBit >= m_pFrame->PrecisionOf())
      JPG_THROW(OVERFLOW_PARAMETER,"Scan::InstallDefaults",
                "Point transformation removes more bits than available in the source data");
  default:
    break;
  }

  if (m_pParser)
    JPG_THROW(OBJECT_EXISTS,"Scan::CompleteSettings",
              "Settings are already installed and active");
  
  CreateParser();
}
///

/// Scan::MakeHiddenRefinementScan
// Make this scan a hidden refinement scan starting at the indicated
// bit position in the indicated component label.
void Scan::MakeHiddenRefinementScan(UBYTE bitposition,class Component *comp,UBYTE start,UBYTE stop)
{
  bool colortrafo = m_pFrame->TablesOf()->hasSeparateChroma(m_pFrame->DepthOf());
  bool residual   = false; // for a residual scan type.
  
  assert(m_pParser == NULL);

  
  if (m_pFrame->DepthOf() > 4)
    JPG_THROW(INVALID_PARAMETER,"Scan::MakeHiddenRefinementScan",
              "hidden refinement scans are confined to four components at most");

  m_ucScanStart    = start;
  m_ucScanStop     = stop; 
  m_ucLowBit       = bitposition;
  m_ucHighBit      = bitposition+1;
  m_ucHiddenBits   = 0; // not here anymore.
  m_bHidden        = true;

  switch(m_pFrame->ScanTypeOf()) { 
  case Residual:
  case ACResidual:
  case ResidualProgressive: 
  case ACResidualProgressive:
    // Only one component in the scan.
    assert(stop >= start);
      
    m_ucCount        = 1;
    m_ucComponent[0] = comp->IDOf();
    break;
  default:
    if (start == 0) {
      UBYTE i;
      
      assert(stop == 0); // This is a DC scan, hopefully.
      
      m_ucCount        = m_pFrame->DepthOf();
      for(i = 0;i < m_ucCount;i++) {
        m_ucComponent[i] = m_pFrame->ComponentOf(i)->IDOf();
        m_ucDCTable[i]   = 0;
        m_ucACTable[i]   = 0; // Fixed later.
      }
    } else {
      // Only one component in the scan.
      assert(stop >= start);
      
      m_ucCount        = 1;
      m_ucComponent[0] = comp->IDOf();
    }
    break;
  }
  
  switch(m_pFrame->ScanTypeOf()) {
  case Baseline:
  case Sequential:
  case Progressive:
    if (colortrafo) {
      m_ucACTable[0] = (comp && comp->IndexOf() == 0)?(0):(1);  // Luma uses a separate table.
      m_ucDCTable[0] = 0;
      m_ucDCTable[1] = m_ucDCTable[2] = m_ucDCTable[3] = 1; // Chroma uses a separate table.
    } else {
      m_ucACTable[0] = 0;
      m_ucDCTable[0] = 0;
      m_ucDCTable[1] = m_ucDCTable[2] = m_ucDCTable[3] = 0; // Chroma uses the same table.
    }
    m_pHuffman = new(m_pEnviron) HuffmanTable(m_pEnviron);
    m_pParser  = new(m_pEnviron) RefinementScan(m_pFrame,this,
                                                start,stop,
                                                bitposition,bitposition+1,
                                                false,false);
    break;
  case ACSequential:
  case ACProgressive:
#if ACCUSOFT_CODE
    m_ucACTable[0] = 0;
    m_ucDCTable[0] = 0;
    m_pConditioner = new(m_pEnviron) ACTable(m_pEnviron);
    m_pParser      = new(m_pEnviron) ACRefinementScan(m_pFrame,this,
                                                      start,stop,
                                                      bitposition,bitposition+1,
                                                      false,false);
#else
    JPG_THROW(NOT_IMPLEMENTED," Scan::MakeHiddenRefinementScan",
              "Arithmetic coding option not available in your code release, please contact Accusoft for a full version");
#endif
    break;
  case Residual:
  case ResidualProgressive:
    residual = true;
    // runs into the following.
  case ResidualDCT:
    if (colortrafo) {
      m_ucACTable[0] = (comp && comp->IndexOf() == 0)?(0):(1);  // Luma uses a separate table.
      m_ucDCTable[0] = 0;
      m_ucDCTable[1] = m_ucDCTable[2] = m_ucDCTable[3] = 1; // Chroma uses a separate table.
    } else {
      m_ucACTable[0] = 0;
      m_ucDCTable[0] = 0;
      m_ucDCTable[1] = m_ucDCTable[2] = m_ucDCTable[3] = 0; // Chroma uses the same table.
    }
    assert(residual == false || (start == 0 && stop == 63));
    m_pHuffman = new(m_pEnviron) HuffmanTable(m_pEnviron);
    m_pParser  = new(m_pEnviron) RefinementScan(m_pFrame,this,
                                                start,stop,
                                                bitposition,bitposition+1,
                                                false,residual);
    break;
  case ACResidual:
  case ACResidualProgressive:
    residual = true;
    // fall through
  case ACResidualDCT:
#if ACCUSOFT_CODE
    m_ucACTable[0] = 0;
    m_ucDCTable[0] = 0;
    assert(residual == false || (start == 0 && stop == 63));
    m_pConditioner = new(m_pEnviron) ACTable(m_pEnviron);
    m_pParser      = new(m_pEnviron) ACRefinementScan(m_pFrame,this,
                                                      start,stop,
                                                      bitposition,bitposition+1,
                                                      false,residual);
#else
    JPG_THROW(NOT_IMPLEMENTED," Scan::MakeHiddenRefinementScan",
              "Arithmetic coding option not available in your code release, please contact Accusoft for a full version");
#endif   
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Scan::MakeHiddenRefinementScan",
              "frame type does not support hidden refinement scans");
    break;
  }
}
///

/// Scan::StartParseHiddenRefinementScan
// Parse off a hidden refinement scan from the given position.
void Scan::StartParseHiddenRefinementScan(class ByteStream *io,class BufferCtrl *ctrl)
{
  m_bHidden = true;
  bool residual = false;

  if (m_pParser == NULL) {
    ScanType type = m_pFrame->ScanTypeOf();
    //
    switch(type) {
    case Baseline:
    case Sequential: 
    case Progressive:
      ParseMarker(io,Progressive);
      if (m_ucHighBit != m_ucLowBit + 1)
        JPG_THROW(MALFORMED_STREAM,"Scan::ParseMarker",
                  "SOS high bit is invalid, hidden refinement must refine by one bit per scan");
      m_pParser = new(m_pEnviron) RefinementScan(m_pFrame,this,
                                                 m_ucScanStart,m_ucScanStop,
                                                 m_ucLowBit,m_ucHighBit,
                                                 false,false);
      break;
    case ACSequential:
    case ACProgressive:
#if ACCUSOFT_CODE
      ParseMarker(io,ACProgressive);
      if (m_ucHighBit != m_ucLowBit + 1)
        JPG_THROW(MALFORMED_STREAM,"Scan::StartParseHiddenRefinementScan",
                  "SOS high bit is invalid, hidden refinement must refine by one bit per scan");
      m_pParser = new(m_pEnviron) ACRefinementScan(m_pFrame,this,
                                                   m_ucScanStart,m_ucScanStop,
                                                   m_ucLowBit,m_ucHighBit,
                                                   false,false);
#else
      JPG_THROW(NOT_IMPLEMENTED,"Scan::StartParseHiddenRefinementScan",
                "Arithmetic coding option not available in your code release, please contact Accusoft for a full version");
#endif
      break; 
    case Residual:
    case ResidualProgressive:
      residual = true;
      // fall through
    case ResidualDCT:
      ParseMarker(io,ResidualProgressive);
      if (m_ucHighBit != m_ucLowBit + 1)
        JPG_THROW(MALFORMED_STREAM,"Scan::StartParseHiddenRefinementScan",
                  "SOS high bit is invalid, hidden refinement must refine by one bit per scan");
      m_pParser  = new(m_pEnviron) RefinementScan(m_pFrame,this,
                                                  m_ucScanStart,m_ucScanStop,
                                                  m_ucLowBit,m_ucHighBit,
                                                  false,residual);
      break;
    case ACResidual:
    case ACResidualProgressive:
      residual = true;
      // fall through
    case ACResidualDCT:
#if ACCUSOFT_CODE
      ParseMarker(io,ACResidualProgressive);
      if (m_ucHighBit != m_ucLowBit + 1)
        JPG_THROW(MALFORMED_STREAM,"Scan::StartParseHiddenRefinementScan",
                  "SOS high bit is invalid, hidden refinement must refine by one bit per scan");
      m_pParser  = new(m_pEnviron) ACRefinementScan(m_pFrame,this, 
                                                    m_ucScanStart,m_ucScanStop,
                                                    m_ucLowBit,m_ucHighBit,
                                                    false,true);
#else
      JPG_THROW(NOT_IMPLEMENTED," Scan::MakeHiddenRefinementScan",
                "Arithmetic coding option not available in your code release, "
                "please contact Accusoft for a full version");
#endif   
      break; 
    default:
      JPG_THROW(NOT_IMPLEMENTED,"Scan::StartParseHiddenRefinementScan",
                "sorry, the coding mode in the codestream is currently not supported");
    }
  } 

  ctrl->PrepareForDecoding();
  m_pParser->StartParseScan(io,NULL,ctrl);
}
///

/// Scan::StartParseScan
// Fill in the decoding tables required.
void Scan::StartParseScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
  //
  // The residual scan has the parser set here already.
  if (m_pParser == NULL)
    CreateParser();
  
  ctrl->PrepareForDecoding();
  m_pParser->StartParseScan(io,chk,ctrl);
}
///

/// Scan::StartWriteScan
// Fill in the encoding tables.
void Scan::StartWriteScan(class ByteStream *io,class Checksum *chk,class BufferCtrl *ctrl)
{
  assert(m_pParser);

  if (m_pHuffman)
    m_pHuffman->AdjustToStatistics();
  
  ctrl->PrepareForEncoding();
  m_pParser->StartWriteScan(io,chk,ctrl);
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

/// Scan::StartOptimizeScan
// Start making a R/D optimization
void Scan::StartOptimizeScan(class BufferCtrl *ctrl)
{
  assert(m_pParser);
  //
  ctrl->PrepareForEncoding();
  m_pParser->StartOptimizeScan(ctrl);
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

  //
  // Do not write the frame type of hidden scans.
  if (m_bHidden) {
    assert(m_pNext);
    m_pNext->WriteFrameType(io);
  } else {
    m_pParser->WriteFrameType(io);
  }
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
  ScanType sc = m_pFrame->ScanTypeOf();

  assert(idx < 4);
  
  t = m_pFrame->TablesOf()->FindDCHuffmanTable(m_ucDCTable[idx],sc,m_pFrame->PrecisionOf(),
                                               m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
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
  ScanType sc = m_pFrame->ScanTypeOf();

  assert(idx < 4);

  t = m_pFrame->TablesOf()->FindACHuffmanTable(m_ucACTable[idx],sc,m_pFrame->PrecisionOf(),
                                               m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
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
  ScanType sc = m_pFrame->ScanTypeOf();

  assert(idx < 4);

  t = m_pHuffman->DCTemplateOf(m_ucDCTable[idx],sc,m_pFrame->PrecisionOf(),
                               m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
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
  ScanType sc = m_pFrame->ScanTypeOf();
  
  assert(idx < 4);

  t = m_pHuffman->ACTemplateOf(m_ucACTable[idx],sc,m_pFrame->PrecisionOf(),
                               m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::ACHuffmanCoderOf","requested AC Huffman coding table not defined");

  t->AdjustToStatistics();
  
  return t->EncoderOf();
}
///

/// Scan::DCHuffmanStatisticsOf
// Find the Huffman decoder of the indicated index.
class HuffmanStatistics *Scan::DCHuffmanStatisticsOf(UBYTE idx) const
{
  class HuffmanTemplate *t;
  ScanType sc = m_pFrame->ScanTypeOf(); 
 
  assert(idx < 4);

  t = m_pHuffman->DCTemplateOf(m_ucDCTable[idx],sc,m_pFrame->PrecisionOf(),
                               m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::DCHuffmanStatisticsOf","requested DC Huffman coding table not defined");

  return t->StatisticsOf(true);
}
///

/// Scan::ACHuffmanStatisticsOf
// Find the Huffman decoder of the indicated index.
class HuffmanStatistics *Scan::ACHuffmanStatisticsOf(UBYTE idx) const
{
  class HuffmanTemplate *t;
  ScanType sc = m_pFrame->ScanTypeOf(); 

  assert(idx < 4);

  t = m_pHuffman->ACTemplateOf(m_ucACTable[idx],sc,m_pFrame->PrecisionOf(),
                               m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
  
  if (t == NULL)
      JPG_THROW(OBJECT_DOESNT_EXIST,"Scan::ACHuffmanStatisticsOf","requested AC Huffman coding table not defined");

  return t->StatisticsOf(false);
}
///

/// Scan::DCConditionerOf
// Find the arithmetic coding conditioner table for the indicated
// component and the DC band.
class ACTemplate *Scan::DCConditionerOf(UBYTE idx) const
{ 
  ScanType sc = m_pFrame->ScanTypeOf();
  assert(idx < 4);

  if (m_pConditioner) {
    return m_pConditioner->DCTemplateOf(m_ucDCTable[idx],sc,m_pFrame->PrecisionOf(),
                                        m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
  }

  return m_pFrame->TablesOf()->FindDCConditioner(m_ucDCTable[idx],sc,m_pFrame->PrecisionOf(),
                                                 m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
}
///

/// Scan::ACConditionerOf
// The same for the AC band.
class ACTemplate *Scan::ACConditionerOf(UBYTE idx) const
{ 
  ScanType sc = m_pFrame->ScanTypeOf();
  assert(idx < 4);

  if (m_pConditioner) {
    return m_pConditioner->ACTemplateOf(m_ucACTable[idx],sc,m_pFrame->PrecisionOf(),
                                        m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
  }

  return m_pFrame->TablesOf()->FindACConditioner(m_ucACTable[idx],sc,m_pFrame->PrecisionOf(),
                                                 m_pFrame->HiddenPrecisionOf(),m_ucScanIndex);
}
///

/// Scan::OptimizeDCTBlock
// Optimize the given DCT block for ideal rate-distortion performance. The
// input parameters are the component this applies to, the critical R/D slope,
// the original transformed but unquantized DCT data and the quantized DCT
// block.
void Scan::OptimizeDCTBlock(LONG bx,LONG by,UBYTE compidx,DOUBLE lambda,
                            class DCT *dct,LONG quantized[64])
{
  UBYTE i;

  assert(m_pParser);

  for(i = 0;i < m_ucCount;i++) {
    if (m_pComponent[i] && m_pComponent[i]->IndexOf() == compidx) {
      m_pParser->OptimizeBlock(bx,by,i,lambda,dct,quantized);
      break;
    }
  }
}
///

/// Scan::OptimizeDC
// Run a joint optimization of the R/D performance of all DC coefficients
// within this scan. This requires a separate joint effort as DC coefficients
// are encoded dependently.
void Scan::OptimizeDC(void)
{
  assert(m_pParser);

  m_pParser->OptimizeDC();
}
///
