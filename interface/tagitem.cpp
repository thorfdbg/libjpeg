/*
 * Tag item definitions
 * 
 * $Id: tagitem.cpp,v 1.8 2014/09/30 08:33:17 thor Exp $
 *
 * Tag items provide a convenient mechanism to build functions with are
 * easely extendable. A tag item consists of:
 *
 * A tag field that identifies the parameter type,
 * the parameter itself.
 *
 * Special system parameters exist to link, filter, etc... tag items.
 *
 */

/// Includes
#include "tagitem.hpp"
#include "types.hpp"
#include "tools/environment.hpp"
///

/// JPG_TagItem::NextTagItem
struct JPG_TagItem *JPG_TagItem::NextTagItem(void)
{
  struct JPG_TagItem *current = this;

  if (current == NULL)
    return NULL;
  //
  // Just a standard user tag. The next one is behind this
  // one.
  if (current->ti_Tag & JPGTAG_TAG_USER)
    current++;

  while(true) {
    switch (current->ti_Tag) {
    case JPGTAG_TAG_DONE: 
      return NULL;  // This is the end of the tag list. Return NULL
    case JPGTAG_TAG_MORE:
      // go to the next list
      current = (struct JPG_TagItem *)(current->ti_Data.ti_pPtr); 
      if (current == NULL)
	return NULL;
      continue;
    case JPGTAG_TAG_SKIP:
      current = current + 1 + current->ti_Data.ti_lData; // skip this and the next n items
      continue;
    default: 
      if (current->ti_Tag & JPGTAG_TAG_USER)
	return current;
      // Runs into the following
    case JPGTAG_TAG_IGNORE:    
      current++;
      continue;
    }
  }
   
  return NULL;
}
///

/// JPG_TagItem::FindTagItem
struct JPG_TagItem *JPG_TagItem::FindTagItem(JPG_Tag id)
{
  struct JPG_TagItem *current = this;
  
  if (current == NULL)
    return NULL;
  
  while(true) {
    switch (current->ti_Tag) {
    case JPGTAG_TAG_DONE: 
      return NULL;  // This is the end of the tag list. Return NULL
    case JPGTAG_TAG_MORE:
      // go to the next list
      current = (struct JPG_TagItem *)(current->ti_Data.ti_pPtr); 
      if (current == NULL)
	return NULL;
      continue;
    case JPGTAG_TAG_SKIP:
      current = current + 1 + current->ti_Data.ti_lData; // skip this and the next n items
      continue;
    default: 
      if (current->ti_Tag & JPGTAG_TAG_USER) {
	if (current->ti_Tag == id)
	  return current;
      }
      // Runs into the following
    case JPGTAG_TAG_IGNORE:    
      current++;
      continue;
    }
  }

  return NULL;
}
///

/// JPG_TagItem::GetTagData
JPG_LONG JPG_TagItem::GetTagData(JPG_Tag id,JPG_LONG defdata) const
{
  const struct JPG_TagItem *current;

  if ((current = FindTagItem(id)) != NULL) {
    return current->ti_Data.ti_lData;
  }

  return defdata;
}
///

/// JPG_TagItem::GetTagFloat
JPG_FLOAT JPG_TagItem::GetTagFloat(JPG_Tag id,JPG_FLOAT defdata) const
{
  const struct JPG_TagItem *current;

  if ((current = FindTagItem(id)) != NULL) {
    return current->ti_Data.ti_fData;
  }

  return defdata;
}
///


/// JPG_TagItem::GetTagPtr
APTR JPG_TagItem::GetTagPtr(JPG_Tag id,APTR defptr) const
{
  const struct JPG_TagItem *current;

  if ((current = FindTagItem(id)) != NULL) {
    return current->ti_Data.ti_pPtr;
  }

  return defptr;
}
///

/// JPG_TagItem::SetTagData
void JPG_TagItem::SetTagData(JPG_Tag id,JPG_LONG data)
{
  struct JPG_TagItem *current;

  if ((current = FindTagItem(id)) != NULL) {
    current->ti_Data.ti_lData = data;
  }
}
///

/// JPG_TagItem::SetTagFloat
void JPG_TagItem::SetTagFloat(JPG_Tag id,JPG_FLOAT data)
{
  struct JPG_TagItem *current;

  if ((current = FindTagItem(id)) != NULL) {
    current->ti_Data.ti_fData = data;
  }
}
///

/// JPG_TagItem::SetTagPtr
void JPG_TagItem::SetTagPtr(JPG_Tag id,APTR ptr)
{
  struct JPG_TagItem *current;

  if ((current = FindTagItem(id)) != NULL) {
    current->ti_Data.ti_pPtr = ptr;
  }
}
///

/// JPG_TagItem::SetTagSet
// Set the internal "tag set" flag.
void JPG_TagItem::SetTagSet()
{
  ti_Tag |= JPGTAG_SET;
}
///

/// JPG_TagItem::ClearTagSets
// Clear the internal "tag set" flags recursively.
void JPG_TagItem::ClearTagSets()
{
  struct JPG_TagItem *current = this;

  while (current) {
    if (current->ti_Tag & JPGTAG_SET) {
      current->ti_Tag &= ~JPGTAG_SET;
    } else {
      // This tag is not set, drop it.
      current->ti_Tag = JPGTAG_TAG_IGNORE;
    }
    current = current->NextTagItem();
  }
}
///

/// JPG_TagItem::FilterTags
// Filtering of tags. The following method takes the current
// tag list, a tag list of defaults and a new taglist that
// should better be large enough. It takes the defaults,
// moves them to the new taglist and overrides its setting
// with that of this taglist. It returns the lenght of the
// new taglist. If the target taglist is NULL, it is not
// filled in, but tags are just counted.
JPG_LONG JPG_TagItem::FilterTags(struct JPG_TagItem *target,const struct JPG_TagItem *source,
				 const struct JPG_TagItem *defaults,const struct JPG_TagItem *drop)
{
  LONG count = 0;
  const struct JPG_TagItem *parse = source;
  //
  while(parse) {
    if (parse->ti_Tag & JPGTAG_TAG_USER) {
      // Got a setting tag, carry it over.
      if (target) {
	*target++ = *parse;
      }
      count++;
    }
    parse = parse->NextTagItem();
  }
  //
  // Now go for the list of defaults. Whenever we do not find
  // a default on the parent list, attach it.
  while(defaults) {
    if (defaults->ti_Tag & JPGTAG_TAG_USER) {
      // Check whether there are any tags we shall not add from
      // the defaults.
      if (drop == NULL || drop->FindTagItem(defaults->ti_Tag) == NULL) {
	if (source == NULL || source->FindTagItem(defaults->ti_Tag) == NULL) {
	  // Ok, we know nothing about this value, attach.
	  if (target) {
	    *target++ = *defaults;
	  }
	  count++;
	}
      }
    }
    defaults = defaults->NextTagItem();
  }
  //
  // Now attach an End-Tag.
  if (target) {
    target->ti_Tag           = JPGTAG_TAG_DONE;
    target->ti_Data.ti_lData = 0;
  }
  count++;
  return count;
}
///

/// JPG_TagItem::TagOn
// Attach a new tag list at the end of this tag list.
struct JPG_TagItem *JPG_TagItem::TagOn(struct JPG_TagItem *add)
{
  struct JPG_TagItem *current = this;
  
  while(current) {
    switch (current->ti_Tag) {
    case JPGTAG_TAG_DONE: 
      // This is the end of the tag list, attach here.
      current->ti_Tag          = JPGTAG_TAG_MORE;
      current->ti_Data.ti_pPtr = add;
      return current;
    case JPGTAG_TAG_IGNORE:
      current++;    // Just skip the current item and continue
      break;
    case JPGTAG_TAG_MORE:
      // go to the next list
      current = (struct JPG_TagItem *)(current->ti_Data.ti_pPtr); 
      break;
    case JPGTAG_TAG_SKIP:
      current = current + 1 + current->ti_Data.ti_lData; // skip this and the next n items
      break;
    default:
      // User tag, skip
      current++;
      break;
    }
  }
  //
  // Shouldn't go here, shuts up g++.
  return NULL;
}
///
