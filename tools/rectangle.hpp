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
 * Definition of a rectangle.
 * 
 * $Id: rectangle.hpp,v 1.2 2012-06-02 10:27:14 thor Exp $
 *
 * This defines a rectangular array of pixels in some domain.
 * It is just a structure, not a class, and only used as a
 * tiny helper structure that I'm going to use here and there.
 *
 */

#ifndef TOOLS_RECTANGLE_HPP
#define TOOLS_RECTANGLE_HPP

/// Includes
#include "interface/types.hpp"
///

/// Design
/** Design
******************************************************************
** struct RectAngle						**
** Super Class:	none						**
** Sub Classes: none						**
** Friends:							**
******************************************************************

Defines simply a rectangular pair of coordinates within whatever.
Typically of either WORD or LONG type.

It provides functions that clips one rectangle to the interiour
of another, or checks whether a rectangle is empty or not.

Rectangle coordinates are understood to be always inclusive,
for the minimum as well as for the maximum coordinates. Keep
care of this when computing the size of a rectangle "in pixels".

* */
///

/// Rectangle templated structure
// This structure defines a rectangle in the pixel domain.
// it should be templated to either LONG or WORD.
template <class T> struct RectAngle {
  T ra_MinX,ra_MinY;   // Minimum coordinate, inclusively
  T ra_MaxX,ra_MaxY;   // Maximum coordinate, inclusively
  // Check whether a rectangle is empty.
  bool IsEmpty(void) const
  {
    return (((ra_MinX > ra_MaxX) || (ra_MinY > ra_MaxY))?true:false);
  }
  //
  // ClipRect: Clip a rectangle into a (supposed to be) larger
  void ClipRect(const RectAngle<T> &bounds)
  {
    if (ra_MinX < bounds.ra_MinX)
      ra_MinX = bounds.ra_MinX;
    
    if (ra_MinY < bounds.ra_MinY)
      ra_MinY = bounds.ra_MinY;
    
    if (ra_MaxX > bounds.ra_MaxX)
      ra_MaxX = bounds.ra_MaxX;
    
    if (ra_MaxY > bounds.ra_MaxY)
      ra_MaxY = bounds.ra_MaxY;
  }
  //
  // Return the width of the rectangle in pixels.
  T WidthOf(void) const
  {
    return ra_MaxX - ra_MinX + 1;
  }
  // Return the height of the rectangle in pixels
  T HeightOf(void) const
  {
    return ra_MaxY - ra_MinY + 1;
  }
  // EnlargeOver: Enlarge a rectangle such that it covers the 
  // passed rectangle. This is somehow the reverse of the above.
  void EnlargeOver(const RectAngle<T> &within)
  {
    if (ra_MinX > within.ra_MinX)
      ra_MinX = within.ra_MinX;

    if (ra_MinY > within.ra_MinY)
      ra_MinY = within.ra_MinY;

    if (ra_MaxX < within.ra_MaxX)
      ra_MaxX = within.ra_MaxX;

    if (ra_MaxY < within.ra_MaxY)
      ra_MaxY = within.ra_MaxY;
  }
  //
  // Move a rectangle by a given amount
  void MoveRect(LONG deltax,LONG deltay)
  {
    ra_MinX  -= deltax;
    ra_MaxX  -= deltax;
    ra_MinY  -= deltay;
    ra_MaxY  -= deltay;
  }
  //
  // Equality operator between rectangles.
  bool operator==(const RectAngle<T> &cmp) const
  {
    if (ra_MinX != cmp.ra_MinX)
      return false;
    if (ra_MinY != cmp.ra_MinY)
      return false;
    if (ra_MaxX != cmp.ra_MaxX)
      return false;
    if (ra_MaxY != cmp.ra_MaxY)
      return false;
    return true;
  } 
  //
  // Inequality between rectangles.
  bool operator!=(const RectAngle<T> &cmp) const 
  {
    if (ra_MinX != cmp.ra_MinX)
      return true;
    if (ra_MinY != cmp.ra_MinY)
      return true;
    if (ra_MaxX != cmp.ra_MaxX)
      return true;
    if (ra_MaxY != cmp.ra_MaxY)
      return true;
    return false;
  }
  // 
  // Check whether this rectangle is covered by the 
  // given rectangle completely. Returns true if so.
  bool IsCoveredBy(const RectAngle<T> &cmp) const
  {
    if (ra_MinX >= cmp.ra_MinX &&
	ra_MinY >= cmp.ra_MinY &&
	ra_MaxX <= cmp.ra_MaxX &&
	ra_MaxY <= cmp.ra_MaxY)
      return true;
    return false;
  }
  //
  // Check whether this rectangle intersects with another
  // rectangle. Returns true if so.
  bool Intersects(const RectAngle<T> &cmp) const
  {
    if (ra_MinX > cmp.ra_MaxX ||
	ra_MaxX < cmp.ra_MinX ||
	ra_MinY > cmp.ra_MaxY ||
	ra_MaxY < cmp.ra_MinY)
      return false;
    return true;
  }
  //
  // Check whether a given point is contained in the rectangle.
  bool Contains(T x,T y) const
  {
    if (x >= ra_MinX && x <= ra_MaxX &&
	y >= ra_MinY && y <= ra_MaxY)
      return true;
    return false;
  }
};
///

///
#endif
