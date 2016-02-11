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
** A JPEG LS scan covering only a single component.
**
** $Id: singlecomponentlsscan.hpp,v 1.8 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CODESTREAM_SINGLECOMPONENTLSSCAN_HPP
#define CODESTREAM_SINGLECOMPONENTLSSCAN_HPP

/// Includes
#include "codestream/jpeglsscan.hpp"
///

/// class SingleComponentLSScan
class SingleComponentLSScan : public JPEGLSScan {
  //
  //
public:
  // Create a new scan. This is only the base type.
  SingleComponentLSScan(class Frame *frame,class Scan *scan,UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~SingleComponentLSScan(void);
  // 
  // Parse a single MCU in this scan. Return true if there are more
  // MCUs in this row.
  virtual bool ParseMCU(void);
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void); 
};
///


///
#endif

