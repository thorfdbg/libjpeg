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
** $Id: filehook.hpp,v 1.2 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CMD_FILEHOOK_HPP
#define CMD_FILEHOOK_HPP

/// Includes
#include "interface/types.hpp"
#include "std/stdio.hpp"
#include <cstring>
///

/// Forwards
struct JPG_Hook;
struct JPG_TagItem;
///

struct HookDataAccessor
{
  virtual JPG_LONG read(JPG_APTR destination, JPG_LONG size) = 0;
  virtual JPG_LONG write(JPG_CPTR data, JPG_LONG size) = 0;
  virtual JPG_LONG seek(JPG_LONG offset, JPG_LONG origin) = 0;
  virtual ~HookDataAccessor();
};

struct FileHookDataAccessor: public HookDataAccessor
{
  FileHookDataAccessor(FILE *file);

  virtual JPG_LONG read(JPG_APTR destination, JPG_LONG size);
  virtual JPG_LONG write(JPG_CPTR data, JPG_LONG size);
  virtual JPG_LONG seek(JPG_LONG offset, JPG_LONG origin);

private:
  FILE *m_file;
};

struct UserDataHookAccessor: public HookDataAccessor
{
  UserDataHookAccessor(JPG_APTR data, JPG_LONG dataSize);

  virtual JPG_LONG read(JPG_APTR destination, JPG_LONG size);
  virtual JPG_LONG write(JPG_CPTR data, JPG_LONG size);
  virtual JPG_LONG seek(JPG_LONG offset, JPG_LONG origin);

private:
  UBYTE* m_data;
  JPG_LONG m_dataSize;
  JPG_LONG m_curPosition;
};

/// Prototypes
extern JPG_LONG FileHook(struct JPG_Hook *hook, struct JPG_TagItem *tags);
///

///
#endif
