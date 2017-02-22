/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2017 Thomas Richter, University of Stuttgart and
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
** This file provides the transformation from RGB to YCbCr in the floating
** point profiles, profiles A and B of 18477-7.
**
** $Id: multiplicationtrafo.hpp,v 1.5 2014/10/13 13:06:42 thor Exp $
**
*/

#ifndef COLORTRAFO_MULTIPLICATIONTRAFO_HPP
#define COLORTRAFO_MULTIPLICATIONTRAFO_HPP

/// Includes
#include "tools/traits.hpp"
#include "colortrafo/colortrafo.hpp"
#include "colortrafo/trivialtrafo.hpp"
#endif
