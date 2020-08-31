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
** This helper class keeps and updates the checksum over the legacy
** JPEG stream.
**
** $Id: checksum.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef TOOLS_CHECKSUM_HPP
#define TOOLS_CHECKSUM_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
///

/// Defines
//#define TESTING
#ifdef TESTING
#include "std/stdio.hpp"
#endif
/// 

/// class Checksum
// This helper class keeps and updates the checksum over the legacy
// JPEG stream.
class Checksum : public JObject {
  //
  // Consists of two bytes.
  UBYTE m_ucCount1;
  UBYTE m_ucCount2;
  //
#ifdef TESTING
  FILE *tmpout;
  int line;
#endif
  //
public:
  Checksum(void)
  : m_ucCount1(0), m_ucCount2(0)
  { 
#ifdef TESTING
    tmpout = fopen("/tmp/chksum","w"); 
    line   = 0;
#endif
  }
  //
  ~Checksum(void)
  {
#ifdef TESTING
    if (tmpout)
      fclose(tmpout);
    tmpout = NULL;
#endif
  }
  //
  // Return the checksum so far.
  ULONG ValueOf(void) const
  {
    return m_ucCount1 | (m_ucCount2 << 8);
  }
  //
  // Update the checksum for a data block.
  void Update(const UBYTE *b,ULONG size)
  {
    UWORD sum;
    //
    while(size) {
      sum  = m_ucCount1;
#ifdef TESTING
      if (tmpout) {
        fprintf(tmpout,"%10d\t%02x\n",line++,*b);
      }
#endif
      sum += *b++;
      sum += (sum + 1) >> 8; // sum modulo 255
      m_ucCount1 = UBYTE(sum);
      assert(m_ucCount1 != 0xff);
      
      sum  = m_ucCount2;
      sum += m_ucCount1;
      sum += (sum + 1) >> 8;
      m_ucCount2 = UBYTE(sum);
      assert(m_ucCount2 != 0xff);
      
      size--;
    }
  }
  //
  // Update the checksum for a single byte.
  void Update(UBYTE b)
  {
    UWORD sum;
    //
    sum  = m_ucCount1;
#ifdef TESTING
    if (tmpout) {
      fprintf(tmpout,"%10d\t%02x\n",line++,b);
    }
#endif
    sum += b;
    sum += (sum + 1) >> 8; // sum modulo 255
    m_ucCount1 = UBYTE(sum);
    assert(m_ucCount1 != 0xff);
      
    sum  = m_ucCount2;
    sum += m_ucCount1;
    sum += (sum + 1) >> 8;
    m_ucCount2 = UBYTE(sum);
    assert(m_ucCount2 != 0xff);
  }
};
///

///
#endif
