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
 * Definition of how to request a given rectangle for display,
 * for load or for checking for a necessary update.
 * 
 * $Id: rectanglerequest.hpp,v 1.10 2015/03/11 16:02:42 thor Exp $
 *
 */

#ifndef CODESTREAM_RECTANGLEREQUEST_HPP
#define CODESTREAM_RECTANGLEREQUEST_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/rectangle.hpp"
#include "tools/environment.hpp"
#include "std/string.hpp"
///

/// Forward references
struct JPG_TagItem;
class Image;
///

/// Design
/** Design
******************************************************************
** struct RectangleRequest                                      **
** Super Class: none                                            **
** Sub Classes: none                                            **
** Friends:                                                     **
******************************************************************

Defines a rectangular image domain that is requested from the
user to be loaded or to be previewed. Hence, the purpose of
this helper class is to pass parameters about the user request
information down the jpeg stream.

This structure is constructed by the decoder, and sent down
to the band class which will generate the requests for each
code block. On its way, more and more components gets parsed
and get interpreted.
* */
///

/// RectangleRequest
// This structure defines the sub-image requested by DisplayRectangle()
// and similar functions. It is consistently used to define an image
// area and as such forwarded to the canvas and all its sub-structures
struct RectangleRequest : public JObject, private Explicit { 
  //
  struct RectangleRequest *rr_pNext;
  RectAngle<LONG>          rr_Request;
  UWORD                    rr_usFirstComponent; // starting component
  UWORD                    rr_usLastComponent;  // inclusive end component
  BYTE                     rr_cPriority;        // order of rectangles
  bool                     rr_bIncludeAlpha;    // include the alpha channel in the request
  //
  RectangleRequest(void)
    : rr_pNext(NULL)
  { }
  //
  // Copy constructor.
  RectangleRequest(const struct RectangleRequest &req)
    : Explicit()
  {
    // Not nice, but this is really faster and simpler
    memcpy(this,&req,sizeof(struct RectangleRequest));
    // Not linked in any way if this is new.
    rr_pNext = NULL;
  }
  //
  // Assignment operator.
  RectangleRequest &operator=(const struct RectangleRequest &req)
  { 
    // Not nice, but this is really faster and simpler
    memcpy(this,&req,sizeof(struct RectangleRequest));
    // Not linked in any way if this is new.
    rr_pNext = NULL;
    //
    return *this;
  }
  //
  //
  // Queues the request in the rectangle request structure.
  void ParseTags(const struct JPG_TagItem *tags,const class Image *image);
  //
  // Check whether this request contains the argument as sub-request, i.e.
  // whether requesting this request first and then the sub-request as
  // argument does nothing.
  bool Contains(const struct RectangleRequest *sub) const;
  // 
  // Check whether this rectangle intersects with another
  // rectangle. Returns true if so.
  bool Intersects(const RectAngle<LONG> &cmp) const
  {
    return rr_Request.Intersects(cmp);
  }
  //
  // Returns the next rectangle in a singly linked list
  struct RectangleRequest *NextOf(void) const 
  {
    return rr_pNext;
  }
  //
  // Enqueue a rectangle into a list according to its priority.
  void Enqueue(struct RectangleRequest *&first);
  //
};
///

///
#endif
