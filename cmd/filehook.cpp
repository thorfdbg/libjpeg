/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** $Id: filehook.cpp,v 1.2 2014/09/30 08:33:15 thor Exp $
**
*/


/// Includes
#include "cmd/filehook.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"
#include "interface/parameters.hpp"
#include "std/assert.hpp"
#include <algorithm>
///

HookDataAccessor::~HookDataAccessor()
{
}

FileHookDataAccessor::FileHookDataAccessor(FILE *file): m_file(file)
{
}

JPG_LONG FileHookDataAccessor::write(JPG_CPTR data, JPG_LONG size)
{
  return fwrite(data, 1, size, m_file);
}

JPG_LONG FileHookDataAccessor::read(JPG_APTR destination, JPG_LONG size)
{
  return fread(destination, 1, size, m_file);
}

JPG_LONG FileHookDataAccessor::seek(JPG_LONG offset, JPG_LONG origin)
{
  return fseek(m_file, offset, origin);
}

UserDataHookAccessor::UserDataHookAccessor(JPG_APTR data, JPG_LONG dataSize):
  m_data(reinterpret_cast<UBYTE*>(data)), m_dataSize(dataSize), m_curPosition(0)
{
}

JPG_LONG UserDataHookAccessor::read(JPG_APTR destination, JPG_LONG size)
{
  const JPG_LONG bytesNumberToCopy = std::min(size, m_dataSize - m_curPosition);
  memcpy(destination, m_data + m_curPosition, bytesNumberToCopy);
  m_curPosition += bytesNumberToCopy;
  return bytesNumberToCopy;
}

JPG_LONG UserDataHookAccessor::write(JPG_CPTR data, JPG_LONG size)
{
  const JPG_LONG bytesNumberToCopy = std::min(size, m_dataSize - m_curPosition);
  memcpy(m_data + m_curPosition, data, bytesNumberToCopy);
  m_curPosition += bytesNumberToCopy;
  return bytesNumberToCopy;
}

JPG_LONG UserDataHookAccessor::seek(JPG_LONG offset, JPG_LONG origin)
{
  switch(origin) {
    case SEEK_CUR:
      m_curPosition += offset;
      break;
    case SEEK_SET:
      m_curPosition = offset;
      break;
    case SEEK_END:
      m_curPosition = m_dataSize - offset;
      break;
  }

  return 0;
}

/// The IO hook function
JPG_LONG FileHook(struct JPG_Hook *hook, struct JPG_TagItem *tags)
{
  HookDataAccessor *in = (HookDataAccessor *)(hook->hk_pData);

  switch(tags->GetTagData(JPGTAG_FIO_ACTION)) {
  case JPGFLAG_ACTION_READ:
    {
      UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
      ULONG  size   = (ULONG  )tags->GetTagData(JPGTAG_FIO_SIZE);

      return in->read(buffer, size);
    }
  case JPGFLAG_ACTION_WRITE:
    {
      UBYTE *buffer = (UBYTE *)tags->GetTagPtr(JPGTAG_FIO_BUFFER);
      ULONG  size   = (ULONG  )tags->GetTagData(JPGTAG_FIO_SIZE);

      return in->write(buffer, size);
    }
  case JPGFLAG_ACTION_SEEK:
    {
      LONG mode   = tags->GetTagData(JPGTAG_FIO_SEEKMODE);
      LONG offset = tags->GetTagData(JPGTAG_FIO_OFFSET);

      switch(mode) {
      case JPGFLAG_OFFSET_CURRENT:
        return in->seek(offset, SEEK_CUR);
      case JPGFLAG_OFFSET_BEGINNING:
        return in->seek(offset, SEEK_SET);
      case JPGFLAG_OFFSET_END:
        return in->seek(offset, SEEK_END);
      }
    }
  case JPGFLAG_ACTION_QUERY:
    return 0;
  }
  return -1;
}
///
