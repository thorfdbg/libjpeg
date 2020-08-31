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
 * Tag item definitions
 * 
 * $Id: tagitem.hpp,v 1.9 2014/09/30 08:33:17 thor Exp $
 *
 * Tag items provide a convenient mechanism to build functions with are
 * easely extendable. A tag item consists of:
 *
 * A tag field that identifies the parameter type,
 * the parameter itself.
 *
 * Special system parameters exist to link, filter, etc... tag items.
 *
 * This is part of the external interface of the jpeg codec.
 *
 */

#ifndef TAGITEM_HPP
#define TAGITEM_HPP

/// Includes
#include "jpgtypes.hpp"
///

/// Cut markers for the perl script
#ifndef JPG_EXPORT
#define JPG_EXPORT
#endif
///

/// Tag itentifiers and system tag ids
typedef JPG_ULONG JPG_Tag;
// Tag identifier

// And now for the system (control) tag definitions:

#define JPGTAG_TAG_DONE   (0L)
// terminates array of TagItems. ti_Data unused
#define JPGTAG_TAG_END    (0L)
// synonym for JPGTAG_TAG_DONE                         
#define JPGTAG_TAG_IGNORE (1L)
// ignore this item, not end of array
#define JPGTAG_TAG_MORE   (2L)
// ti_Ptr is pointer to another array of TagItems
// note that this tag terminates the current array
//
#define JPGTAG_TAG_SKIP   (3L)
// skip this and the next ti_Data items 

// differentiates user tags from control tags:
// All user (yes, that's you!) tag items you may define
// must have the following bit set:

#define JPGTAG_TAG_USER   (((JPG_ULONG)1)<<31)

// If the TAG_USER bit is set in a tag number, it tells the library that
// the tag is not a control tag (like JPGTAG_TAG_DONE, TAG_IGNORE, TAG_MORE) and is
// instead an application tag. "USER" means a client of the tag routines in
// general, including the "other" j2k library routines. It is not related to
// whether the function is external to the lib.

// Reserved for library internal use.
#define JPGTAG_SET (((JPG_ULONG)1)<<30)
///

/// Design
/** Design
******************************************************************
** struct JPG_TagItem                                           **
** Super Class: none                                            **
** Sub Classes: none                                            **
** Friends:                                                     **
******************************************************************

TagItems define flexible and extensible methods to pass variable
arguments to functions. It is used by the j2k lib to communicate 
with the client application. Input parameters for decoder and
encoder are passed in by using tag lists, and arguments for the
call-back hooks are again passed out by tag lists.
A tag list is simply an array of TagItems.

Tag items consist of a tag identifier, defining the type of
information delivered, and the tag data, containing the 
delivered information itself. The tag data can be either a LONG
or an APTR (generic pointer). It is the matter of the tag
parser to identify the proper type from the tag id and to
select the proper type for the tag data.

Tag ids can be selected freely as long as the TAG_USER bit
(bit 31, that is) is set in the tag id. All tags below are
reserved for internal use of the tag management functions.

Currently, we define

JPGTAG_TAG_DONE   to abort a tag list,
JPGTAG_TAG_MORE   to link two tag lists together, i.e. to continue
                  parsing a tag list at different position,
JPGTAG_TAG_IGNORE to simply skip a single tag item and
JPGTAG_TAG_SKIP   to skip one or more tag items at once.

Tag item methods keep care about these tags already, no need
to worry about them.
* */
///

/// struct JPG_TagItem
// More macros for data handling
// Unfortunately, C++ doesn't cast from void * 
// automatically.
#ifdef __cplusplus
#define JPG_PointerTag(id,ptr)  JPG_TagItem(id,(JPG_APTR)(ptr))
#define JPG_ValueTag(id,v)      JPG_TagItem(id,(JPG_LONG)(v))
#define JPG_FloatTag(id,f)      JPG_TagItem(id,(JPG_FLOAT)(f))
#define JPG_Continue(tag)       JPG_TagItem(JPGTAG_TAG_MORE,const_cast<struct JPG_TagItem *>(tag))
#define JPG_EndTag              JPG_TagItem(JPGTAG_TAG_DONE)
#endif

// For ANSI-C conformance, this is a struct.
// All fields are public anyhow, so this isn't a loss.
struct JPG_EXPORT JPG_TagItem {
  JPG_Tag ti_Tag;     // identifies the type of data
  //
  // private tag contents fields.
  //
  union JPG_EXPORT TagContents {
    JPG_LONG       ti_lData;
    JPG_FLOAT      ti_fData;
    JPG_APTR       ti_pPtr;
    //
#ifdef __cplusplus
    // Constructors
    TagContents(JPG_LONG data)
    {
#if CHECK_LEVEL > 0
      ti_pPtr  = (void *)(0xdeadbef);
#else
      ti_pPtr  = 0;
#endif
      ti_lData = data;
    }
    //
    TagContents(JPG_FLOAT data)
    {
#if CHECK_LEVEL > 0
      ti_pPtr  = (void *)(0xdeadbef);
#else
      ti_pPtr  = 0;
#endif
      ti_fData = data;
    }
    //
    TagContents(JPG_APTR data)
      : ti_pPtr(data)
    { }
    //
    TagContents(void)
    { }
#endif
  } ti_Data;
  //
#ifdef __cplusplus
  // and now for the member functions:
  // Constructors
  JPG_TagItem(JPG_Tag tag, JPG_LONG data) 
    : ti_Tag(tag), ti_Data(data)
  { }
  //
  JPG_TagItem(JPG_Tag tag, JPG_FLOAT data) 
    : ti_Tag(tag), ti_Data(data)
  { }
  //
  JPG_TagItem(JPG_Tag tag, JPG_APTR ptr = 0)
    : ti_Tag(tag), ti_Data(ptr)
  { }
  //
  JPG_TagItem(void)
  { }
  // Returns the next tag item of this tag item or NULL
  // if this is the last item in the list.
  struct JPG_TagItem *NextTagItem(void);
  //
  // The same for const
  const struct JPG_TagItem *NextTagItem(void) const
  {
    return const_cast<struct JPG_TagItem *>(this)->NextTagItem();
  }
  //
  // Scans the tag list for a specific entry. Returns NULL if not available.
  struct JPG_TagItem *FindTagItem(JPG_Tag id);
  //
  // The same as const
  const struct JPG_TagItem *FindTagItem(JPG_Tag id) const
  {
    return const_cast<struct JPG_TagItem *>(this)->FindTagItem(id);
  }
  //
  // Attach a new tag list at the end of this tag list. Returns
  // a pointer to the point where the addendum was made.
  struct JPG_TagItem *TagOn(struct JPG_TagItem *add);
  //
  // Link a tag list pointer into the given tag. Return the tag.
  // Note that TAG_MORE must already been installed here.
  const struct JPG_TagItem *Continue(const struct JPG_TagItem *add)
  {
    ti_Data.ti_pPtr = const_cast<struct JPG_TagItem *>(add);
    return this;
  }
  //
  // Searches the tag list for a specific entry. Returns ti_Data if
  // found. Returns the default otherwise.
  JPG_LONG GetTagData(JPG_Tag id, JPG_LONG defdata = 0) const;
  //
  // Return a float data from the tag list.
  JPG_FLOAT GetTagFloat(JPG_Tag id, JPG_FLOAT defdata = 0.0) const;
  //
  // Scans the tag list for a specific entry. Returns ti_Ptr if 
  // found. Returns the default otherwise.
  JPG_APTR GetTagPtr(JPG_Tag id, JPG_APTR defptr = 0) const;
  //
  // If the given tag item is contained in the tag list, set its
  // value to the given value
  void SetTagData(JPG_Tag id, JPG_LONG data);
  //
  // Install a floating point value into the tag list.
  void SetTagFloat(JPG_Tag id, JPG_FLOAT data);
  //
  // Scans the tag list and fills the pointer in if the given
  // tag is found. Does nothing otherwise.
  void SetTagPtr(JPG_Tag id, JPG_APTR ptr);  
  //
  // Set the internal "tag set" flag.
  void SetTagSet(void);
  //
  // Clear the internal "tag set" flags recursively,
  // set unset tags to "IGNORE". This is used to 
  // filter out unused tags, and to make set tags valid.
  void ClearTagSets(void);
  //
  // Filtering of tags. The following method takes the current
  // tag list, a tag list of defaults and a new taglist that
  // should better be large enough. It takes the defaults,
  // moves them to the new taglist and overrides its setting
  // with that of this taglist. It returns the lenght of the
  // new taglist. If the target taglist is NULL, it is not
  // filled in, but tags are just counted. The drop argument
  // contains tags that are to be removed from the defaults
  // before they get attached to the target.
  static JPG_LONG FilterTags(struct JPG_TagItem *target,const struct JPG_TagItem *source,
                             const struct JPG_TagItem *defaults,const struct JPG_TagItem *drop);
  //
#endif
};
///

/// Examples
// The following presents an example for tag item usage:
/*
ULONG Compress(class JPG_TagItem *in)
{
  Picture *in;
  FILE *stream;
  JPG_ULONG width,height;

  in     = in->GetTagPtr(TAG_SOURCE,NULL);
  stream = in->GetTagPtr(TAG_DESTINATION,NULL); 
  width  = in->GetTagData(TAG_WIDTH,in->Width());
  height = in->GetTagData(TAG_HEIGHT,in->Height());
  // ... etc.... 

  if (in==NULL || stream==NULL || width==0 || height==0)
    THROW(INVALID_PARAMETERS,0);

  ....
}
*/

// and here is how you *could* call a function:
//
/*
void UserFunction(FILE *out,Picture *in)
{
  class JPG_TagItem tags[] = {
    PointerTag(TAG_SOURCE,in),
    PointerTag(TAG_DEST,out),
    EndTag
  }

  Compress(tags);
}
*/
///

///
#endif // TAGITEM_H
