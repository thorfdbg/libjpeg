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
** This module implements the IO-Hook function that reads and writes
** the encoded data.
**
** $Id: filehook.cpp,v 1.3 2020/04/08 10:05:38 thor Exp $
**
*/


/// Includes
#include "cmd/filehook.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "std/stdio.hpp"
///

/// The IO hook function
JPG_LONG FileHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
  FILE *in = (FILE *)(hook->hk_pData);

  switch(tags->GetTagData(JPGTAG_FIO_ACTION)) {
  case JPGFLAG_ACTION_READ:
    {
      UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
      ULONG  size   = (ULONG  )tags->GetTagData(JPGTAG_FIO_SIZE);
      
      return fread(buffer,1,size,in);
    }
    break;
  case JPGFLAG_ACTION_WRITE:
    {
      UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
      ULONG  size   = (ULONG  )tags->GetTagData(JPGTAG_FIO_SIZE);
      
      return fwrite(buffer,1,size,in);
    }
    break;
  case JPGFLAG_ACTION_SEEK:
    {
      LONG mode   = tags->GetTagData(JPGTAG_FIO_SEEKMODE);
      LONG offset = tags->GetTagData(JPGTAG_FIO_OFFSET);

      switch(mode) {
      case JPGFLAG_OFFSET_CURRENT:
        return fseek(in,offset,SEEK_CUR);
      case JPGFLAG_OFFSET_BEGINNING:
        return fseek(in,offset,SEEK_SET);
      case JPGFLAG_OFFSET_END:
        return fseek(in,offset,SEEK_END);
      }
    }
    break;
  case JPGFLAG_ACTION_QUERY:
    return 0;
  }
  return -1;
}
///
