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
** This class parses the markers and holds the decoder together.
**
** $Id: encoder.cpp,v 1.26 2012-07-26 19:17:35 thor Exp $
**
*/

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "codestream/encoder.hpp"
#include "io/bytestream.hpp"
#include "codestream/tables.hpp"
#include "codestream/image.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "std/assert.hpp"
///

/// Encoder::Encoder
// Create the default encoder
Encoder::Encoder(class Environ *env)
  : JKeeper(env), m_pImage(NULL), m_pTables(NULL)
{
}
///

/// Encoder::~Encoder
// Delete the encoder
Encoder::~Encoder(void)
{
  delete m_pImage;
  delete m_pTables;
}
///


/// Encoder::CreateImage
// Create an image from the layout specified in the tags. See interface/parameters
// for the available tags.
class Image *Encoder::CreateImage(const struct JPG_TagItem *tags)
{
  /*
// Encoder::CreateDefaultImage
// Install frame defaults for a sequential scan.
class Image *Encoder::CreateDefaultImage(ULONG width,ULONG height,UBYTE depth,UBYTE precision,
					 ScanType scantype,UBYTE quality,UBYTE maxerror,
					 bool colortrafo,bool reversible,bool residual,
					 bool dconly,UBYTE levels,bool scale,
					 bool writednl,UWORD restart,
					 const UBYTE *subx,const UBYTE *suby)
{
  */
  ScanType scantype;
  LONG frametype  = tags->GetTagData(JPGTAG_IMAGE_FRAMETYPE);
  ULONG width     = tags->GetTagData(JPGTAG_IMAGE_WIDTH);
  ULONG height    = tags->GetTagData(JPGTAG_IMAGE_HEIGHT);
  ULONG depth     = tags->GetTagData(JPGTAG_IMAGE_DEPTH,3);
  ULONG precision = tags->GetTagData(JPGTAG_IMAGE_PRECISION,8);
  ULONG maxerror  = tags->GetTagData(JPGTAG_IMAGE_ERRORBOUND);
  bool accoding   = (frametype & JPGFLAG_ARITHMETIC)?true:false;
  bool scale      = (frametype & JPGFLAG_PYRAMIDAL)?true:false;
  bool residual   = (frametype & JPGFLAG_RESIDUAL_CODING)?true:false;
  bool writednl   = tags->GetTagData(JPGTAG_IMAGE_WRITE_DNL);
  ULONG restart   = tags->GetTagData(JPGTAG_IMAGE_RESTART_INTERVAL);
  ULONG levels    = tags->GetTagData(JPGTAG_IMAGE_RESOLUTIONLEVELS);
  const UBYTE *subx = (UBYTE *)tags->GetTagPtr(JPGTAG_IMAGE_SUBX);
  const UBYTE *suby = (UBYTE *)tags->GetTagPtr(JPGTAG_IMAGE_SUBY);

  if (depth > 256)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","image depth can be at most 256");
  
  if (precision < 1 || precision > 16)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","image precision must be between 1 and 16");

  if (levels > 32)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","number of resolution levels must be between 0 and 32");

  if (restart > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::CreateImage","restart interval must be between 0 and 65535");

  switch(frametype & 0x07) {
  case JPGFLAG_BASELINE:
    scantype = Baseline;
    if (accoding)
      JPG_THROW(INVALID_PARAMETER,"Encoder::CreateImage","baseline coding does not allow arithmetic coding");
    break;
  case JPGFLAG_SEQUENTIAL:
    scantype = Sequential;
    if (accoding)
      scantype = ACSequential;
    break;
  case JPGFLAG_PROGRESSIVE:
    scantype = Progressive;
    if (accoding)
      scantype = ACProgressive;
    break;
  case JPGFLAG_LOSSLESS:
    scantype = Lossless;
    if (accoding)
      scantype = ACLossless;
    break;
  case JPGFLAG_JPEG_LS:
    scantype = JPEG_LS;
    break; // Could enable AC coding, maybe? In future revisions.
  default:
    JPG_THROW(INVALID_PARAMETER,"Encoder::CreateImage","specified invalid frame type");
  }
  
  if (maxerror > 255)
    JPG_THROW(OVERFLOW_PARAMETER,"Encoder::WriteHeader","the maximum error must be between 0 and 255");

  if (m_pImage || m_pTables)
    JPG_THROW(OBJECT_EXISTS,"Encoder::CreateImage","the image is already initialized");

  m_pTables = new(m_pEnviron) class Tables(m_pEnviron);

  m_pTables->InstallDefaultTables(tags);

  if (residual) {
    switch(frametype & 0x07) {
    case JPGFLAG_BASELINE:
    case JPGFLAG_SEQUENTIAL:
    case JPGFLAG_PROGRESSIVE:
      if (precision > 8) {
	precision = 8;
      }
      break;
    }
  }
  
  // Do not indicate baseline here, though it may...
  m_pImage  = new(m_pEnviron) class Image(m_pTables);
  m_pImage->InstallDefaultParameters(width,height,depth,
				     precision,scantype,levels,scale,
				     writednl,subx,suby,tags);

  return m_pImage;
}
///
