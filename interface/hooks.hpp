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
 * Customizable hooks.
 * 
 * $Id: hooks.hpp,v 1.3 2012-06-02 10:27:14 thor Exp $
 *
 * Hooks provide a standard method to supply call-out functions.
 * They are initialized by the client application and called by
 * the library. They pass data in terms of tag items.
 * They also pass client private data.
 *
 * This is part of the external interface of the jpeg
 * and visible to the outher world.
 */

#ifndef HOOKS_HPP
#define HOOKS_HPP

/// Includes
#include "jpgtypes.hpp"
#include "tagitem.hpp"
///

///
#ifndef JPG_EXPORT
#define JPG_EXPORT
#endif
///

/// Design
/** Design
******************************************************************
** struct J2k_Hook						**
** Super Class:	none						**
** Sub Classes: none						**
** Friends:							**
******************************************************************

The hook structure defines a generic call-back hook. This allows
the user to "hook" into the library and to get called on certain
events. The j2k lib uses this structure for all its call-back
functions.

A hook defines first an entry point to be called. This callback
function takes a pointer to the hook structure and a tag list
as arguments. Tag lists are discussed in a separate file and
provide enough flexibility to extend options.

The callback function can be one of two kinds, it may either
return a LONG (as a result code) or a generic pointer. The
library uses currently both types, see the documentation which
hook uses which.

Furthermore, hooks provide a "sub entry" point. This is not
used by the library at all, but you may use it. A typical
application would be to let the main entry point point to a
small "stub" function which loads registers, base pointers etc.
and let the "sub entry" point to the real thing.

Last but not least, a hook contains a user data field you may
use for whatever you feel like. Typically, this would be either
a base register, or a pointer to the corresponding base object
the hook is part of. 

In a C++ environment, a hook entry would consist of a static
member function of your object, which would forward hook calls
to the non-static methods of the object.
* */
///

/// Hook structure
// this is a function as it is called from the library
// It is called with the hook as one parameter such that the 
// client is able to extract its private data. 
// It also comes with a tag item parameter to pass data.

struct JPG_EXPORT JPG_Hook {
  // The callbacks.
#ifdef __cplusplus  
  typedef JPG_LONG (LongHookFunction)(struct JPG_Hook *, struct JPG_TagItem *tag);
  typedef JPG_APTR (APtrHookFunction)(struct JPG_Hook *, struct JPG_TagItem *tag);
#endif
  //
  union JPG_EXPORT HookCallOut {
    JPG_LONG (*hk_pLongEntry)(struct JPG_Hook *, struct JPG_TagItem *tag);      // the main entry point.
    JPG_APTR (*hk_pAPtrEntry)(struct JPG_Hook *, struct JPG_TagItem *tag);
    //
#ifdef __cplusplus
    // Constructors
    HookCallOut(LongHookFunction *hook)
      : hk_pLongEntry(hook)
    { }
    //
    HookCallOut(APtrHookFunction *hook)
      : hk_pAPtrEntry(hook)
    { }
    //
    HookCallOut(void)
      : hk_pLongEntry(0)
    { }
#endif
    //
  }                   hk_Entry,hk_SubEntry;
  //
  // hk_SubEntry
  // can be used by the application to forward the request
  // and to load the private data below in whatever register it needs.
  // Is otherwise not used by the application.
  //
  JPG_APTR hk_pData;  // for private use of the client.
  //
#ifdef __cplusplus
  // Constructors
  JPG_Hook(LongHookFunction *hook = 0, JPG_APTR data = 0)
    : hk_Entry(hook), hk_pData(data)
  { }
  //  
  JPG_Hook(APtrHookFunction *hook, JPG_APTR data = 0)
    : hk_Entry(hook), hk_pData(data)
  { }
  //
  //
  JPG_LONG CallLong(struct JPG_TagItem *tag)
  {
    return (*hk_Entry.hk_pLongEntry)(this,tag);
  }
  //
  JPG_APTR CallAPtr(struct JPG_TagItem *tag)
  {
    return (*hk_Entry.hk_pAPtrEntry)(this,tag);
  }
#endif
  //
};
///

///
#endif
