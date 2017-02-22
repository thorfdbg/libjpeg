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
** This class represents a quantization table of 64 UWORD
** entries, plus the statistics that have been measured for
** this particular table to optimize the quality.
**
** $Id: quantizationtable.cpp,v 1.2 2016/10/28 13:58:54 thor Exp $
**
*/

/// Includes
#include "std/string.hpp"
#include "std/math.hpp"
#include "tools/environment.hpp"
#include "marker/quantizationtable.hpp"
///

/// QuantizationTable::DefineBucketSizes
// Create a table from 64 already defined quantizer values.
void QuantizationTable::DefineBucketSizes(const UWORD table[64])
{
  memcpy(m_usDelta,table,sizeof(m_usDelta));
}
///
