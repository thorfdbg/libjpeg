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
 * Definition of how to request a given rectangle for display,
 * for load or for checking for a necessary update.
 * 
 * $Id: rectanglerequest.hpp,v 1.4 2012-06-02 10:27:13 thor Exp $
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
** struct RectangleRequest					**
** Super Class:	none						**
** Sub Classes: none						**
** Friends:							**
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
  UBYTE                    rr_ucUpsampling;     // zero: no upsampling. Otherwise dowscale factor, 1 for full upsampling
  bool                     rr_bColorTransform;  // enable the YCbCr to RGB transformation
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
