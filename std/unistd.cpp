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

/// Includes
#include "unistd.hpp"
#include "math.hpp"
///

#ifndef HAVE_SLEEP
#ifdef WIN32
// Comes with its own "sleep" version
#include <windows.h>
unsigned int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}
#else
unsigned int sleep(unsigned int seconds)
{
  int x;
  
  // Pretty stupid default implementation
  seconds <<= 8;
  while(seconds > 0) {
    double y = 1.1;
    for(x = 0;x < 32767;x++) {
      // Just keep it busy.
      y = pow(y,y);
      if (y > 2.0)
        y /= 2.0;
      
    }
    seconds--;
  }
  return 0;
}
#endif
#endif

  
/// longseek
// A generic 64-bit version of seek
long long longseek(int stream,long long offset,int whence)
{
#if defined(HAVE_LSEEK64)
  return lseek64(stream,offset,whence);
#elif defined(HAVE_LLSEEK)
  return llseek(stream,offset,whence);
#elif defined(HAVE_LSEEK)
  return lseek(stream,offset,whence);
#elif defined(HAVE__LSEEKI64)
  return _lseeki64(stream,offset,whence);
#else
  // No seeking possible.
  return -1;
#endif
}
///
