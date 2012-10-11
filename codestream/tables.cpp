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
** This class keeps all the coding tables, huffman, AC table, quantization
** and other side information.
**
** $Id: tables.cpp,v 1.56 2012-10-07 15:58:08 thor Exp $
**
*/

/// Include
#include "codestream/tables.hpp"
#include "marker/quantization.hpp"
#include "marker/huffmantable.hpp"
#include "marker/actable.hpp"
#include "marker/adobemarker.hpp"
#include "marker/losslessmarker.hpp"
#include "marker/residualmarker.hpp"
#include "marker/residualspecsmarker.hpp"
#include "marker/restartintervalmarker.hpp"
#include "marker/jfifmarker.hpp"
#include "marker/exifmarker.hpp"
#include "marker/tonemappingmarker.hpp"
#include "marker/thresholds.hpp"
#include "marker/lscolortrafo.hpp"
#include "coding/huffmantemplate.hpp"
#include "coding/actemplate.hpp"
#include "io/bytestream.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/ycbcrtrafo.hpp"
#include "colortrafo/trivialtrafo.hpp"
#include "colortrafo/lslosslesstrafo.hpp"
#include "tools/traits.hpp"
#include "dct/dct.hpp"
#include "dct/idct.hpp"
#include "dct/sermsdct.hpp"
///

/// Tables::Tables
Tables::Tables(class Environ *env)
  : JKeeper(env), m_pQuant(NULL), m_pHuffman(NULL), m_pConditioner(NULL), 
    m_pRestart(NULL), m_pColorInfo(NULL), m_pResolutionInfo(NULL), m_pCameraInfo(NULL),
    m_pResidualData(NULL), m_pRefinementData(NULL), m_pResidualSpecs(NULL), m_pColorTrafo(NULL), 
    m_pLosslessMarker(NULL), m_pToneMappingMarker(NULL),
    m_pThresholds(NULL), m_pLSColorTrafo(NULL),
    m_pusToneMapping(NULL), m_pusInverseMapping(NULL),
    m_bForceFixpoint(false), m_bDisableColor(false)
{
}
///

/// Tables::~Tables
Tables::~Tables(void)
{
  class ToneMappingMarker *tone;

  while((tone = m_pToneMappingMarker)) {
    m_pToneMappingMarker = tone->NextOf();
    delete tone;
  }

  if (m_pusToneMapping)
    m_pEnviron->FreeVec(m_pusToneMapping);

  if (m_pusInverseMapping)
    m_pEnviron->FreeVec(m_pusInverseMapping);
 
  delete m_pLSColorTrafo;
  delete m_pThresholds;
  delete m_pQuant;
  delete m_pHuffman;
  delete m_pConditioner;
  delete m_pColorInfo;
  delete m_pResolutionInfo;
  delete m_pCameraInfo;
  delete m_pResidualData;
  delete m_pRefinementData;
  delete m_pResidualSpecs;
  delete m_pColorTrafo;
  delete m_pLosslessMarker;
  delete m_pRestart;
}
///

/// Tables::InstallDefaultTables
// For writing, install the standard suggested tables
// for the given quality index between 0 and 100.
void Tables::InstallDefaultTables(const struct JPG_TagItem *tags)
{
  class ToneMappingMarker *tone;
  LONG frametype  = tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE);
  ULONG quality   = tags->GetTagData(JPGTAG_IMAGE_QUALITY,80);
  ULONG hdrquality= tags->GetTagData(JPGTAG_IMAGE_HDRQUALITY,MAX_ULONG);
  ULONG maxerror  = tags->GetTagData(JPGTAG_IMAGE_ERRORBOUND); // also the near value for LS
  ULONG precision = tags->GetTagData(JPGTAG_IMAGE_PRECISION,8);
  LONG colortrafo = tags->GetTagData(JPGTAG_IMAGE_COLORTRANSFORMATION,JPGFLAG_IMAGE_COLORTRANSFORMATION_YCBCR);
  bool lossless   = (frametype & JPGFLAG_REVERSIBLE_DCT)?true:false;
  bool residual   = (frametype & JPGFLAG_RESIDUAL_CODING)?true:false;
  bool hadamard   = tags->GetTagData(JPGTAG_IMAGE_ENABLE_HADAMARD,false)?true:false;
  bool noiseshaping = tags->GetTagData(JPGTAG_IMAGE_ENABLE_NOISESHAPING,false)?true:false;
  ULONG restart   = tags->GetTagData(JPGTAG_IMAGE_RESTART_INTERVAL);
  UBYTE preshift  = 0;
  UBYTE hiddenbits= tags->GetTagData(JPGTAG_IMAGE_HIDDEN_DCTBITS,0);
  UWORD *map0     = (UWORD *)(tags->GetTagPtr(JPGTAG_IMAGE_TONEMAPPING0));
  UWORD *map1     = (UWORD *)(tags->GetTagPtr(JPGTAG_IMAGE_TONEMAPPING1));
  UWORD *map2     = (UWORD *)(tags->GetTagPtr(JPGTAG_IMAGE_TONEMAPPING2));
  UWORD *map3     = (UWORD *)(tags->GetTagPtr(JPGTAG_IMAGE_TONEMAPPING3));

  if (quality > 100)
    JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
	      "image quality can be at most 100");

  if (hdrquality != MAX_ULONG && hdrquality > 100)
    JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
	      "quality of the extensions layer can be at most 100");

  if (residual) {
    switch(frametype & 0x07) {
    case JPGFLAG_BASELINE:
    case JPGFLAG_SEQUENTIAL:
    case JPGFLAG_PROGRESSIVE:
      if (precision > 8) {
	preshift  = precision - 8;
      }
      break;
    }
  }

  if (hiddenbits) {
    if (int(hiddenbits) > precision - 8)
      JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
		"can only hide at most the number of extra bits between "
		"the native bit depth of the image and eight bits per pixel");
    if (hiddenbits + 8 > 12)
      JPG_THROW(OVERFLOW_PARAMETER,"Tables::InstallDefaultTables",
		"the maximum number of hidden DCT bits can be at most four");
  }

  if (m_pQuant != NULL || m_pHuffman != NULL || m_pColorInfo != NULL || m_pResolutionInfo != NULL ||
      m_pLosslessMarker != NULL || m_pRestart != NULL)
    JPG_THROW(OBJECT_EXISTS,"Tables::InstallDefaultTables","Huffman and quantization tables are already defined");

  //
  // Lossy modes require a DQT table.
  switch(frametype & 0x07) {
  case JPGFLAG_BASELINE:
  case JPGFLAG_SEQUENTIAL:
  case JPGFLAG_PROGRESSIVE:
    m_pQuant = new(m_pEnviron) Quantization(m_pEnviron);
    m_pQuant->InitDefaultTables(quality,hdrquality,
				colortrafo != JPGFLAG_IMAGE_COLORTRANSFORMATION_NONE);
    break;
  }

  switch(colortrafo) {
  case JPGFLAG_IMAGE_COLORTRANSFORMATION_NONE:
    m_pColorInfo = new(m_pEnviron) AdobeMarker(m_pEnviron);
    m_pColorInfo->SetColorSpace(AdobeMarker::None);
    break;
  case JPGFLAG_IMAGE_COLORTRANSFORMATION_YCBCR:
    // Also build the JFIF marker here if it is YCbCr.
    // Later on, we should be able to define the image resolution.
    /*
    **
    ** Not a good idea if RGB marker is written.
    **
    m_pResolutionInfo = new(m_pEnviron) JFIFMarker(m_pEnviron);
    m_pResolutionInfo->SetImageResolution(96,96);
    */
    break;
  case JPGFLAG_IMAGE_COLORTRANSFORMATION_LSRCT:
    m_pLSColorTrafo = new(m_pEnviron) LSColorTrafo(m_pEnviron);
    m_pLSColorTrafo->InstallDefaults(precision,maxerror);
    break;
  }

  if (restart) {
    m_pRestart      = new(m_pEnviron) class RestartIntervalMarker(m_pEnviron);
    m_pRestart->InstallDefaults(restart);
  }

  if ((frametype & 0x07) == JPGFLAG_JPEG_LS) {
    if (maxerror > 0) {
      m_pThresholds = new(m_pEnviron) class Thresholds(m_pEnviron);
      m_pThresholds->InstallDefaults(precision,maxerror);
    }
  }
  //
  // Create tone mapping curves. Even if all components use identical curves, I'm
  // here writing the same table multiple times. Could be done simpler, indeed.
  //
  if (map0) {
    tone = new(m_pEnviron) class ToneMappingMarker(m_pEnviron);
    tone->NextOf() = m_pToneMappingMarker;
    m_pToneMappingMarker = tone;
    tone->InstallDefaultParameters(0,precision,hiddenbits,map0);
  }
  if (map1 && map1 != map0) {
    tone = new(m_pEnviron) class ToneMappingMarker(m_pEnviron);
    tone->NextOf() = m_pToneMappingMarker;
    m_pToneMappingMarker = tone;
    tone->InstallDefaultParameters(1,precision,hiddenbits,map1);
  }
  if (map2 && map2 != map0) {
    tone = new(m_pEnviron) class ToneMappingMarker(m_pEnviron);
    tone->NextOf() = m_pToneMappingMarker;
    m_pToneMappingMarker = tone;
    tone->InstallDefaultParameters(2,precision,hiddenbits,map2);
  }
  if (map3 && map3 != map0) {
    tone = new(m_pEnviron) class ToneMappingMarker(m_pEnviron);
    tone->NextOf() = m_pToneMappingMarker;
    m_pToneMappingMarker = tone;
    tone->InstallDefaultParameters(3,precision,hiddenbits,map3);
  }
  
  if (residual || hiddenbits) {
    if (hdrquality > 0)
      m_pResidualData   = new(m_pEnviron) ResidualMarker(m_pEnviron,ResidualMarker::Residual);
    if (hiddenbits)
      m_pRefinementData = new(m_pEnviron) ResidualMarker(m_pEnviron,ResidualMarker::Refinement);
    m_pResidualSpecs  = new(m_pEnviron) ResidualSpecsMarker(m_pEnviron);

    m_pResidualSpecs->InstallPreshift(preshift);
    m_pResidualSpecs->InstallHiddenBits(hiddenbits);
    
    if (map0)
      m_pResidualSpecs->InstallToneMapping(0,0);
    if (map1) {
      if (map1 == map0) 
	m_pResidualSpecs->InstallToneMapping(1,0);
      else 
	m_pResidualSpecs->InstallToneMapping(1,1);
    }
    if (map2) {
      if (map2 == map0) 
	m_pResidualSpecs->InstallToneMapping(2,0);
      else
	m_pResidualSpecs->InstallToneMapping(2,2);
    }
    if (map3) {
      if (map3 == map0)
	m_pResidualSpecs->InstallToneMapping(3,0);
      else
	m_pResidualSpecs->InstallToneMapping(3,3);
    }

    //
    // Install the quantization matrices for the extension
    // layer if defined. The setup by this code is that
    // 2 and 3 are reserved for this.
    if (hdrquality != MAX_ULONG)
      m_pResidualSpecs->InstallQuantization(2,(colortrafo != JPGFLAG_IMAGE_COLORTRANSFORMATION_NONE)?(3):(2));

    //
    // Enable or disable the hadamard transformation.
    m_pResidualSpecs->InstallHadamardTrafo(hadamard);
    m_pResidualSpecs->InstallNoiseShaping(noiseshaping);

    // Also build an EXIF marker if residual markers are included.
    // This is a bug work-around for eog. Yuck!
    
    if (m_pCameraInfo == NULL)
      m_pCameraInfo = new(m_pEnviron) class EXIFMarker(m_pEnviron);
  }

  if (lossless) {
    m_pLosslessMarker = new(m_pEnviron) LosslessMarker(m_pEnviron);
  }
}
///

/// Tables::WriteTables
// Write the tables to the codestream.
void Tables::WriteTables(class ByteStream *io)
{
  class ToneMappingMarker *tone;
  
  if (m_pCameraInfo) {
    io->PutWord(0xffe1); // APP-1 marker
    m_pCameraInfo->WriteMarker(io);
  }
  
  if (m_pResolutionInfo) {
    io->PutWord(0xffe0); // APP-0 marker
    m_pResolutionInfo->WriteMarker(io);
  }
  
  if (m_pQuant) {
    io->PutWord(0xffdb); // DQT table.
    m_pQuant->WriteMarker(io);
  }
 
  if (m_pRestart) {
    io->PutWord(0xffdd);
    m_pRestart->WriteMarker(io);
  }

  if (m_pThresholds) {
    io->PutWord(0xfff8);
    m_pThresholds->WriteMarker(io);
  }

  if (m_pLSColorTrafo) {
    io->PutWord(0xfff8); // Also using LSE
    m_pLSColorTrafo->WriteMarker(io);
  }

  if (m_pColorInfo) {
    io->PutWord(0xffee); // APP-14 marker
    m_pColorInfo->WriteMarker(io);
  }

  if (m_pLosslessMarker) {
    io->PutWord(0xffe9); // APP9-marker
    m_pLosslessMarker->WriteMarker(io);
  }

  if (m_pResidualSpecs) {
    io->PutWord(0xffe9); // Also APP9
    m_pResidualSpecs->WriteMarker(io);
  }

  for(tone = m_pToneMappingMarker;tone;tone = tone->NextOf()) {
    io->PutWord(0xffe9); // Also APP9
    tone->WriteMarker(io);
  }
}
///

/// Tables::ParseTables
// Parse off tables, including an application marker,
// comment, huffman tables or quantization tables.
// Returns on the first unknown marker.
void Tables::ParseTables(class ByteStream *io)
{
  do {
    LONG marker = io->PeekWord();
    switch(marker) {
    case 0xffdb: // DQT
      io->GetWord();
      if (m_pQuant == NULL)
	m_pQuant = new(m_pEnviron) Quantization(m_pEnviron);
      m_pQuant->ParseMarker(io);
      break;
    case 0xffc4: // DHT 
      io->GetWord();
      if (m_pHuffman == NULL)
	m_pHuffman = new(m_pEnviron) HuffmanTable(m_pEnviron);
      m_pHuffman->ParseMarker(io);
      break;
    case 0xffcc: // DAC
      io->GetWord();
      if (m_pConditioner == NULL)
	m_pConditioner = new(m_pEnviron) class ACTable(m_pEnviron);
      m_pConditioner->ParseMarker(io);
      break;
    case 0xffdd: // DRI
      io->GetWord();
      if (m_pRestart == NULL)
	m_pRestart = new(m_pEnviron) class RestartIntervalMarker(m_pEnviron);
      m_pRestart->ParseMarker(io);
      break;
    case 0xfffe: // COM
      {
	LONG size; 
	io->GetWord();
	size = io->GetWord();
	// Application marker.
	if (size == ByteStream::EOF)
	  JPG_THROW(UNEXPECTED_EOF,"Tables::ParseTables","COM marker incomplete, stream truncated");
	//
	if (size <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","COM marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(size - 2);
      }
      break; 
    case 0xfff8: // LSE: JPEG LS extensions marker.
      {
	io->GetWord();
	LONG len = io->GetWord();
	if (len > 3) {
	  UBYTE id = io->Get();
	  if (id == 1) {
	    // Thresholds marker.
	    if (m_pThresholds == NULL)
	      m_pThresholds = new(m_pEnviron) class Thresholds(m_pEnviron);
	    m_pThresholds->ParseMarker(io,len);
	    break;
	  } else if (id == 2 || id == 3) {
	    JPG_THROW(NOT_IMPLEMENTED,"Tables::ParseTables",
		      "JPEG LS mapping tables are not implemented by this code, sorry");
	  } else if (id == 4) {
	    JPG_THROW(NOT_IMPLEMENTED,"Tables::ParseTables",
		      "JPEG LS size extensions are not implemented by this code, sorry");
	  } else if (id == 0x0d) {
	    // LS Reversible Color transformation
	    if (m_pLSColorTrafo)
	      JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables",
			"found duplicate JPEG LS color transformation specification");
	    m_pLSColorTrafo = new(m_pEnviron) class LSColorTrafo(m_pEnviron);
	    m_pLSColorTrafo->ParseMarker(io,len);
	    break;
	  } else {
	    JPG_WARN(NOT_IMPLEMENTED,"Tables::ParseMarker",
		     "skipping over unknown JPEG LS extensions marker");
	  }
	}
	if (len <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(len - 2);
      }
      break;
    case 0xffe0: // APP0: Maybe the JFIF marker.
      {
	io->GetWord();
	LONG len = io->GetWord();
	if (len >= 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1) { 
	  const char *id = "JFIF";
	  while(*id) {
	    len--;
	    if (io->Get() != *id)
	      break;
	    id++;
	  }
	  if (*id == 0) {
	    len--;
	    if (io->Get() == 0) {
	      if (m_pResolutionInfo == NULL)
		m_pResolutionInfo = new(m_pEnviron) class JFIFMarker(m_pEnviron);
	      m_pResolutionInfo->ParseMarker(io,len + 5);
	      break;
	    }
	  }
	}
	if (len <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(len - 2);
      }
      break;  
    case 0xffe1: // APP1: Maybe the EXIF marker.
      {
	io->GetWord();
	LONG len = io->GetWord();
	if (len >= 2 + 4 + 2 + 2 + 2 + 4 + 2) { 
	  const char *id = "Exif";
	  while(*id) {
	    len--;
	    if (io->Get() != *id)
	      break;
	    id++;
	  }
	  if (*id == 0) {
	    len -= 2;
	    if (io->GetWord() == 0) {
	      if (m_pCameraInfo == NULL)
		m_pCameraInfo = new(m_pEnviron) class EXIFMarker(m_pEnviron);
	      m_pCameraInfo->ParseMarker(io,len + 4 + 2);
	      break;
	    }
	  }
	}
	if (len <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(len - 2);
      }
      break;
    case 0xffe9: // APP9:  Application markers defined here.
      {
	UWORD len;
	io->GetWord();
	len = io->GetWord();
	if (len > 2 + 2 + 4) {
	  UWORD id = io->GetWord();
	  if (id == (('J' << 8) | ('P'))) {
	    LONG type = io->GetWord() << 16;
	    type |= io->GetWord();
	    if (type == JPG_MAKEID('S','E','R','M')) {
	      if (m_pLosslessMarker == NULL)
		m_pLosslessMarker = new(m_pEnviron) class LosslessMarker(m_pEnviron);
	      m_pLosslessMarker->ParseMarker(io,len);
	      if (m_bForceFixpoint) {
		delete m_pLosslessMarker;m_pLosslessMarker = NULL;
	      }
	      break;
	    } else if (type == JPG_MAKEID('R','E','S','I')) {
	      if (m_pResidualData == NULL)
		m_pResidualData = new(m_pEnviron) class ResidualMarker(m_pEnviron,
								       ResidualMarker::Residual);
	      //
	      // Parse off the residual data.
	      m_pResidualData->ParseMarker(io,len);
	      break;
	    } else if (type == JPG_MAKEID('F','I','N','E')) {
	      if (m_pRefinementData == NULL)
		m_pRefinementData = new(m_pEnviron) class ResidualMarker(m_pEnviron,
									 ResidualMarker::Refinement);
	      //
	      m_pRefinementData->ParseMarker(io,len);
	      break;
	    } else if (type == JPG_MAKEID('S','P','E','C')) {
	      if (m_pResidualSpecs == NULL)
		m_pResidualSpecs = new(m_pEnviron) class ResidualSpecsMarker(m_pEnviron);
	      //
	      // Parse off the residual specifications.
	      m_pResidualSpecs->ParseMarker(io,len);
	      break;
	    } else if (type == JPG_MAKEID('T','O','N','E')) {
	      class ToneMappingMarker *tone = new(m_pEnviron) class ToneMappingMarker(m_pEnviron);
	      //
	      // Append to the end of the list.
	      tone->NextOf()       = m_pToneMappingMarker;
	      m_pToneMappingMarker = tone;
	      //
	      // Parse the stuff off.
	      tone->ParseMarker(io,len);
	      break;
	    }
	    len -= 2 + 4; // bytes already read.
	  } else {
	    len -= 2;
	  }
	}
	if (len <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(len - 2);
      }
      break;
    case 0xffee: // APP14: Maybe the adobe marker.
      {
	io->GetWord();
	LONG len = io->GetWord();
	if (len == 2 + 5 + 2 + 2 + 2 + 1) { 
	  const char *id = "Adobe";
	  while(*id) {
	    len--;
	    if (io->Get() != *id)
	      break;
	    id++;
	  }
	  if (*id == 0) {
	    if (m_pColorInfo == NULL)
	      m_pColorInfo = new(m_pEnviron) class AdobeMarker(m_pEnviron);
	    m_pColorInfo->ParseMarker(io,len + 5);
	    break;
	  }
	}
	if (len <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(len - 2);
      }
      break;
    case 0xffc0:
    case 0xffc1:
    case 0xffc2:
    case 0xffc3:
    case 0xffc5:
    case 0xffc6:
    case 0xffc7:
    case 0xffc8:
    case 0xffc9:
    case 0xffca:
    case 0xffcb:
    case 0xffcd:
    case 0xffce:
    case 0xffcf: // all start of frame markers.
    case 0xffda: // Start of scan.
    case 0xffde: // DHP
    case 0xfff7: // JPEG LS SOS
      return;
    case 0xffff: // A filler byte followed by a marker. Skip.
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
      JPG_WARN(MALFORMED_STREAM,"Tables::ParseTables","found a stray restart marker segment, ignoring");
      break;
    default: 
      if (marker >= 0xffc0 && (marker < 0xffd0 || marker >= 0xffd8) && marker < 0xfff0) {
	LONG size;
	io->GetWord();
	size = io->GetWord();
	// Application marker.
	if (size == ByteStream::EOF)
	  JPG_THROW(UNEXPECTED_EOF,"Tables::ParseTables","marker incomplete, stream truncated");
	//
	if (size <= 0x02)
	  JPG_THROW(MALFORMED_STREAM,"Tables::ParseTables","marker size out of range");
	//
	// Just skip the contents. For now. More later on.
	io->SkipBytes(size - 2);
      } else {
	LONG dt;
	//
	JPG_WARN(MALFORMED_STREAM,"Tables::ParseTables",
		 "found invalid marker, probably a marker size is out of range");
	// Advance to the next marker manually.
	io->Get();
	do {
	  dt = io->Get();
	} while(dt != 0xff && dt != ByteStream::EOF);
	//
	if (dt == 0xff) {
	  io->LastUnDo();
	} else {
	  return;
	}
      }
    }
  } while(true);
}
///

/// Tables::FindDCHuffmanTable
// Find the DC huffman table of the indicated index.
class HuffmanTemplate *Tables::FindDCHuffmanTable(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden,bool residual) const
{
  class HuffmanTemplate *t;

  if (m_pHuffman == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindDCHuffmanTable","DHT marker missing for huffman encoded scan");

  t = m_pHuffman->DCTemplateOf(idx,type,depth,hidden,residual);
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindDCHuffmanTable","requested DC huffman coding table not defined");
  return t;
}
///

/// Tables::FindACHuffmanTable
// Find the AC huffman table of the indicated index.
class HuffmanTemplate *Tables::FindACHuffmanTable(UBYTE idx,ScanType type,UBYTE depth,UBYTE hidden,bool residual) const
{ 
  class HuffmanTemplate *t;

  if (m_pHuffman == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindACHuffmanTable","DHT marker missing for huffman encoded scan");

  t = m_pHuffman->ACTemplateOf(idx,type,depth,hidden,residual);
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindACHuffmanTable","requested AC huffman coding table not defined");
  return t;
}
///

/// Tables::FindDCConditioner
class ACTemplate *Tables::FindDCConditioner(UBYTE idx) const
{
  if (m_pConditioner) {
    return m_pConditioner->DCTemplateOf(idx);
  }

  return NULL;
}
///

/// Tables::FindACConditioner
class ACTemplate *Tables::FindACConditioner(UBYTE idx) const
{
  if (m_pConditioner) {
    return m_pConditioner->ACTemplateOf(idx);
  }

  return NULL;
}
///

/// Tables::FindQuantizationTable
// Find the quantization table of the given index.
const UWORD *Tables::FindQuantizationTable(UBYTE idx) const
{
  const UWORD *t;
  
  if (m_pQuant == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindQuantizationTable","DQT marker missing, no quantization table defined");

  t = m_pQuant->QuantizationTable(idx);
  if (t == NULL)
    JPG_THROW(OBJECT_DOESNT_EXIST,"Tables::FindQuantizationTable","requested quantization matrix not defined");
  return t;
}
///

/// Tables::ResidualDataOf
// Return the residual data data if any.
class ResidualMarker *Tables::ResidualDataOf(void) const
{
  return m_pResidualData;
}
///

/// Tables::RefinementDataOf
// Return the refinement data if any.
class ResidualMarker *Tables::RefinementDataOf(void) const
{
  return m_pRefinementData;
}
///

/// Tables::ResidualSpecsOf
// Return the information on the residual marker data if any.
class ResidualSpecsMarker *Tables::ResidualSpecsOf(void) const
{
  return m_pResidualSpecs;
}
///

/// Tables::FindEncodingToneMappingCurve
// Find the tone mapping curve for encoding that performs the HDR to LDR mapping
const UWORD *Tables::FindEncodingToneMappingCurve(UBYTE comp,UBYTE ldrbpp)
{
  UBYTE preshift = 0; // if present.

  assert(comp < 4);

  if (m_pResidualSpecs) {
    if (m_pResidualSpecs->isToneMapped(comp)) {
      class ToneMappingMarker *tone;
      UBYTE tabidx = m_pResidualSpecs->ToneMappingTableOf(comp);

      for(tone = m_pToneMappingMarker;tone;tone = tone->NextOf()) {
	if (tone->IndexOf() == tabidx)
	  break;
      }
      if (tone == NULL) 
	JPG_THROW(MALFORMED_STREAM,"Tables::FindEncodingToneMappingCurve",
		  "the stream requested a tone mapping table not present in the data");

      return tone->EncodingCurveOf();
    } else {
      preshift = m_pResidualSpecs->PointPreShiftOf();
    }
  }

  if (m_pusInverseMapping == NULL) {
    int i;
    int size  = (1 << (ldrbpp + preshift));
    int round = (1 << preshift) >> 1;

    m_pusInverseMapping = (UWORD *)m_pEnviron->AllocVec(size * sizeof(UWORD));

    for(i = 0;i < size;i++) {
      m_pusInverseMapping[i] = (i + round) >> preshift;
    }
  }

  return m_pusInverseMapping;
}
///

/// Tables::FindDecodingToneMappingCurve
// Find the tone mapping curve for decoding that runs the LDR to HDR mapping.
const UWORD *Tables::FindDecodingToneMappingCurve(UBYTE comp,UBYTE ldrbpp)
{
  UBYTE preshift = 0; // if present.

  assert(comp < 4);

  if (m_pResidualSpecs) {
    if (m_pResidualSpecs->isToneMapped(comp)) {
      class ToneMappingMarker *tone;
      UBYTE tabidx = m_pResidualSpecs->ToneMappingTableOf(comp);

      for(tone = m_pToneMappingMarker;tone;tone = tone->NextOf()) {
	if (tone->IndexOf() == tabidx)
	  break;
      }
      if (tone == NULL) 
	JPG_THROW(MALFORMED_STREAM,"Tables::FindDecodingToneMappingCurve",
		  "the stream requested a tone mapping table not present in the data");

      return tone->ToneMappingCurveOf();
    } else {
      preshift = m_pResidualSpecs->PointPreShiftOf();
    }
  }

  if (m_pusToneMapping == NULL) {
    int i,size = (1 << ldrbpp);
    m_pusToneMapping = (UWORD *)m_pEnviron->AllocVec(size * sizeof(UWORD));

    for(i = 0;i < size;i++) {
      m_pusToneMapping[i] = i << preshift;
    }
  }

  return m_pusToneMapping;
}
///

/// Tables::ColorTrafoOf
// Return the color transformer.
class ColorTrafo *Tables::ColorTrafoOf(class Frame *frame,UBYTE type,UBYTE bpp,UBYTE count,bool encoding)
{
  if (m_pColorTrafo) // FIXME: Check whether the type fits.
    return m_pColorTrafo;

  if (count == 1 || m_bDisableColor ||
      (m_pColorInfo && 
       m_pColorInfo->EnumeratedColorSpaceOf() == AdobeMarker::None)) {
    switch(count) {
    case 1:
      switch(type) {
      case CTYP_UBYTE:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UBYTE,1>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UWORD,1>(m_pEnviron);
	break;
      }
      break;
    case 2:
      switch(type) {
      case CTYP_UBYTE:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UBYTE,2>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UWORD,2>(m_pEnviron);
	break;
      }
      break;
    case 3:
      switch(type) {
      case CTYP_UBYTE:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UBYTE,3>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UWORD,3>(m_pEnviron);
	break;
      }
      break;
    case 4:
      switch(type) {
      case CTYP_UBYTE:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UBYTE,4>(m_pEnviron);
	break;
      case CTYP_UWORD:
	m_pColorTrafo = new(m_pEnviron) class TrivialTrafo<UWORD,4>(m_pEnviron);
	break;
      }
      break;
    }
  } else {
    if (m_pLSColorTrafo) {
      switch(count) {
      case 3:
	switch(type) {
	case CTYP_UBYTE:
	  {
	    LSLosslessTrafo<UBYTE,3> *ls = new(m_pEnviron) class LSLosslessTrafo<UBYTE,3>(m_pEnviron);
	    ls->InstallMarker(m_pLSColorTrafo,frame);
	    m_pColorTrafo = ls;
	  }
	  break;
	case CTYP_UWORD:
	  {
	    LSLosslessTrafo<UWORD,3> *ls = new(m_pEnviron) class LSLosslessTrafo<UWORD,3>(m_pEnviron);
	    ls->InstallMarker(m_pLSColorTrafo,frame);
	    m_pColorTrafo = ls;
	  }
	  break;
	}
	break;
     case 4:
       switch(type) {
       case CTYP_UBYTE:
	 {
	   LSLosslessTrafo<UBYTE,4> *ls = new(m_pEnviron) class LSLosslessTrafo<UBYTE,4>(m_pEnviron);
	   ls->InstallMarker(m_pLSColorTrafo,frame);
	   m_pColorTrafo = ls;
	 }
	 break;
       case CTYP_UWORD:
	 {
	   LSLosslessTrafo<UWORD,4> *ls = new(m_pEnviron) class LSLosslessTrafo<UWORD,4>(m_pEnviron);
	   ls->InstallMarker(m_pLSColorTrafo,frame);
	   m_pColorTrafo = ls;
	 }
	 break;
       }
       break;
      }
    } else {
      switch(count) {
      case 3:
	switch(type) {
	case CTYP_UBYTE:
	  m_pColorTrafo = new(m_pEnviron) class YCbCrTrafo<UBYTE,3>(m_pEnviron);
	  break;
	case CTYP_UWORD:
	  m_pColorTrafo = new(m_pEnviron) class YCbCrTrafo<UWORD,3>(m_pEnviron);
	  break;
	}
	break;
      case 4:
	switch(type) {
	case CTYP_UBYTE:
	  m_pColorTrafo = new(m_pEnviron) class YCbCrTrafo<UBYTE,4>(m_pEnviron);
	  break;
	case CTYP_UWORD:
	  m_pColorTrafo = new(m_pEnviron) class YCbCrTrafo<UWORD,4>(m_pEnviron);
	  break;
	}
	break;
      }
    }
  }

  if (m_pColorTrafo) {
    UBYTE i; 
    //
    // Find the tone mapping curves.
    if (encoding) {
      const UWORD *encode[4] = {NULL,NULL,NULL,NULL};
      for(i = 0;i < count;i++) {
	encode[i] = FindEncodingToneMappingCurve(i,bpp);
      }
      m_pColorTrafo->DefineEncodingTables(encode);
    }
    //
    // Always define the decoding table since residual coding
    // will require it in either case, encoding and decoding.
    {
      const UWORD *decode[4] = {NULL,NULL,NULL,NULL};
       for(i = 0;i < count;i++) {
	 decode[i] = FindDecodingToneMappingCurve(i,bpp);
       }
       m_pColorTrafo->DefineDecodingTables(decode);
    }
    
    return m_pColorTrafo;
  } else {
    JPG_THROW(INVALID_PARAMETER,"Tables::ColorTrafoOf","no transformation for the given parameters available");
    return NULL;
  }
}
///


/// Tables::HiddenDCTBitsOf
// Check how many bits are hidden in invisible refinement scans.
UBYTE Tables::HiddenDCTBitsOf(void) const
{
  if (m_pResidualSpecs)
    return m_pResidualSpecs->HiddenBitsOf();

  return 0;
}
///

/// Tables::UseResiduals
// Check whether residual data in the APP9 marker shall be written.
bool Tables::UseResiduals(void) const
{
  if (m_pResidualData)
    return true;
  return false;
}
///

/// Tables::UseRefinements
// Check whether refinement data shall be written.
bool Tables::UseRefinements(void) const
{
  if (m_pRefinementData)
    return true;
  return false;
}
///

/// Tables::UseReversibleDCT
// Check whether the integer reversible SERMS based DCT shall be used.
bool Tables::UseReversibleDCT(void) const
{  
  if (m_pLosslessMarker)
    return true;
  return false;
}
///

/// Tables::FractionalColorBitsOf
// Check how many fractional bits the color transformation will use.
UBYTE Tables::FractionalColorBitsOf(UBYTE count) const
{ 
  if (count == 1 || m_bDisableColor || m_pLSColorTrafo ||
      (m_pColorInfo && 
       m_pColorInfo->EnumeratedColorSpaceOf() == AdobeMarker::None)) {
    return 0; // No fractional bits needed.
  } else {
    return ColorTrafo::COLOR_BITS;
  }
}
///

/// Tables::UseColortrafo
// Check whether color tranformation is enabled.
bool Tables::UseColortrafo(void) const
{  
  if (m_bDisableColor || (m_pColorInfo && 
			  m_pColorInfo->EnumeratedColorSpaceOf() == AdobeMarker::None)) {
    return false;
  } else {
    return true;
  }
}
///

/// Tables::ForceIntegerCodec
// Force to use the integer DCT even if the headers say otherwise.
void Tables::ForceIntegerCodec(void)
{
  if (m_pLosslessMarker == NULL)
    m_pLosslessMarker = new(m_pEnviron) class LosslessMarker(m_pEnviron);
}
///

/// Tables::ForceFixpointCodec
// Force to use the fixpoint codec even if the headers say otherwise.
void Tables::ForceFixpointCodec(void)
{
  delete m_pLosslessMarker;m_pLosslessMarker = NULL;
  m_bForceFixpoint = true;
}
///
 
/// Tables::ForceColorTrafoOff
// Disable the color transformation even in the absense of the Adobe marker.
void Tables::ForceColorTrafoOff(void)
{
  m_bDisableColor = true;
}
///

/// Tables::BuildDCT
// Build the proper DCT transformation for the specification
// recorded in this class.
class DCT *Tables::BuildDCT(UBYTE quantizer,UBYTE count) const
{
  bool reversible  = UseReversibleDCT();
  UBYTE fractional = FractionalColorBitsOf(count);
  class DCT *dct   = NULL;
  const UWORD *quant;

  quant = FindQuantizationTable(quantizer);

  if (reversible) {
    if (fractional == 0)
      dct = new(m_pEnviron) class SERMSDCT(m_pEnviron);
  } else {
    if (fractional == 0) 
      dct = new(m_pEnviron) class IDCT<0>(m_pEnviron);
    else if (fractional == ColorTrafo::COLOR_BITS)
      dct =new(m_pEnviron) class IDCT<ColorTrafo::COLOR_BITS>(m_pEnviron);
  }

  if (dct == NULL) {
    JPG_THROW(INVALID_PARAMETER,"BlockCtrl::DCTOf","unsupported DCT requested");
  } else {
    dct->DefineQuant(quant); // does not throw
  }

  return dct;
}
///

/// Tables::RestartIntervalOf
// Return the currently active restart interval in MCUs or zero
// in case restart markers are disabled.
UWORD Tables::RestartIntervalOf(void) const
{
  if (m_pRestart)
    return m_pRestart->RestartIntervalOf();
  return 0;
}
///
