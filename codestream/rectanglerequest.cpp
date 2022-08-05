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
 * Definition of how to request a given rectangle for display,
 * for load or for checking for a necessary update.
 * 
 * $Id: rectanglerequest.cpp,v 1.17 2022/08/05 11:25:28 thor Exp $
 *
 */

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "interface/tagitem.hpp"
#include "codestream/image.hpp"
#include "rectanglerequest.hpp"
#include "interface/parameters.hpp"
///

/// RectangleRequest::ParseTags
// Queues the request in the rectangle request structure, return a
// boolean indicator that is true when the full canvas has been
// requested.
void RectangleRequest::ParseTags(const struct JPG_TagItem *tags,const class Image *image)
{
  class Environ *m_pEnviron = image->EnvironOf(); // Faked for the exceptions
  LONG coord;
  //
  rr_Request.ra_MinX    = 0;
  rr_Request.ra_MinY    = 0;
  rr_Request.ra_MaxX    = image->WidthOf()-1;
  rr_Request.ra_MaxY    = image->HeightOf();
  if (rr_Request.ra_MaxY == 0) {
    rr_Request.ra_MaxY = MAX_LONG; // Height is not yet defined
  } else {
    rr_Request.ra_MaxY--;
  }
  rr_usFirstComponent   = 0;
  rr_usLastComponent    = image->DepthOf() - 1;
  //
  /*
  rr_usUpToLayer        = MAX_UWORD; // exclusive bound
  rr_ucThumbSize        = 0;
  */
  rr_cPriority          = 0;
  rr_bUpsampling        = true;
  rr_bIncludeAlpha      = false;
  rr_bColorTrafo        = true;
  //
  // Changed a bit the reaction on coordinates: We no longer throw
  // errors, but rather clip into the valid coordinates. This goes
  // consistent with the handling of the resolution and component
  // coordinates.
  //
  while(tags) {
    coord = tags->ti_Data.ti_lData;
    switch (tags->ti_Tag) { 
    case JPGTAG_DECODER_MINX:
      if (coord < 0)
        JPG_THROW(OVERFLOW_PARAMETER,"RectangleRequest::ParseFromTagList",
                  "Rectangle MinX underflow, must be >= 0");
      if (coord > rr_Request.ra_MinX) {
        rr_Request.ra_MinX = coord;
      }
      break;
    case JPGTAG_DECODER_MINY:
      if (coord < 0)
        JPG_THROW(OVERFLOW_PARAMETER,"RectangleRequest::ParseFromTagList",
                  "Rectangle MinY underflow, must be >= 0");
      if (coord > rr_Request.ra_MinY) {
        rr_Request.ra_MinY = coord;
      }
      break;
    case JPGTAG_DECODER_MAXX:
      if (coord < 0)
        JPG_THROW(OVERFLOW_PARAMETER,"RectangleRequest::ParseFromTagList",
                  "Rectangle MaxX underflow, must be >= 0");
      if (coord < rr_Request.ra_MaxX) {
        rr_Request.ra_MaxX = coord;
      }
      break;
    case JPGTAG_DECODER_MAXY:
      if (coord < 0)
        JPG_THROW(OVERFLOW_PARAMETER,"RectangleRequest::ParseFromTagList",
                  "Rectangle MaxY underflow, must be >= 0");
      if (coord < rr_Request.ra_MaxY) {
        rr_Request.ra_MaxY = coord;
      }
      break;
    case JPGTAG_DECODER_MINCOMPONENT:
      if (coord < 0 || coord > MAX_UWORD)
        JPG_THROW(OVERFLOW_PARAMETER,"RectangleRequest::ParseFromTagList",
                  "MinComponent overflow, must be >= 0 && < 65536");
      if (UWORD(coord) > rr_usFirstComponent) {
        rr_usFirstComponent = coord;
      }
      break;
    case JPGTAG_DECODER_MAXCOMPONENT:
      if (coord < 0 || coord > MAX_UWORD)
        JPG_THROW(OVERFLOW_PARAMETER,"RectangleRequest::ParseFromTagList",
                  "MaxComponent overflow, must be >= 0 && < 65536");
      if (UWORD(coord) < rr_usLastComponent) {
        rr_usLastComponent = coord;
      }
      break;
    case JPGTAG_DECODER_INCLUDE_ALPHA:
      rr_bIncludeAlpha = (coord != 0)?true:false;
      break;
    case JPGTAG_DECODER_UPSAMPLE:
      rr_bUpsampling   = (coord != 0)?true:false;
      break;
    case JPGTAG_MATRIX_LTRAFO:
      rr_bColorTrafo   = (coord != JPGFLAG_MATRIX_COLORTRANSFORMATION_NONE)?true:false;
      break;
    }
    tags = tags->NextTagItem();
  }
  //
  // If upsampling is disabled, disable the color transformation as well.
  if (!rr_bUpsampling)
    rr_bColorTrafo = FALSE;
  // Make a consistency check for the rectangle. Otherwise, the decoder
  // will fall over...
  if (rr_Request.IsEmpty())
    JPG_THROW(INVALID_PARAMETER,"RectangleRequest::ParseFromTagList",
              "the requested rectangle is empty");
}
///

/// RectangleRequest::Contains
// Check whether this request contains the argument as sub-request, i.e.
// whether requesting this request first and then the sub-request as
// argument does nothing.
bool RectangleRequest::Contains(const struct RectangleRequest *sub) const
{

  // First, check whether the rectangle of sub is contained
  // in our rectangle.
  if (sub->rr_Request.ra_MinX < rr_Request.ra_MinX)
    return false;

  if (sub->rr_Request.ra_MinY < rr_Request.ra_MinY)
    return false;

  if (sub->rr_Request.ra_MaxX > rr_Request.ra_MaxX)
    return false;

  if (sub->rr_Request.ra_MaxY > rr_Request.ra_MaxY)
    return false;

  //
  // Then check the same for the component interval
  if (sub->rr_usFirstComponent < rr_usFirstComponent)
    return false;
  
  if (sub->rr_usLastComponent < rr_usLastComponent)
    return false;

  // Otherwise, the sub request is truly superfluous
  // and need not to be issued again.
  return true;

}
///

/// RectangleRequest::Enqueue
// Enqueue a (this) rectangle into a list according to its priority.
void RectangleRequest::Enqueue(struct RectangleRequest *&first)
{
  struct RectangleRequest **current = &first;

  for(;;) {
    if (*current == NULL || (*current)->rr_cPriority < rr_cPriority) {
      // Enqueue in front of "*current".
      if (*current && (*current)->Contains(this)) {
        // This request is just superfluous. Dispose.
        delete this;
      } else {
        rr_pNext = *current;
        *current = this;
      }
      return;
    }
    current = &((*current)->rr_pNext);
  }
}
///

